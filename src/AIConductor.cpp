#include "AIConductor.h"
#include "DirectorTypes.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>
#include <regex>
#ifdef _WIN32
#include <windows.h>
#endif

namespace NeonGlyph {

// Musical key mapping
const std::vector<std::string> MUSICAL_KEYS = {
    "C Major", "C# Major", "D Major", "D# Major", "E Major", "F Major",
    "F# Major", "G Major", "G# Major", "A Major", "A# Major", "B Major",
    "C Minor", "C# Minor", "D Minor", "D# Minor", "E Minor", "F Minor",
    "F# Minor", "G Minor", "G# Minor", "A Minor", "A# Minor", "B Minor"
};

// Genre classification mapping
const std::vector<std::string> MUSIC_GENRES = {
    "Techno", "House", "Trance", "Drum and Bass", "Dubstep", "Trap",
    "Hip Hop", "Rap", "R&B", "Pop", "Rock", "Metal", "Punk", "Jazz",
    "Blues", "Classical", "Ambient", "Experimental", "Industrial", "EBM"
};

// Mood classification mapping
const std::vector<std::string> MUSIC_MOODS = {
    "Aggressive", "Energetic", "Happy", "Uplifting", "Dark", "Melancholic",
    "Mysterious", "Dreamy", "Relaxed", "Chill", "Intense", "Dramatic",
    "Playful", "Romantic", "Nostalgic", "Hopeful", "Anxious", "Powerful"
};

AIConductor::AIConductor() 
    : m_currentGenre("Unknown")
    , m_currentMood("Neutral")
    , m_energyLevel(0.5f)
    , m_danceability(0.5f)
    , m_valence(0.5f)
    , m_arousal(0.5f)
    , m_eventTriggered(false)
    , m_inferenceTimeMs(0.0f)
    , m_modelMemoryUsageMB(0)
    , m_shouldStop(false)
    , m_onnxEnv(nullptr)
    , m_sessionOptions(nullptr)
    , m_genreClassifier(nullptr)
    , m_moodPredictor(nullptr)
    , m_sceneGenerator(nullptr)
    , m_memoryInfo(nullptr) {
}

AIConductor::~AIConductor() {
    Shutdown();
}

Result AIConductor::Initialize(const Config& config) {
    m_config = config;
    
    if (!config.ai.enabled) {
        return Result::Success;
    }
    
#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
    Result mr = LoadModels();
    if (mr != Result::Success) {
        return mr;
    }
#endif
        
    // Load safety data
    LoadNSFWKeywords();
    
    // Initialize analysis thread
    m_analysisThread = std::thread(&AIConductor::AnalysisThreadFunc, this);
    std::cout << "[AIConductor] Analysis thread started" << std::endl;
#ifdef _WIN32
    HANDLE h = (HANDLE)m_analysisThread.native_handle();
    SetThreadPriority(h, THREAD_PRIORITY_ABOVE_NORMAL);
#endif
    
    return Result::Success;
}

void AIConductor::Shutdown() {
    m_shouldStop = true;
    
    // Stop analysis thread
    if (m_analysisThread.joinable()) {
        m_queueCondition.notify_all();
        m_analysisThread.join();
    }
    
#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
    // Cleanup models - these would be smart pointers in real ONNX implementation
    // For now, we just set them to nullptr since they're opaque pointers
    m_sceneGenerator = nullptr;
    m_moodPredictor = nullptr;
    m_genreClassifier = nullptr;
    
    // Cleanup environment
    m_onnxEnv = nullptr;
    m_sessionOptions = nullptr;
    m_memoryInfo = nullptr;
#endif
}

#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
Result AIConductor::LoadModels() {
    try {
        // Initialize genre classifier
        Result result = InitializeGenreClassifier();
        if (result != Result::Success) {
            return result;
        }
        
        // Initialize mood predictor
        result = InitializeMoodPredictor();
        if (result != Result::Success) {
            return result;
        }
        
        // Initialize scene generator
        result = InitializeSceneGenerator();
        if (result != Result::Success) {
            return result;
        }
        
        return Result::Success;
        
    } catch (const std::exception& e) {
        std::cerr << "Model loading failed: " << e.what() << std::endl;
        return Result::FileNotFound;
    } catch (...) {
        std::cerr << "Model loading failed with unknown exception" << std::endl;
        return Result::FileNotFound;
    }
}
#endif

#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
Result AIConductor::InitializeGenreClassifier() {
    m_genreClassifier = nullptr;
    
    // Set up input/output names
    m_genreInputNames = {"input_spectrum"};
    m_genreOutputNames = {"output_genre"};
    
    return Result::Success;
}
#endif

#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
Result AIConductor::InitializeMoodPredictor() {
    m_moodPredictor = nullptr;
    
    // Set up input/output names
    m_moodInputNames = {"input_features"};
    m_moodOutputNames = {"output_mood"};
    
    return Result::Success;
}
#endif

#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
Result AIConductor::InitializeSceneGenerator() {
    m_sceneGenerator = nullptr;
    
    // Set up input/output names
    m_sceneInputNames = {"input_context"};
    m_sceneOutputNames = {"output_scene"};
    
    return Result::Success;
}
#endif

Result AIConductor::AnalyzeAudioFrame(const AudioFrame& frame) {
    // Queue frame for analysis
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_audioQueue.push(frame);
    }
    m_queueCondition.notify_one();
    
    return Result::Success;
}

Result AIConductor::AnalyzeSpectrum(const Spectrum& spectrum) {
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Extract spectral features
        std::vector<float32> features = ExtractSpectralFeatures(spectrum);
        
        // Analyze genre
        Result result = AnalyzeGenre(spectrum);
        if (result != Result::Success) {
            return result;
        }
        
        // Analyze danceability
        result = AnalyzeDanceability(spectrum);
        if (result != Result::Success) {
            return result;
        }
        
        // Update performance metrics
        auto end = std::chrono::high_resolution_clock::now();
        m_inferenceTimeMs = std::chrono::duration<float32, std::milli>(end - start).count();
        
        return Result::Success;
        
    } catch (const std::exception& e) {
        std::cerr << "Spectrum analysis failed: " << e.what() << std::endl;
        return Result::Error;
    }
}

Result AIConductor::AnalyzeGenre(const Spectrum& spectrum) {
    std::vector<float32> f = ExtractSpectralFeatures(spectrum);
    m_currentGenre = ClassifyGenre(f);
    m_genreHistory.push_back(m_currentGenre);
    if (m_genreHistory.size() > 10) {
        m_genreHistory.erase(m_genreHistory.begin());
    }
    return Result::Success;
}

Result AIConductor::AnalyzeMood(const AudioFrame& frame, const Spectrum& spectrum) {
    float32 energy = 0.0f;
    for (uint32_t i = 0; i < frame.frameCount * frame.channelCount; i++) {
        energy += frame.data[i] * frame.data[i];
    }
    energy = sqrt(energy / (frame.frameCount * frame.channelCount));
    std::vector<float32> f = ExtractSpectralFeatures(spectrum);
    std::vector<float32> combined;
    combined.push_back(energy);
    combined.insert(combined.end(), f.begin(), f.end());
    m_currentMood = PredictMood(combined);
    m_moodHistory.push_back(m_currentMood);
    if (m_moodHistory.size() > 10) {
        m_moodHistory.erase(m_moodHistory.begin());
    }
    return Result::Success;
}

Result AIConductor::AnalyzeEnergy(const AudioFrame& frame) {
    float32 energy = 0.0f;
    for (uint32_t i = 0; i < frame.frameCount * frame.channelCount; i++) {
        energy += frame.data[i] * frame.data[i];
    }
    m_energyLevel = sqrt(energy / (frame.frameCount * frame.channelCount));
    
    m_energyHistory.push_back(m_energyLevel);
    if (m_energyHistory.size() > 100) {
        m_energyHistory.erase(m_energyHistory.begin());
    }
    
    return Result::Success;
}

Result AIConductor::AnalyzeDanceability(const Spectrum& spectrum) {
    // Simple danceability based on beat regularity and spectral flux
    float32 spectralFlux = 0.0f;
    
    if (m_spectralCentroidHistory.size() > 1) {
        for (size_t i = 1; i < m_spectralCentroidHistory.size(); i++) {
            spectralFlux += std::abs(m_spectralCentroidHistory[i] - m_spectralCentroidHistory[i-1]);
        }
        spectralFlux /= (m_spectralCentroidHistory.size() - 1);
    }
    
    // Normalize and combine with energy
    m_danceability = std::min(1.0f, (m_energyLevel * 0.7f + spectralFlux * 0.3f));
    
    m_spectralCentroidHistory.push_back(spectralFlux);
    if (m_spectralCentroidHistory.size() > 50) {
        m_spectralCentroidHistory.erase(m_spectralCentroidHistory.begin());
    }
    
    return Result::Success;
}

std::string AIConductor::GenerateSceneDescription() {
    std::stringstream description;
    
    description << "Current musical context: " << m_currentGenre << " genre with " 
                << m_currentMood << " mood. Energy level: " 
                << static_cast<int>(m_energyLevel * 100) << "%. ";
    
    // Add contextual information
    {
        std::lock_guard<std::mutex> lock(m_contextMutex);
        for (const auto& [key, value] : m_context) {
            description << key << ": " << value << ". ";
        }
    }
    
    // Generate scene suggestion
    description << "Recommended visual style: " << GenerateColorPaletteFromMood() 
                << " with " << GenerateCharsetFromGenre() << " characters.";
    
    return description.str();
}

std::string AIConductor::GenerateColorPaletteSuggestion() {
    return GenerateColorPaletteFromMood();
}

std::string AIConductor::GenerateCharacterSetSuggestion() {
    return GenerateCharsetFromGenre();
}

void AIConductor::UpdateContext(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    m_context[key] = value;
}

std::string AIConductor::GetContext(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    auto it = m_context.find(key);
    return it != m_context.end() ? it->second : "";
}

bool AIConductor::IsContentSafe(const std::string& content) {
    return !ContainsNSFWContent(content);
}

std::string AIConductor::FilterNSFWContent(const std::string& content) {
    return CleanNSFWContent(content);
}

std::vector<float32> AIConductor::ExtractAudioFeatures(const AudioFrame& frame) {
    std::vector<float32> features;
    
    // Extract basic features
    float32 energy = 0.0f;
    float32 zeroCrossingRate = 0.0f;
    float32 maxAmplitude = 0.0f;
    
    for (uint32_t i = 0; i < frame.frameCount * frame.channelCount; i++) {
        energy += frame.data[i] * frame.data[i];
        maxAmplitude = std::max(maxAmplitude, std::abs(frame.data[i]));
        
        if (i > 0 && (frame.data[i-1] < 0.0f) != (frame.data[i] < 0.0f)) {
            zeroCrossingRate++;
        }
    }
    
    energy = sqrt(energy / (frame.frameCount * frame.channelCount));
    zeroCrossingRate /= (frame.frameCount * frame.channelCount);
    
    features.push_back(energy);
    features.push_back(zeroCrossingRate);
    features.push_back(maxAmplitude);
    
    return features;
}

std::vector<float32> AIConductor::ExtractSpectralFeatures(const Spectrum& spectrum) {
    std::vector<float32> features;
    
    // Spectral centroid
    float32 spectralCentroid = 0.0f;
    float32 totalMagnitude = 0.0f;
    
    for (uint32_t i = 0; i < spectrum.binCount; i++) {
        float32 frequency = i * spectrum.frequencyResolution;
        spectralCentroid += frequency * spectrum.magnitudes[i];
        totalMagnitude += spectrum.magnitudes[i];
    }
    
    if (totalMagnitude > 0.0f) {
        spectralCentroid /= totalMagnitude;
    }
    
    // Spectral rolloff
    float32 rolloffThreshold = 0.85f * totalMagnitude;
    float32 cumulativeMagnitude = 0.0f;
    float32 spectralRolloff = 0.0f;
    
    for (uint32_t i = 0; i < spectrum.binCount; i++) {
        cumulativeMagnitude += spectrum.magnitudes[i];
        if (cumulativeMagnitude >= rolloffThreshold) {
            spectralRolloff = i * spectrum.frequencyResolution;
            break;
        }
    }
    
    // Spectral flux
    float32 spectralFlux = 0.0f;
    if (!m_spectralCentroidHistory.empty()) {
        for (uint32_t i = 0; i < spectrum.binCount && i < m_spectralCentroidHistory.size(); i++) {
            spectralFlux += std::max(0.0f, spectrum.magnitudes[i] - m_spectralCentroidHistory[i]);
        }
    }
    
    features.push_back(spectralCentroid);
    features.push_back(spectralRolloff);
    features.push_back(spectralFlux);
    
    return features;
}

std::string AIConductor::ClassifyGenre(const std::vector<float32>& features) {
    if (features.size() < 3) return "Ambient";
    float32 centroid = features[0];
    float32 rolloff = features[1];
    float32 flux = features[2];
    if (centroid < 300.0f && flux < 0.05f) return "Ambient";
    if (centroid < 600.0f && flux < 0.1f) return "House";
    if (centroid < 900.0f && flux < 0.15f) return "Trance";
    if (centroid < 1500.0f && flux >= 0.15f) return "Drum and Bass";
    if (rolloff > 4000.0f && flux >= 0.1f) return "Techno";
    return "Experimental";
}

std::string AIConductor::PredictMood(const std::vector<float32>& features) {
    if (features.empty()) return "Neutral";
    float32 energy = features[0];
    float32 centroid = features.size() > 1 ? features[1] : 0.0f;
    float32 flux = features.size() > 3 ? features[3] : 0.0f;
    if (energy > 0.6f && centroid > 1000.0f) return "Energetic";
    if (energy < 0.25f && centroid < 600.0f) return "Relaxed";
    if (flux > 0.2f && centroid < 800.0f) return "Dark";
    return "Neutral";
}

std::string AIConductor::GeneratePromptFromAudioState() {
    std::stringstream prompt;
    
    prompt << "Create a " << m_currentGenre << " style visual with " 
           << m_currentMood << " mood. Energy: " << static_cast<int>(m_energyLevel * 100) << "%. ";
    
    if (m_energyLevel > 0.7f) {
        prompt << "Fast, intense, high contrast. ";
    } else if (m_energyLevel < 0.3f) {
        prompt << "Slow, calm, low contrast. ";
    }
    
    return prompt.str();
}

std::string AIConductor::GenerateColorPaletteFromMood() {
    if (m_currentMood == "Dark" || m_currentMood == "Melancholic") {
        return "cyberpunk"; // Green/black palette
    } else if (m_currentMood == "Energetic" || m_currentMood == "Aggressive") {
        return "vaporwave"; // Pink/cyan palette
    } else if (m_currentMood == "Relaxed" || m_currentMood == "Chill") {
        return "matrix"; // Green gradient palette
    } else {
        return "noir"; // Grayscale palette
    }
}

std::string AIConductor::GenerateCharsetFromGenre() {
    if (m_currentGenre == "Techno" || m_currentGenre == "Industrial") {
        return "@%#*+=-:. "; // High contrast characters
    } else if (m_currentGenre == "Ambient" || m_currentGenre == "Classical") {
        return ".:-=+*#%@"; // Smooth gradient characters
    } else {
        return "@%#*+=-:. "; // Default charset
    }
}

void AIConductor::LoadNSFWKeywords() {
    m_nsfwKeywords = {
        "explicit", "adult", "nsfw", "mature", "restricted", "sensitive",
        "inappropriate", "offensive", "violence", "gore", "nudity"
    };
    
    m_safetyWhitelist = {
        "safe", "clean", "family-friendly", "appropriate", "professional"
    };
}

bool AIConductor::ContainsNSFWContent(const std::string& text) {
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    for (const auto& keyword : m_nsfwKeywords) {
        if (lowerText.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

std::string AIConductor::CleanNSFWContent(const std::string& text) {
    std::string cleaned = text;
    
    for (const auto& keyword : m_nsfwKeywords) {
        std::regex pattern("\\b" + keyword + "\\b", std::regex_constants::icase);
        cleaned = std::regex_replace(cleaned, pattern, "[filtered]");
    }
    
    return cleaned;
}

void AIConductor::DetectMusicalEvents() {
    // Detect various musical events
    bool beatDrop = DetectBeatDrop();
    bool buildUp = DetectBuildUp();
    bool breakDown = DetectBreakDown();
    bool genreChange = DetectGenreChange();
    bool moodShift = DetectMoodShift();
    
    // Trigger events
    if (beatDrop && !m_eventTriggered) {
        m_eventTriggered = true;
        m_eventType = "beat_drop";
        m_lastEventTime = std::chrono::steady_clock::now();
    } else if (buildUp && !m_eventTriggered) {
        m_eventTriggered = true;
        m_eventType = "build_up";
        m_lastEventTime = std::chrono::steady_clock::now();
    }
}

bool AIConductor::DetectBeatDrop() {
    if (m_energyHistory.size() < 10) return false;
    
    // Simple beat drop detection: sudden energy increase
    float32 recentAvg = 0.0f;
    for (size_t i = m_energyHistory.size() - 5; i < m_energyHistory.size(); i++) {
        recentAvg += m_energyHistory[i];
    }
    recentAvg /= 5.0f;
    
    float32 previousAvg = 0.0f;
    for (size_t i = m_energyHistory.size() - 10; i < m_energyHistory.size() - 5; i++) {
        previousAvg += m_energyHistory[i];
    }
    previousAvg /= 5.0f;
    
    return recentAvg > previousAvg * 1.5f;
}

bool AIConductor::DetectBuildUp() {
    if (m_energyHistory.size() < 20) return false;
    
    // Simple build-up detection: gradual energy increase
    float32 recentTrend = 0.0f;
    for (size_t i = m_energyHistory.size() - 10; i < m_energyHistory.size() - 1; i++) {
        recentTrend += m_energyHistory[i + 1] - m_energyHistory[i];
    }
    
    return recentTrend > 0.1f;
}

bool AIConductor::DetectBreakDown() {
    if (m_energyHistory.size() < 10) return false;
    
    // Simple break-down detection: sudden energy decrease
    float32 recentAvg = 0.0f;
    for (size_t i = m_energyHistory.size() - 5; i < m_energyHistory.size(); i++) {
        recentAvg += m_energyHistory[i];
    }
    recentAvg /= 5.0f;
    
    float32 previousAvg = 0.0f;
    for (size_t i = m_energyHistory.size() - 10; i < m_energyHistory.size() - 5; i++) {
        previousAvg += m_energyHistory[i];
    }
    previousAvg /= 5.0f;
    
    return recentAvg < previousAvg * 0.5f;
}

bool AIConductor::DetectGenreChange() {
    if (m_genreHistory.size() < 5) return false;
    
    // Check if recent genres are different from older ones
    std::string recentGenre = m_genreHistory.back();
    std::string oldGenre = m_genreHistory[m_genreHistory.size() - 5];
    
    return recentGenre != oldGenre;
}

bool AIConductor::DetectMoodShift() {
    if (m_moodHistory.size() < 5) return false;
    
    // Check if recent moods are different from older ones
    std::string recentMood = m_moodHistory.back();
    std::string oldMood = m_moodHistory[m_moodHistory.size() - 5];
    
    return recentMood != oldMood;
}

void AIConductor::AnalysisThreadFunc() {
    while (!m_shouldStop) {
        ProcessAudioQueue();
        DetectMusicalEvents();
        UpdatePerformanceMetrics();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void AIConductor::ProcessAudioQueue() {
    std::queue<AudioFrame> localQueue;
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        localQueue = std::move(m_audioQueue);
    }
    
    while (!localQueue.empty() && !m_shouldStop) {
        AudioFrame frame = localQueue.front();
        localQueue.pop();
        
        AnalyzeEnergy(frame);
        AnalyzeMood(frame, Spectrum()); // Would need actual spectrum
    }
}

void AIConductor::UpdatePerformanceMetrics() {
    size_t bytes = 0;
    bytes += m_context.size() * sizeof(std::pair<std::string, std::string>);
    bytes += m_genreHistory.size() * sizeof(std::string);
    bytes += m_moodHistory.size() * sizeof(std::string);
    bytes += m_energyHistory.size() * sizeof(float32);
    bytes += m_spectralCentroidHistory.size() * sizeof(float32);
    m_modelMemoryUsageMB = static_cast<uint32_t>(bytes / (1024u * 1024u));
    
    
    if (m_eventTriggered) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastEventTime);
        if (elapsed.count() > 2) { // 2 second timeout
            m_eventTriggered = false;
            m_eventType = "";
        }
    }
}

void AIConductor::LogInferenceTime(float32 timeMs) {
    m_inferenceTimeMs = timeMs;
}

void AIConductor::AcknowledgeEvent() {
    m_eventTriggered = false;
    m_eventType = "";
}

void AIConductor::ApplyDirective(const DirectorDirective& directive) {
    std::cout << "[AIConductor] Applying directive: " << directive.sceneIdentity << std::endl;
    
    // Apply scene identity changes
    if (!directive.sceneIdentity.empty() && directive.sceneIdentity != "maintain") {
        std::cout << "[AIConductor] Scene identity changed to: " << directive.sceneIdentity << std::endl;
        // Store in context for scene generation
        UpdateContext("director_scene_identity", directive.sceneIdentity);
    }
    
    // Apply motion preset changes
    if (!directive.motionPreset.empty() && directive.motionPreset != "maintain") {
        std::cout << "[AIConductor] Motion preset changed to: " << directive.motionPreset << std::endl;
        UpdateContext("director_motion_preset", directive.motionPreset);
    }
    
    // Apply palette shifts
    if (!directive.paletteShifts.empty()) {
        std::cout << "[AIConductor] Palette shifts applied: " << directive.paletteShifts.size() << " shifts" << std::endl;
        for (const auto& shift : directive.paletteShifts) {
            std::cout << "  - " << shift << std::endl;
        }
        UpdateContext("director_palette_shifts", "active");
    }
    
    // Apply intensity changes
    if (directive.intensity >= 0.0f && directive.intensity <= 1.0f) {
        std::cout << "[AIConductor] Intensity set to: " << directive.intensity << std::endl;
        // Scale energy level based on intensity
        m_energyLevel = directive.intensity;
        UpdateContext("director_intensity", std::to_string(directive.intensity));
    }
    
    // Apply mood changes
    if (!directive.mood.empty() && directive.mood != "maintain") {
        std::cout << "[AIConductor] Mood overridden to: " << directive.mood << std::endl;
        m_currentMood = directive.mood;
        UpdateContext("director_mood", directive.mood);
    }
    
    // Process micro-events
    for (const auto& event : directive.microEvents) {
        std::cout << "[AIConductor] Triggering micro-event: " << event.type << std::endl;
        if (event.type == "flash") {
            // Trigger visual flash event
            m_eventTriggered = true;
            m_eventType = "director_flash";
        } else if (event.type == "shake") {
            // Trigger screen shake event
            m_eventTriggered = true;
            m_eventType = "director_shake";
        } else if (event.type == "glitch") {
            // Trigger glitch effect
            m_eventTriggered = true;
            m_eventType = "director_glitch";
        } else if (event.type == "color_burst") {
            // Trigger color burst
            m_eventTriggered = true;
            m_eventType = "director_color_burst";
        }
    }
    
    std::cout << "[AIConductor] Directive application complete" << std::endl;
}

} // namespace NeonGlyph