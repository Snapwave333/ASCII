#include "AIDirector.h"
#include <algorithm>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <exception>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif
#include <nlohmann/json.hpp>
#include "ScenarioManager.h"
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include "cache/SegmentedLRUCache.h"
#include "cache/RedisClient.h"
#include "cache/CacheMonitor.h"
#include "cache/SegmentedLRUCache.h"
#pragma comment(lib, "winhttp.lib")
#endif

namespace NeonGlyph {

// Safe string to float conversion with error handling
static float SafeStof(const std::string& str, float default_val = 0.0f) {
    try {
        if (str.empty()) return default_val;
        return std::stof(str);
    } catch (const std::exception& e) {
        std::cerr << "[AIDirector] Warning: Failed to parse float value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

// Safe string to integer conversion with error handling
static int SafeStoi(const std::string& str, int default_val = 0) {
    try {
        if (str.empty()) return default_val;
        return std::stoi(str);
    } catch (const std::exception& e) {
        std::cerr << "[AIDirector] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

AIDirector::AIDirector() : m_initialized(false) {}
AIDirector::~AIDirector() { Shutdown(); }

Result AIDirector::Initialize(const Config& config) {
    m_config = config;
    m_initialized.store(true);
    m_state.current_scene_id = "SCN-001";
    m_state.intent_mode = "abstract_flow";
    m_state.palette = "ambient_color_palette";
    m_state.glyph_map = "default_ascii";
    m_state.nsfw_blocked = true;
    m_lastTransition = std::chrono::steady_clock::now();
    m_lastSection = "intro";
    
    // Initialize story system
    m_story_clock = std::make_unique<StoryClock>();
    m_novelty_detector = std::make_unique<NoveltyDetector>();
    m_silence_detector = std::make_unique<AudioSilenceDetector>();
    
    m_novelty_detector->Initialize(0.3f, 0.5f, 50); // local_threshold, act_threshold, history_size
    m_silence_detector->Initialize(-40.0f, 3000); // threshold_db, grace_period_ms
    
    m_set_start_time_ms = static_cast<int64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    m_palette_manager = std::make_unique<PaletteManager>();
    m_palette_manager->LoadFromJson("config/palettes.json");
    LoadSceneWeights("config/scene_weights.json");
    
    if (m_config.llm.enabled) {
        m_shouldStop.store(false);
        m_aiThread = std::thread(&AIDirector::AIWorkerLoop, this);
        std::cout << "[AIDirector] Worker thread started" << std::endl;
#ifdef _WIN32
        HANDLE h = (HANDLE)m_aiThread.native_handle();
        SetThreadPriority(h, THREAD_PRIORITY_ABOVE_NORMAL);
#endif
    }
    return Result::Success;
}

void AIDirector::Shutdown() {
    m_initialized.store(false);
    m_shouldStop.store(true);
    if (m_aiThread.joinable()) m_aiThread.join();
    
    // Cleanup story system
    if (m_story_clock) m_story_clock->Shutdown();
    m_story_clock.reset();
    m_novelty_detector.reset();
    m_silence_detector.reset();
}

DirectorCommand AIDirector::Update(const MusicData& m) {
    UpdateStateFromMusic(m);

    // Update story context if enabled
    if (m_story_enabled && m_story_clock && m_silence_detector) {
        int64 current_time_ms = static_cast<int64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        int64 set_time_ms = current_time_ms - m_set_start_time_ms;
        
        // Process audio for silence detection
        // Convert energy (0.0-1.0) to dB scale for silence detection
        float energy_db = 20.0f * std::log10((std::max)(m.energy, 0.001f));
        bool is_silent = m_silence_detector->ProcessAudioFrame(energy_db, current_time_ms);
        
        // Update story context
        m_story_context = m_story_clock->Update(set_time_ms, is_silent);
        
        // Handle blackout state
        if (is_silent && !m_story_context.is_blackout) {
            m_story_context.is_blackout = true;
            return CreateBlackoutCommand();
        } else if (!is_silent && m_story_context.is_blackout) {
            m_story_context.is_blackout = false;
            // Re-entry from blackout - create a fresh command
        }
    }

    DirectorCommand cmd;
    cmd.timestamp_ms = static_cast<int64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    cmd.scene_id = m_state.current_scene_id;
    cmd.narrative_beat = m_state.narrative_beat;
    cmd.pacing = m_state.pacing;
    cmd.shot_type = m_state.shot_type;
    cmd.motion = MakeMotion(m);
    cmd.composition = MakeComposition();
    cmd.mise_en_scene = MakeMiseEnScene();
    cmd.transition = NextTransition();
    cmd.effects = EffectsFor();
    cmd.glyph_map = m_state.glyph_map;
    cmd.audio_sync = { true, m.bpm, m.beat_phase, m.key, m.energy, m.song_section };
    
    if (SectionChanged(m_state.section)) {
        EnqueueMusic(m);
    }
    
    if (m_hasLLMCmd.load()) {
        cmd = m_latestLLMCmd;
        m_hasLLMCmd.store(false);
    } else {
        cmd.intent = { m_state.intent_mode, "stateful-intent" };
    }
    
    cmd.directives = GeneratorDirectives(m);
    cmd.priority = PriorityFor();
    cmd.ttl_ms = TTLFor();
    cmd.repeat = false;

    // Apply story constraints if enabled
    if (m_story_enabled && m_story_context.act) {
        cmd = ApplyStoryConstraints(cmd, m_story_context);
        
        // Check novelty if we have a detector
        if (m_novelty_detector) {
            VisualFingerprint fingerprint = CalculateVisualFingerprint(cmd);
            if (!m_novelty_detector->IsNovel(fingerprint)) {
                // Not novel enough - mutate or fallback
                cmd = Fallback(m);
                fingerprint = CalculateVisualFingerprint(cmd);
                
                // If still not novel, force some variation
                if (!m_novelty_detector->IsNovel(fingerprint)) {
                    cmd.mise_en_scene.brightness *= 0.8f + (rand() % 40) / 100.0f;
                    cmd.mise_en_scene.contrast *= 0.9f + (rand() % 20) / 100.0f;
                }
            }
            m_novelty_detector->AcceptFingerprint(fingerprint);
        }
    }

    SelectionInput si;
    si.phase = m_state.section.empty() ? std::string("live_performance") : m_state.section;
    si.mood = { m_state.emotional_tone };
    si.tempo_bpm = static_cast<int32>(m.bpm);
    si.energy_level_0_1 = m.energy;
    si.color_mode = "truecolor";
    si.is_dark_venue = true;
    si.is_test_sequence = false;
    if (m_palette_manager) {
        RichPalette rp = m_palette_manager->SelectPalette(si);
        m_state.palette = rp.name;
        Directive setp;
        setp.name = "SetPalette";
        setp.params["name"] = rp.name;
        auto hex = [&](uint32 c){ std::stringstream ss; ss<<"#"<<std::hex<<std::setfill('0')<<std::setw(8)<<c; return ss.str(); };
        setp.params["primary"] = hex(rp.primary);
        setp.params["secondary"] = hex(rp.secondary);
        setp.params["secondary2"] = hex(rp.secondary2);
        setp.params["accent"] = hex(rp.accent);
        setp.params["shadow"] = hex(rp.shadow);
        setp.params["highlight"] = hex(rp.highlight);
        cmd.directives.insert(cmd.directives.begin(), setp);
        cmd.mise_en_scene.palette = rp.name;
    }

    if (!Validate(cmd)) {
        return Fallback(m);
    }
    return cmd;
}

void AIDirector::SetRandomSeed(uint64 seed) { m_state.random_seed = seed; }
void AIDirector::SetSafetyEnabled(bool enabled) { m_state.nsfw_blocked = enabled; }
void AIDirector::SetIntentMode(const std::string& mode) { m_state.intent_mode = mode; }
DirectorState AIDirector::GetState() const { return m_state; }

NarrativeBeat AIDirector::MapSectionToBeat(const std::string& section) const {
    if (section == "chorus") return NarrativeBeat::Climax;
    if (section == "bridge") return NarrativeBeat::RisingAction;
    if (section == "verse") return NarrativeBeat::Setup;
    if (section == "breakdown") return NarrativeBeat::FallingAction;
    return NarrativeBeat::RisingAction;
}

Motion AIDirector::MakeMotion(const MusicData& m) const {
    Motion mot;
    mot.type = (m.dynamics > 0.5f) ? "pan" : "none";
    mot.axis = "x";
    mot.speed = std::clamp(m.energy * 1.5f, 0.0f, 2.0f);
    mot.amplitude = std::clamp(m.dynamics, 0.0f, 1.0f);
    mot.duration_ms = 2000;
    mot.easing = "smooth";
    return mot;
}

Composition AIDirector::MakeComposition() const {
    Composition c;
    c.focus_density = 0.3f;
    c.rule_of_thirds = true;
    c.center_bias = 0.1f;
    c.symmetry = "none";
    return c;
}

MiseEnScene AIDirector::MakeMiseEnScene() const {
    MiseEnScene m;
    m.palette = m_state.palette;
    m.brightness = (m_state.pacing == Pacing::FastCuts) ? 0.7f : 0.4f;
    m.contrast = (m_state.pacing == Pacing::FastCuts) ? 0.8f : 0.5f;
    m.set_dressing = (m_state.pacing == Pacing::LongTake) ? "soft_gradient_patterns" : "dynamic_patterns";
    return m;
}

Transition AIDirector::NextTransition() {
    Transition t;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTransition).count();
    if (elapsed < m_transitionMinIntervalMs) {
        t.type = "none";
        t.duration_ms = 0;
        return t;
    }
    t.type = (m_state.pacing == Pacing::FastCuts) ? "cut" : "dissolve";
    t.duration_ms = (m_state.pacing == Pacing::FastCuts) ? 100 : 600;
    m_lastTransition = now;
    return t;
}

std::vector<std::string> AIDirector::EffectsFor() const { return { "none" }; }

std::vector<Directive> AIDirector::GeneratorDirectives(const MusicData& m) const {
    std::vector<Directive> d;
    if (m.song_section == "chorus") {
        d.push_back({ "RenderText", { {"string","CHORUS"}, {"font","block"}, {"style","pulse"} } });
    } else if (m.song_section == "bridge") {
        d.push_back({ "GenerateMaze", { {"width","64"}, {"height","32"}, {"complexity","0.4"} } });
    } else if (m.song_section == "verse") {
        d.push_back({ "RenderText", { {"string","VERSE"}, {"font","thin"}, {"style","fade"} } });
    }
    return d;
}

int32 AIDirector::PriorityFor() const {
    float w = GetSceneWeight(m_state.current_scene_id);
    float e = std::clamp(m_state.energy_smoothed, 0.0f, 1.0f);
    float p = 3.0f + w * 4.0f + e * 3.0f;
    return static_cast<int32>(std::clamp(p, 1.0f, 10.0f));
}
int32 AIDirector::TTLFor() const {
    float w = GetSceneWeight(m_state.current_scene_id);
    float e = std::clamp(m_state.energy_smoothed, 0.0f, 1.0f);
    float base = 4000.0f;
    float scale = 1.0f - (e * 0.4f) - (std::min(w, 3.0f) * 0.1f);
    int32 ttl = static_cast<int32>(std::clamp(base * scale, 1500.0f, 6000.0f));
    return ttl;
}

bool AIDirector::Validate(const DirectorCommand& cmd) const {
    return !cmd.scene_id.empty();
}

DirectorCommand AIDirector::Fallback(const MusicData& m) const {
    DirectorCommand cmd;
    cmd.timestamp_ms = 0;
    cmd.scene_id = "SCN-FALLBACK";
    cmd.narrative_beat = NarrativeBeat::Setup;
    cmd.pacing = Pacing::LongTake;
    cmd.shot_type = ShotType::WideShot;
    cmd.motion = { "none", "x", 0.0f, 0.0f, 0, "linear" };
    cmd.composition = { 0.2f, true, 0.0f, "none" };
    cmd.mise_en_scene = { "ambient_color_palette", 0.4f, 0.4f, "soft_gradient_patterns" };
    cmd.transition = { "cut", 120 };
    cmd.effects = { "none" };
    cmd.glyph_map = "default_ascii";
    cmd.audio_sync = { true, m.bpm, m.beat_phase, m.key, m.energy, m.song_section };
    cmd.intent = { "abstract_flow", "fallback" };
    cmd.priority = 1;
    cmd.ttl_ms = 2000;
    cmd.repeat = false;
    return cmd;
}
void AIDirector::UpdateStateFromMusic(const MusicData& m) {
    m_state.energy_smoothed = 0.9f * m_state.energy_smoothed + 0.1f * m.energy;
    m_state.section = m.song_section;
    if (SectionChanged(m_state.section)) {
        m_state.narrative_beat = MapSectionToBeat(m_state.section);
        m_lastSection = m_state.section;
        if (m_state.section == "chorus") {
            m_state.palette = "bright_colors";
            m_state.glyph_map = "bold_ascii";
        } else if (m_state.section == "bridge") {
            m_state.palette = "transition_colors";
            m_state.glyph_map = "thin_sans_ascii";
        } else if (m_state.section == "verse") {
            m_state.palette = "ambient_color_palette";
            m_state.glyph_map = "default_ascii";
        }
    }
    if (m_state.energy_smoothed > m_energyThresholdHigh) {
        m_state.pacing = Pacing::FastCuts;
        m_state.shot_type = ShotType::CloseUp;
        m_state.emotional_tone = "ecstatic";
    } else if (m_state.energy_smoothed < m_energyThresholdLow) {
        m_state.pacing = Pacing::LongTake;
        m_state.shot_type = ShotType::WideShot;
        m_state.emotional_tone = "calm";
    } else {
        m_state.pacing = Pacing::Medium;
        m_state.shot_type = ShotType::TrackingShot;
        m_state.emotional_tone = "tense";
    }
    PushShot(m_state.shot_type);
}

bool AIDirector::SectionChanged(const std::string& s) const { return s != m_lastSection; }

void AIDirector::PushShot(ShotType s) {
    m_state.shot_history.push_back(s);
    if (m_state.shot_history.size() > 8) m_state.shot_history.pop_front();
}

std::string AIDirector::BuildPrompt(const MusicData& m) const {
    std::string p = "Music section: " + m.song_section + ", bpm=" + std::to_string(m.bpm) + ", energy=" + std::to_string(m.energy) + ". Describe a short abstract cinematic ASCII scene with keywords: wide_shot, close_up, fast_cuts, long_take, ambient_color_palette, high_contrast_palette.";
    return p;
}

std::string AIDirector::BuildLLMRequest(const MusicData& m) const {
    nlohmann::json j;
    nlohmann::json audio;
    audio["volume"] = (std::min)(1.0f, (std::max)(0.0f, m.energy));
    audio["bpm"] = static_cast<float>(m.bpm);
    audio["energy"] = m.energy;
    audio["spectral_centroid"] = 0.0;
    audio["is_beat"] = m.beat_phase < 0.1f || m.beat_phase > 0.9f;
    audio["beat_strength"] = (std::min)(1.0f, (std::max)(0.0f, m.dynamics));
    audio["genre"] = "unknown";
    j["audio"] = audio;
    j["section"] = m.song_section;
    ScenarioManager sm;
    sm.Initialize(m_config);
    std::string hint = sm.GetHint("track_001", m.song_section);
    j["story_hint"] = hint;
    nlohmann::json constraints;
    constraints["safe_for_epilepsy"] = m_config.llm.safeEpilepsy;
    constraints["max_flash_hz"] = 2.0;
    j["constraints"] = constraints;
    
    // Random psychedelic trigger (35-50% probability)
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    bool psychedelicMode = dis(gen) <= 42; // 42% probability for DMT/acid trip visuals
    
    std::string role;
    {
        std::ifstream f("config/llm_role_prompt.txt", std::ios::binary);
        if (f.is_open()) {
            std::ostringstream ss; ss << f.rdbuf(); role = ss.str();
        }
    }
    const char* ADDON =
        "SUPPLEMENTAL COLOR IMPROVISATION & MORPHING FREEDOM — NeonGlyph AGENT\n"
        "(Add-on only. Do NOT alter or overwrite the existing core prompt or color schema. Append this block as an extra behavior layer.)\n";
    if (!role.empty()) role += std::string("\n\n") + ADDON;
    
    // Add psychedelic enhancement to role prompt when triggered
    std::string enhancedRole = role;
    if (psychedelicMode) {
        enhancedRole += "\n\nPSYCHEDELIC MODE ACTIVATED: Generate abstract DMT acid trip visuals with fractal patterns, morphing sacred geometries, kaleidoscopic color explosions, melting reality effects, infinite recursion tunnels, entity contact visions, and reality-bending transformations. Embrace the chaos and beauty of altered consciousness states.";
    }
    
    std::string promptText = enhancedRole.empty() ? j.dump() : (enhancedRole + "\n\n" + j.dump());
    nlohmann::json req;
    req["model"] = m_config.llm.model;
    req["prompt"] = promptText;
    req["stream"] = false;
    return req.dump();
}

bool AIDirector::QueryLLM(const std::string& body, std::string& response) const {
#ifdef _WIN32
    // Enhanced HTTP client with security, timeouts, and retry logic
    const int MAX_RETRIES = 3;
    const int CONNECTION_TIMEOUT_MS = 5000;  // 5 seconds
    const int RESPONSE_TIMEOUT_MS = 10000;   // 10 seconds
    const int MAX_RESPONSE_SIZE = 10 * 1024 * 1024; // 10MB max response size
    
    std::string ep = m_config.llm.endpoint;
    if (ep.empty()) {
        std::cerr << "[AIDirector] Error: LLM endpoint not configured" << std::endl;
        return false;
    }
    
    // Validate endpoint format
    if (ep.find("://") == std::string::npos) {
        std::cerr << "[AIDirector] Error: Invalid endpoint format: " << ep << std::endl;
        return false;
    }
    
    // Security: Sanitize input payload
    if (body.size() > 1024 * 1024) { // 1MB max request size
        std::cerr << "[AIDirector] Error: Request body too large (" << body.size() << " bytes)" << std::endl;
        return false;
    }
    
    // Retry logic with exponential backoff
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        if (attempt > 0) {
            int backoff_ms = 1000 * (1 << (attempt - 1)); // Exponential backoff
            std::cout << "[AIDirector] Retry attempt " << attempt << " after " << backoff_ms << "ms backoff" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        }
        
        try {
            // Parse endpoint URL
            size_t schemeEnd = ep.find("://");
            std::string scheme = ep.substr(0, schemeEnd);
            std::string hostPort = schemeEnd != std::string::npos ? ep.substr(schemeEnd + 3) : ep;
            
            // Determine port based on scheme
            INTERNET_PORT port = (scheme == "https") ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
            
            // Extract host and port
            std::wstring host;
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                host = std::wstring(hostPort.begin(), hostPort.begin() + colonPos);
                port = static_cast<INTERNET_PORT>(SafeStoi(hostPort.substr(colonPos + 1)));
            } else {
                host = std::wstring(hostPort.begin(), hostPort.end());
            }
            
            // Security: Validate host
            if (host.empty() || host.length() > 255) {
                std::cerr << "[AIDirector] Error: Invalid host configuration" << std::endl;
                return false;
            }
            
            // Create HTTP session with enhanced security settings
            HINTERNET hSession = WinHttpOpen(L"NeonGlyph/1.0 (Secure HTTP Client)",
                                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS, 
                                             WINHTTP_FLAG_SECURE_DEFAULTS);
            if (!hSession) {
                std::cerr << "[AIDirector] Error: Failed to create HTTP session (attempt " << attempt + 1 << ")" << std::endl;
                continue;
            }
            
            // Set security options
            DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
            WinHttpSetOption(hSession, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
            
            // Set timeouts
            DWORD connectTimeout = CONNECTION_TIMEOUT_MS;
            DWORD responseTimeout = RESPONSE_TIMEOUT_MS;
            WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
            WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &responseTimeout, sizeof(responseTimeout));
            WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &responseTimeout, sizeof(responseTimeout));
            
            // Connect to server
            HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
            if (!hConnect) {
                std::cerr << "[AIDirector] Error: Failed to connect to " << std::string(host.begin(), host.end()) << ":" << port 
                         << " (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Create request with proper headers
            std::wstring endpoint = L"/api/generate";
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint.c_str(),
                                                    NULL, WINHTTP_NO_REFERER,
                                                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    (scheme == "https") ? WINHTTP_FLAG_SECURE : 0);
            if (!hRequest) {
                std::cerr << "[AIDirector] Error: Failed to create HTTP request (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Enhanced headers with security and performance settings
            std::wstring headers = L"Content-Type: application/json\r\n"
                                  L"Accept: application/json\r\n"
                                  L"User-Agent: NeonGlyph/1.0\r\n"
                                  L"Connection: close\r\n";
            
            // Send request
            BOOL sent = WinHttpSendRequest(hRequest,
                                           headers.c_str(), (DWORD)headers.size(),
                                           (LPVOID)body.data(), (DWORD)body.size(), 
                                           (DWORD)body.size(), 0);
            if (!sent) {
                std::cerr << "[AIDirector] Error: Failed to send HTTP request (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Receive response
            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                std::cerr << "[AIDirector] Error: Failed to receive HTTP response (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Read response with size limit and timeout protection
            std::string resp;
            DWORD dwSize = 0;
            DWORD totalBytesRead = 0;
            
            do {
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                    std::cerr << "[AIDirector] Error: Failed to query response data availability" << std::endl;
                    break;
                }
                if (dwSize == 0) break;
                
                // Security: Enforce response size limit
                if (totalBytesRead + dwSize > MAX_RESPONSE_SIZE) {
                    std::cerr << "[AIDirector] Error: Response too large (" << (totalBytesRead + dwSize) << " bytes)" << std::endl;
                    break;
                }
                
                std::string chunk;
                chunk.resize(dwSize);
                DWORD dwDownloaded = 0;
                if (!WinHttpReadData(hRequest, &chunk[0], dwSize, &dwDownloaded)) {
                    std::cerr << "[AIDirector] Error: Failed to read response data" << std::endl;
                    break;
                }
                resp.append(chunk.data(), dwDownloaded);
                totalBytesRead += dwDownloaded;
                
            } while (dwSize > 0);
            
            // Cleanup handles
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            
            // Validate response
            if (resp.empty()) {
                std::cerr << "[AIDirector] Error: Empty response received (attempt " << attempt + 1 << ")" << std::endl;
                continue;
            }
            
            // Parse JSON response with error handling
            try {
                auto j = nlohmann::json::parse(resp, nullptr, false);
                if (j.is_discarded()) {
                    std::cerr << "[AIDirector] Error: Invalid JSON response (attempt " << attempt + 1 << ")" << std::endl;
                    continue;
                }
                
                if (j.contains("response")) {
                    response = j["response"].get<std::string>();
                    std::cout << "[AIDirector] Successfully received LLM response (" << response.size() << " bytes)" << std::endl;
                    return true;
                } else {
                    std::cerr << "[AIDirector] Error: Missing 'response' field in JSON (attempt " << attempt + 1 << ")" << std::endl;
                    continue;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "[AIDirector] Error: JSON parsing failed: " << e.what() << " (attempt " << attempt + 1 << ")" << std::endl;
                continue;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[AIDirector] Exception during HTTP request: " << e.what() << " (attempt " << attempt + 1 << ")" << std::endl;
            continue;
        }
    }
    
    std::cerr << "[AIDirector] Error: All " << MAX_RETRIES << " retry attempts failed" << std::endl;
    return false;
#else
    std::cerr << "[AIDirector] Error: LLM queries not supported on this platform" << std::endl;
    return false;
#endif
}

void AIDirector::ApplyLLMResponse(const std::string& resp, DirectorCommand& cmd) {
    std::string r = resp;
    for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (r.find("fast_cuts") != std::string::npos) cmd.pacing = Pacing::FastCuts;
    if (r.find("long_take") != std::string::npos) cmd.pacing = Pacing::LongTake;
    if (r.find("wide_shot") != std::string::npos) cmd.shot_type = ShotType::WideShot;
    if (r.find("close_up") != std::string::npos) cmd.shot_type = ShotType::CloseUp;
    if (r.find("high_contrast_palette") != std::string::npos) cmd.mise_en_scene.palette = "high_contrast_palette";
    if (r.find("ambient_color_palette") != std::string::npos) cmd.mise_en_scene.palette = "ambient_color_palette";
}

void AIDirector::AIWorkerLoop() {
    while (!m_shouldStop.load()) {
        MusicData m;
        bool has = false;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_musicQueue.empty()) {
                m = m_musicQueue.front();
                m_musicQueue.pop();
                has = true;
            }
        }
        if (!has) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        auto t0 = std::chrono::steady_clock::now();
        std::string resp;
        std::string body = BuildLLMRequest(m);
        static NeonGlyph::Cache::SegmentedLRUCache<std::string, std::string> llmCache(2048, 32, std::chrono::seconds(30));
        static NeonGlyph::Cache::RedisClientPool redisPool;
        static bool redisInit = false;
        if (!redisInit) {
            const char* rep = std::getenv("NG_REDIS_ENDPOINT");
            redisPool.init(rep ? rep : "127.0.0.1:6379", 4);
            redisPool.subscribe("cache_invalidate", [&](const std::string& msg){ llmCache.invalidate(msg); });
            redisInit = true;
        }
        if (auto cached = llmCache.get(body)) { resp = *cached; }
        if (!resp.size() && redisInit) {
            auto rds = redisPool.get(body);
            if (rds.has_value()) { resp = *rds; }
        }
        if (!resp.size() && QueryLLM(body, resp)) {
            DirectorCommand out;
            out.timestamp_ms = static_cast<int64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
            out.scene_id = m_state.current_scene_id;
            out.narrative_beat = MapSectionToBeat(m.song_section);
            out.pacing = Pacing::Medium;
            out.shot_type = ShotType::WideShot;
            out.motion = MakeMotion(m);
            out.composition = MakeComposition();
            out.mise_en_scene = MakeMiseEnScene();
            out.transition = NextTransition();
            out.effects = EffectsFor();
            out.glyph_map = m_state.glyph_map;
            out.audio_sync = { true, m.bpm, m.beat_phase, m.key, m.energy, m.song_section };
            out.intent = { m_state.intent_mode, "llm-intent" };
            out.directives = GeneratorDirectives(m);
            out.priority = PriorityFor();
            out.ttl_ms = TTLFor();
            out.repeat = false;
            auto jr = nlohmann::json::parse(resp, nullptr, false);
            if (!jr.is_discarded()) {
                if (jr.contains("scene_type")) {
                    std::string st = jr["scene_type"].get<std::string>();
                    if (st == "chorus") out.narrative_beat = NarrativeBeat::Chorus;
                    else if (st == "verse") out.narrative_beat = NarrativeBeat::Verse;
                }
                if (jr.contains("intensity")) {
                    float inten = jr["intensity"].get<float>();
                    if (inten > 0.66f) out.pacing = Pacing::FastCuts;
                    else if (inten < 0.33f) out.pacing = Pacing::LongTake;
                    else out.pacing = Pacing::Medium;
                }
                if (jr.contains("color_palette")) {
                    out.mise_en_scene.palette = jr["color_palette"].get<std::string>();
                }
                if (jr.contains("transition_speed")) {
                    float ts = jr["transition_speed"].get<float>();
                    out.transition.type = ts > 0.5f ? "cut" : "dissolve";
                    out.transition.duration_ms = ts > 0.5f ? 100 : 600;
                }
                if (jr.contains("parameters")) {
                    auto arr = jr["parameters"];
                    if (arr.is_array() && arr.size() >= 3) {
                        out.motion.speed = std::clamp(arr[1].get<float>(), 0.0f, 2.0f);
                        out.motion.amplitude = std::clamp(arr[2].get<float>(), 0.0f, 1.0f);
                    }
                }
                if (jr.contains("duration_ms")) {
                    out.ttl_ms = jr["duration_ms"].get<int32>();
                }
            }
            auto t1 = std::chrono::steady_clock::now();
            llmCache.set(body, resp);
            if (redisInit) { redisPool.set(body, resp); }
            NeonGlyph::Cache::CacheMonitor::Update("ai_llm", llmCache.hits(), llmCache.misses());
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            if (ms <= m_config.llm.maxLatencyMs) {
                m_latestLLMCmd = out;
                m_hasLLMCmd.store(true);
                std::cout << "LLM latency " << ms << "ms" << std::endl;
            } else {
                std::cout << "LLM latency exceeded " << ms << "ms" << std::endl;
            }
        }
        else {
            std::cout << "LLM call failed" << std::endl;
        }
    }
}

void AIDirector::EnqueueMusic(const MusicData& m) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_musicQueue.push(m);
}

// Story integration methods
Result AIDirector::LoadStorySet(const std::string& json_content) {
    if (!m_story_clock) return Result::Error;
    return m_story_clock->LoadStorySet(json_content);
}

VisualFingerprint AIDirector::CalculateVisualFingerprint(const DirectorCommand& cmd) const {
    VisualFingerprint fingerprint;
    
    // Hash the key visual components
    std::hash<std::string> hasher;
    fingerprint.motif_hash = hasher(cmd.scene_id + cmd.intent.mode);
    fingerprint.palette_hash = hasher(cmd.mise_en_scene.palette);
    fingerprint.composition_hash = hasher(std::to_string(static_cast<int>(cmd.shot_type)) + 
                                       std::to_string(static_cast<int>(cmd.pacing)));
    
    // Calculate intensity based on various factors
    fingerprint.intensity = (cmd.motion.speed + cmd.motion.amplitude + 
                           (cmd.mise_en_scene.brightness + cmd.mise_en_scene.contrast) / 2.0f) / 3.0f;
    
    // Calculate complexity based on effects and motion
    fingerprint.complexity = static_cast<float>(cmd.effects.size()) * 0.1f + 
                           cmd.motion.speed * 0.3f + cmd.composition.focus_density * 0.6f;
    
    return fingerprint;
}

DirectorCommand AIDirector::ApplyStoryConstraints(const DirectorCommand& cmd, const StoryContext& story) {
    DirectorCommand constrained_cmd = cmd;
    
    if (!story.act || !story.scene) return constrained_cmd;
    
    // Apply scene-specific constraints
    const Scene* scene = story.scene;
    
    // Use allowed motifs if available
    if (!scene->allowed_motifs.empty()) {
        // For now, just use the first allowed motif as a hint
        constrained_cmd.intent.description += " (motif: " + scene->allowed_motifs[0] + ")";
    }
    
    // Apply palette constraints
    if (!scene->dominant_palettes.empty()) {
        constrained_cmd.mise_en_scene.palette = scene->dominant_palettes[0];
    }
    
    // Adjust transition speed based on scene
    constrained_cmd.transition.duration_ms = static_cast<int32>(
        constrained_cmd.transition.duration_ms * scene->transition_speed);
    
    // Apply intensity curve
    if (scene->intensity_curve.find("->") != std::string::npos) {
        float start_intensity = SafeStof(scene->intensity_curve.substr(0, scene->intensity_curve.find("->")));
        float end_intensity = SafeStof(scene->intensity_curve.substr(scene->intensity_curve.find("->") + 2));
        float target_intensity = start_intensity + (end_intensity - start_intensity) * story.scene_phase;
        
        // Adjust brightness and contrast to match target intensity
        constrained_cmd.mise_en_scene.brightness = target_intensity;
        constrained_cmd.mise_en_scene.contrast = 0.5f + target_intensity * 0.5f;
    }
    
    return constrained_cmd;
}

DirectorCommand AIDirector::CreateBlackoutCommand() const {
    DirectorCommand cmd;
    cmd.timestamp_ms = static_cast<int64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    cmd.scene_id = "BLACKOUT";
    cmd.narrative_beat = NarrativeBeat::FallingAction;
    cmd.pacing = Pacing::LongTake;
    cmd.shot_type = ShotType::WideShot;
    cmd.motion = { "none", "x", 0.0f, 0.0f, 1000, "smooth" };
    cmd.composition = { 0.0f, true, 0.0f, "none" };
    cmd.mise_en_scene = { "blackout", 0.0f, 0.0f, "none" };
    cmd.transition = { "fade_to_black", 2000 };
    cmd.effects = { "fade_out" };
    cmd.glyph_map = "empty";
    cmd.audio_sync = { false, 0, 0.0f, "", 0.0f, "" };
    cmd.intent = { "blackout", "audio_silence_blackout" };
    cmd.priority = 10;
    cmd.ttl_ms = 5000;
    cmd.repeat = false;
    return cmd;
}

Result AIDirector::LoadSceneWeights(const std::string& path) {
    try {
        std::ifstream in(path);
        if (!in.is_open()) return Result::FileNotFound;
        nlohmann::json j;
        in >> j;
        m_sceneWeights.clear();
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                float v = 1.0f;
                if (it.value().is_number()) v = it.value().get<float>();
                m_sceneWeights[it.key()] = std::clamp(v, 0.1f, 10.0f);
            }
        }
        return Result::Success;
    } catch (...) {
        return Result::Error;
    }
}

float AIDirector::GetSceneWeight(const std::string& scene_id) const {
    auto it = m_sceneWeights.find(scene_id);
    if (it != m_sceneWeights.end()) return it->second;
    return 1.0f;
}

bool AIDirector::ShouldInjectEasterEgg() const {
    auto now = std::chrono::steady_clock::now();
    int64 ms = static_cast<int64>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    if (m_lastEasterEggMs == 0) return false;
    if (ms - m_lastEasterEggMs < 60000) return false;
    if (m_state.section == "bridge" || m_state.section == "breakdown") return true;
    return false;
}

} // namespace NeonGlyph
