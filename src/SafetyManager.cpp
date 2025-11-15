#include "SafetyManager.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <map>
#include <chrono>
#include <thread>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace NeonGlyph {

// SafetyManager Implementation
SafetyManager::SafetyManager() 
    : m_epilepsyModeEnabled(true)
    , m_contentFilteringEnabled(true)
    , m_crashRecoveryEnabled(true)
    , m_luminanceThreshold(0.3f)
    , m_filterSensitivity(0.5f)
    , m_maxFrameTimeMs(16.67f) // 60 FPS
    , m_maxMemoryMB(2048)
    , m_crashDetected(false)
    , m_crashCount(0)
    , m_violationCount(0)
    , m_watchdogShouldStop(false) {
    
    InitializeNSFWKeywords();
    InitializeSafetyWhitelist();
}

SafetyManager::~SafetyManager() {
    Shutdown();
}

Result SafetyManager::Initialize(const Config& config) {
    m_config = config;
    
    // Load configuration
    m_epilepsyModeEnabled = config.safety.photosensitiveMode;
    m_contentFilteringEnabled = config.safety.nsfwFilter;
    m_crashRecoveryEnabled = config.safety.crashRecovery;
    m_luminanceThreshold = config.safety.luminanceThreshold;
    
    // Initialize subsystems
    if (m_epilepsyModeEnabled) {
        // Epilepsy protection is handled inline
    }
    
    if (m_crashRecoveryEnabled) {
        Result result = EnableCrashRecovery();
        if (result != Result::Success) {
            return result;
        }
    }
    
    return Result::Success;
}

void SafetyManager::Shutdown() {
    if (m_watchdogThread.joinable()) {
        m_watchdogShouldStop = true;
        m_watchdogThread.join();
    }
}

bool SafetyManager::ShouldClampBrightness(float32 currentLuminance, float32 previousLuminance) {
    if (!m_epilepsyModeEnabled) {
        return false;
    }
    
    float32 delta = CalculateLuminanceDelta(currentLuminance, previousLuminance);
    return IsLuminanceChangeDangerous(delta);
}

float32 SafetyManager::CalculateLuminanceDelta(float32 current, float32 previous) {
    return std::abs(current - previous);
}

float32 SafetyManager::GetSafeBrightnessLevel(float32 targetBrightness) {
    if (!m_epilepsyModeEnabled) {
        return targetBrightness;
    }
    
    // Apply gradual smoothing to prevent sudden changes
    if (!m_luminanceHistory.empty()) {
        float32 averageLuminance = CalculateRollingAverageLuminanceDelta();
        if (averageLuminance > m_luminanceThreshold) {
            // Gradually approach target brightness
            float32 currentAverage = m_luminanceHistory.back();
            float32 maxChange = m_luminanceThreshold * 0.5f; // Allow 50% of threshold per frame
            
            if (targetBrightness > currentAverage) {
                return (std::min)(targetBrightness, currentAverage + maxChange);
            } else {
                return (std::max)(targetBrightness, currentAverage - maxChange);
            }
        }
    }
    
    return targetBrightness;
}

bool SafetyManager::IsContentSafe(const std::string& content) {
    if (!m_contentFilteringEnabled) {
        return true;
    }
    
    return !ContainsNSFWContent(content);
}

std::string SafetyManager::FilterContent(const std::string& content) {
    if (!m_contentFilteringEnabled) {
        return content;
    }
    
    return CleanNSFWContent(content);
}

void SafetyManager::AddNSFWKeyword(const std::string& keyword) {
    m_nsfwKeywords.push_back(keyword);
}

void SafetyManager::RemoveNSFWKeyword(const std::string& keyword) {
    m_nsfwKeywords.erase(std::remove(m_nsfwKeywords.begin(), m_nsfwKeywords.end(), keyword), m_nsfwKeywords.end());
}

Result SafetyManager::EnableCrashRecovery() {
    // Start watchdog thread
    m_watchdogThread = std::thread(&SafetyManager::WatchdogThreadFunc, this);
    
    std::cout << "Crash recovery enabled with watchdog thread" << std::endl;
    return Result::Success;
}

void SafetyManager::OnCrashDetected() {
    m_crashDetected = true;
    m_lastCrashTime = std::chrono::steady_clock::now();
    m_crashCount++;
    
    LogSafetyViolation("Application crash detected");
    
    std::cout << "Crash detected! Count: " << m_crashCount << std::endl;
}

bool SafetyManager::ShouldRestartApplication() {
    if (!m_crashRecoveryEnabled || !m_crashDetected) {
        return false;
    }
    
    // Check if enough time has passed since last crash
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastCrashTime);
    
    return elapsed.count() >= 5; // Wait 5 seconds before restart
}

bool SafetyManager::IsPerformanceAcceptable(const PerformanceMetrics& metrics) {
    // Check frame time
    if (metrics.frameTimeMs > m_maxFrameTimeMs) {
        HandleSafetyViolation("Performance", "Frame time exceeded threshold: " + std::to_string(metrics.frameTimeMs) + "ms");
        return false;
    }
    
    // Check memory usage
    if (metrics.memoryUsageMB > m_maxMemoryMB) {
        HandleSafetyViolation("Memory", "Memory usage exceeded threshold: " + std::to_string(metrics.memoryUsageMB) + "MB");
        return false;
    }
    
    return true;
}

void SafetyManager::LogSafetyViolation(const std::string& violation) {
    HandleSafetyViolation("General", violation);
}

std::vector<std::string> SafetyManager::GetRecentViolations() const {
    std::lock_guard<std::mutex> lock(m_violationMutex);
    
    std::vector<std::string> violations;
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& [time, violation] : m_violations) {
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - time);
        if (elapsed.count() < 60) { // Keep violations from last hour
            violations.push_back(violation);
        }
    }
    
    return violations;
}

void SafetyManager::SetEpilepsyThreshold(float32 threshold) {
    m_luminanceThreshold = threshold;
}

void SafetyManager::SetPerformanceThresholds(float32 maxFrameTime, uint32_t maxMemoryMB) {
    m_maxFrameTimeMs = maxFrameTime;
    m_maxMemoryMB = maxMemoryMB;
}

void SafetyManager::SetContentFilterSensitivity(float32 sensitivity) {
    m_filterSensitivity = sensitivity;
}

void SafetyManager::InitializeNSFWKeywords() {
    m_nsfwKeywords = {
        "explicit", "adult", "nsfw", "mature", "restricted", "sensitive",
        "inappropriate", "offensive", "violence", "gore", "nudity", "sexual",
        "porn", "xxx", "hentai", "erotic", "provocative", "suggestive"
    };
}

void SafetyManager::InitializeSafetyWhitelist() {
    m_safetyWhitelist = {
        "safe", "clean", "family-friendly", "appropriate", "professional",
        "artistic", "educational", "documentary", "news", "information"
    };
}

bool SafetyManager::ContainsNSFWContent(const std::string& content) {
    std::string lowerContent = content;
    std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
    
    // Check against keywords
    for (const auto& keyword : m_nsfwKeywords) {
        if (lowerContent.find(keyword) != std::string::npos) {
            // Check if whitelisted
            bool whitelisted = false;
            for (const auto& whitelistWord : m_safetyWhitelist) {
                if (lowerContent.find(whitelistWord) != std::string::npos) {
                    whitelisted = true;
                    break;
                }
            }
            
            if (!whitelisted) {
                return true;
            }
        }
    }
    
    return false;
}

std::string SafetyManager::CleanNSFWContent(const std::string& content) {
    std::string cleaned = content;
    
    for (const auto& keyword : m_nsfwKeywords) {
        std::regex pattern("\\b" + keyword + "\\b", std::regex_constants::icase);
        cleaned = std::regex_replace(cleaned, pattern, "[filtered]");
    }
    
    return cleaned;
}

float32 SafetyManager::CalculateRollingAverageLuminanceDelta() const {
    if (m_luminanceHistory.size() < 2) {
        return 0.0f;
    }
    
    float32 totalDelta = 0.0f;
    for (size_t i = 1; i < m_luminanceHistory.size(); i++) {
        totalDelta += std::abs(m_luminanceHistory[i] - m_luminanceHistory[i-1]);
    }
    
    return totalDelta / (m_luminanceHistory.size() - 1);
}

bool SafetyManager::IsLuminanceChangeDangerous(float32 delta) const {
    return delta > m_luminanceThreshold;
}

void SafetyManager::WatchdogThreadFunc() {
    while (!m_watchdogShouldStop) {
        MonitorPerformance();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SafetyManager::MonitorPerformance() {
    #ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        uint32 memMB = static_cast<uint32>(pmc.WorkingSetSize / (1024ull * 1024ull));
        if (memMB > m_memoryThresholdMB) {
            HandleSafetyViolation("Memory", std::string("UsageMB=") + std::to_string(memMB));
        }
    }
    #endif
    auto now = std::chrono::steady_clock::now();
    if (!m_lastHeartbeat.time_since_epoch().count()) m_lastHeartbeat = now;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastHeartbeat).count();
    if (elapsed > 5) {
        HandleSafetyViolation("Heartbeat", "No heartbeat in 5s");
        m_lastHeartbeat = now;
    }
}

void SafetyManager::HandleSafetyViolation(const std::string& type, const std::string& details) {
    m_violationCount++;
    
    std::lock_guard<std::mutex> lock(m_violationMutex);
    auto now = std::chrono::steady_clock::now();
    
    std::string violation = "[" + type + "] " + details;
    m_violations.push_back({now, violation});
    
    // Keep only recent violations
    if (m_violations.size() > 100) {
        m_violations.erase(m_violations.begin());
    }
    
    std::cout << "SAFETY VIOLATION: " << violation << std::endl;
    LogViolation(violation);
}

void SafetyManager::LogViolation(const std::string& violation) {
    const char* path = "safety_violations.log";
    {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (in.is_open()) {
            auto size = in.tellg();
            if (size > static_cast<std::streamoff>(1024 * 1024)) {
                in.close();
                std::remove("safety_violations.log.1");
                std::rename(path, "safety_violations.log.1");
            }
        }
    }
    std::ofstream logFile(path, std::ios::app);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char buf[26];
        ctime_s(buf, sizeof(buf), &time_t);
        logFile << buf << ": " << violation << std::endl;
    }
}

void SafetyManager::CleanupOldViolations() {
    std::lock_guard<std::mutex> lock(m_violationMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto it = m_violations.begin();
    
    while (it != m_violations.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - it->first);
        if (elapsed.count() > 24) { // Remove violations older than 24 hours
            it = m_violations.erase(it);
        } else {
            ++it;
        }
    }
}

// EpilepsyProtector Implementation
EpilepsyProtector::EpilepsyProtector() 
    : m_threshold(0.3f)
    , m_historySize(10)
    , m_frameRate(60)
    , m_flashCount(0)
    , m_protectionActive(false) {
}

EpilepsyProtector::~EpilepsyProtector() {
    Shutdown();
}

void EpilepsyProtector::Initialize(float32 threshold, uint32_t historySize) {
    m_threshold = threshold;
    m_historySize = historySize;
    m_protectionActive = true;
}

void EpilepsyProtector::Shutdown() {
    m_protectionActive = false;
    m_luminanceHistory.clear();
    m_flashHistory.clear();
}

float32 EpilepsyProtector::AnalyzeFrameLuminance(const uint8_t* frameData, uint32_t width, uint32_t height) {
    if (!frameData || width == 0 || height == 0) {
        return 0.0f;
    }
    
    uint64_t totalLuminance = 0;
    uint32_t sampleCount = 0;
    
    // Sample every 4th pixel for performance
    for (uint32_t y = 0; y < height; y += 4) {
        for (uint32_t x = 0; x < width; x += 4) {
            uint32_t index = (y * width + x) * 4; // RGBA format
            if (index + 2 < width * height * 4) {
                uint8_t r = frameData[index];
                uint8_t g = frameData[index + 1];
                uint8_t b = frameData[index + 2];
                
                // Calculate luminance using standard formula
                uint32_t luminance = (299 * r + 587 * g + 114 * b) / 1000;
                totalLuminance += luminance;
                sampleCount++;
            }
        }
    }
    
    return sampleCount > 0 ? static_cast<float32>(totalLuminance) / (sampleCount * 255.0f) : 0.0f;
}

float32 EpilepsyProtector::AnalyzeRegionLuminance(const uint8_t* regionData, uint32_t regionWidth, uint32_t regionHeight) {
    return AnalyzeFrameLuminance(regionData, regionWidth, regionHeight);
}

bool EpilepsyProtector::DetectFlash(float32 currentLuminance, float32 previousLuminance) {
    if (!m_protectionActive) {
        return false;
    }
    
    float32 delta = std::abs(currentLuminance - previousLuminance);
    
    if (IsRapidChange(delta)) {
        m_flashCount++;
        m_flashHistory.push_back(delta);
        
        if (m_flashHistory.size() > m_historySize) {
            m_flashHistory.erase(m_flashHistory.begin());
        }
        
        m_lastFlashTime = std::chrono::steady_clock::now();
        
        // Check if this is part of a dangerous pattern
        return DetectPatternedFlash(m_flashHistory);
    }
    
    return false;
}

bool EpilepsyProtector::DetectPatternedFlash(const std::vector<float32>& luminanceHistory) {
    if (luminanceHistory.size() < 3) {
        return false;
    }
    
    // Check for rapid alternating pattern
    uint32_t alternations = 0;
    for (size_t i = 1; i < luminanceHistory.size(); i++) {
        if ((luminanceHistory[i] > 0) != (luminanceHistory[i-1] > 0)) {
            alternations++;
        }
    }
    
    // Consider it dangerous if more than 70% of changes are alternations
    float32 alternationRatio = static_cast<float32>(alternations) / (luminanceHistory.size() - 1);
    return alternationRatio > 0.7f;
}

float32 EpilepsyProtector::ClampLuminanceChange(float32 targetLuminance, float32 previousLuminance) {
    if (!m_protectionActive) {
        return targetLuminance;
    }
    
    float32 delta = targetLuminance - previousLuminance;
    
    if (IsRapidChange(std::abs(delta))) {
        // Apply smoothing to reduce the change
        return ApplySmoothingFilter(targetLuminance, previousLuminance);
    }
    
    return targetLuminance;
}

uint8_t EpilepsyProtector::ClampPixelValue(uint8_t current, uint8_t previous) {
    if (!m_protectionActive) {
        return current;
    }
    
    float32 currentFloat = current / 255.0f;
    float32 previousFloat = previous / 255.0f;
    
    float32 clampedFloat = ClampLuminanceChange(currentFloat, previousFloat);
    
    return static_cast<uint8_t>(clampedFloat * 255.0f);
}

bool EpilepsyProtector::IsPatternDangerous(const std::vector<float32>& pattern) {
    if (pattern.size() < 3) {
        return false;
    }
    
    // Check for high-frequency patterns
    float32 frequency = CalculatePatternFrequency(pattern);
    
    // Patterns above 3 Hz are considered dangerous
    return frequency > 3.0f;
}

float32 EpilepsyProtector::CalculatePatternFrequency(const std::vector<float32>& pattern) {
    if (pattern.size() < 2) {
        return 0.0f;
    }
    
    // Count zero crossings
    uint32_t zeroCrossings = 0;
    for (size_t i = 1; i < pattern.size(); i++) {
        if ((pattern[i] > 0) != (pattern[i-1] > 0)) {
            zeroCrossings++;
        }
    }
    
    // Calculate frequency based on frame rate
    float32 duration = static_cast<float32>(pattern.size()) / m_frameRate;
    return zeroCrossings / (2.0f * duration); // Divide by 2 for full cycles
}

void EpilepsyProtector::SetHistorySize(uint32_t size) {
    m_historySize = size;
    
    // Resize history vectors
    if (m_luminanceHistory.size() > size) {
        m_luminanceHistory.resize(size);
    }
    
    if (m_flashHistory.size() > size) {
        m_flashHistory.resize(size);
    }
}

bool EpilepsyProtector::IsRapidChange(float32 delta) const {
    return delta > m_threshold;
}

bool EpilepsyProtector::IsPatternedChange(const std::vector<float32>& changes) const {
    if (changes.size() < 3) {
        return false;
    }
    
    // Check for regular pattern
    float32 avgChange = 0.0f;
    for (float32 change : changes) {
        avgChange += change;
    }
    avgChange /= changes.size();
    
    // Check if changes are consistently above average
    uint32_t aboveAvgCount = 0;
    for (float32 change : changes) {
        if (change > avgChange * 0.5f) {
            aboveAvgCount++;
        }
    }
    
    return static_cast<float32>(aboveAvgCount) / changes.size() > 0.6f;
}

float32 EpilepsyProtector::CalculateChangeRate(float32 current, float32 previous) const {
    return std::abs(current - previous) * m_frameRate;
}

float32 EpilepsyProtector::ApplySmoothingFilter(float32 target, float32 previous) {
    // Simple exponential smoothing
    float32 alpha = 0.3f; // Smoothing factor
    return alpha * target + (1.0f - alpha) * previous;
}

uint8_t EpilepsyProtector::ApplyPixelSmoothing(uint8_t current, uint8_t previous) {
    float32 currentFloat = current / 255.0f;
    float32 previousFloat = previous / 255.0f;
    
    float32 smoothedFloat = ApplySmoothingFilter(currentFloat, previousFloat);
    
    return static_cast<uint8_t>(smoothedFloat * 255.0f);
}

// ContentFilter Implementation
ContentFilter::ContentFilter() 
    : m_sensitivity(0.5f)
    , m_maxScore(1.0f)
    , m_filterCount(0)
    , m_blockedCount(0) {
}

ContentFilter::~ContentFilter() {
    Shutdown();
}

void ContentFilter::Initialize(float32 sensitivity) {
    m_sensitivity = sensitivity;
    
    // Add default keywords
    AddKeyword("explicit", 1.0f);
    AddKeyword("adult", 0.8f);
    AddKeyword("nsfw", 1.0f);
    AddKeyword("mature", 0.6f);
    AddKeyword("sensitive", 0.4f);
    
    // Add default whitelist words
    AddWhitelistWord("safe");
    AddWhitelistWord("clean");
    AddWhitelistWord("family-friendly");
    AddWhitelistWord("artistic");
    AddWhitelistWord("educational");
}

void ContentFilter::Shutdown() {
    m_keywords.clear();
    m_whitelist.clear();
    m_patterns.clear();
}

bool ContentFilter::ContainsNSFWText(const std::string& text) {
    m_filterCount++;
    
    float32 score = GetTextSafetyScore(text);
    bool unsafe = score > (m_maxScore * m_sensitivity);
    
    if (unsafe) {
        m_blockedCount++;
    }
    
    return unsafe;
}

std::string ContentFilter::FilterTextContent(const std::string& text) {
    if (!ContainsNSFWText(text)) {
        return text;
    }
    
    std::string filtered = text;
    std::vector<std::string> tokens = TokenizeText(text);
    
    for (const auto& token : tokens) {
        if (!IsWhitelisted(token)) {
            for (const auto& [keyword, weight] : m_keywords) {
                if (token.find(keyword) != std::string::npos) {
                    filtered = ReplaceWithAlternatives(filtered, token);
                    break;
                }
            }
        }
    }
    
    return filtered;
}

float32 ContentFilter::GetTextSafetyScore(const std::string& text) {
    std::string normalized = NormalizeText(text);
    std::vector<std::string> tokens = TokenizeText(normalized);
    
    float32 keywordScore = CalculateKeywordScore(tokens);
    float32 patternScore = CalculatePatternScore(normalized);
    
    return (std::max)(keywordScore, patternScore);
}

bool ContentFilter::ContainsNSFWASCII(const std::string& asciiArt) {
    m_filterCount++;
    
    float32 score = GetASCIISafetyScore(asciiArt);
    bool unsafe = score > (m_maxScore * m_sensitivity);
    
    if (unsafe) {
        m_blockedCount++;
    }
    
    return unsafe;
}

std::string ContentFilter::FilterASCIIContent(const std::string& asciiArt) {
    if (!ContainsNSFWASCII(asciiArt)) {
        return asciiArt;
    }
    
    // Simple ASCII filtering - replace suspicious patterns
    std::string filtered = asciiArt;
    
    if (HasSuspiciousPatterns(asciiArt)) {
        // Replace with safer characters
        std::replace(filtered.begin(), filtered.end(), '#', '*');
        std::replace(filtered.begin(), filtered.end(), '@', '+');
    }
    
    return filtered;
}

float32 ContentFilter::GetASCIISafetyScore(const std::string& asciiArt) {
    float32 patternDensity = CalculatePatternDensity(asciiArt);
    bool hasBlockedChars = ContainsBlockedCharacters(asciiArt);
    bool hasSuspiciousPatterns = HasSuspiciousPatterns(asciiArt);
    
    float32 score = patternDensity * 0.4f;
    if (hasBlockedChars) score += 0.3f;
    if (hasSuspiciousPatterns) score += 0.3f;
    
    return (std::min)(score, m_maxScore);
}

void ContentFilter::AddKeyword(const std::string& keyword, float32 weight) {
    m_keywords.push_back({keyword, weight});
}

void ContentFilter::RemoveKeyword(const std::string& keyword) {
    m_keywords.erase(
        std::remove_if(m_keywords.begin(), m_keywords.end(),
            [&keyword](const auto& kw) { return kw.first == keyword; }),
        m_keywords.end()
    );
}

void ContentFilter::AddWhitelistWord(const std::string& word) {
    m_whitelist.push_back(word);
}

void ContentFilter::RemoveWhitelistWord(const std::string& word) {
    m_whitelist.erase(
        std::remove(m_whitelist.begin(), m_whitelist.end(), word),
        m_whitelist.end()
    );
}

void ContentFilter::AddPattern(const std::string& pattern, float32 weight) {
    m_patterns.push_back({pattern, weight});
}

bool ContentFilter::MatchesPattern(const std::string& text, const std::string& pattern) {
    std::regex regex(pattern, std::regex_constants::icase);
    return std::regex_search(text, regex);
}

std::string ContentFilter::NormalizeText(const std::string& text) {
    std::string normalized = text;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove punctuation
    normalized.erase(
        std::remove_if(normalized.begin(), normalized.end(),
            [](char c) { return std::ispunct(c); }),
        normalized.end()
    );
    
    return normalized;
}

std::vector<std::string> ContentFilter::TokenizeText(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    
    while (stream >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

float32 ContentFilter::CalculateKeywordScore(const std::vector<std::string>& tokens) {
    float32 totalScore = 0.0f;
    uint32_t keywordCount = 0;
    
    for (const auto& token : tokens) {
        for (const auto& [keyword, weight] : m_keywords) {
            if (token.find(keyword) != std::string::npos) {
                totalScore += weight;
                keywordCount++;
                break;
            }
        }
    }
    
    return keywordCount > 0 ? totalScore / tokens.size() : 0.0f;
}

float32 ContentFilter::CalculatePatternScore(const std::string& text) {
    float32 totalScore = 0.0f;
    
    for (const auto& [pattern, weight] : m_patterns) {
        if (MatchesPattern(text, pattern)) {
            totalScore += weight;
        }
    }
    
    return (std::min)(totalScore, m_maxScore);
}

bool ContentFilter::HasSuspiciousPatterns(const std::string& asciiArt) {
    // Check for patterns that might be inappropriate
    std::string normalized = NormalizeText(asciiArt);
    
    // Look for repetitive patterns that might indicate inappropriate content
    std::map<std::string, uint32_t> patternCounts;
    
    for (size_t i = 0; i < normalized.length() - 2; i++) {
        std::string pattern = normalized.substr(i, 3);
        patternCounts[pattern]++;
    }
    
    // Check if any pattern appears too frequently
    for (const auto& [pattern, count] : patternCounts) {
        if (count > asciiArt.length() * 0.1f) { // Pattern appears in >10% of text
            return true;
        }
    }
    
    return false;
}

float32 ContentFilter::CalculatePatternDensity(const std::string& asciiArt) {
    uint32_t specialChars = 0;
    uint32_t totalChars = 0;
    
    for (char c : asciiArt) {
        if (std::isgraph(c)) {
            totalChars++;
            if (c == '#' || c == '@' || c == '*' || c == '&') {
                specialChars++;
            }
        }
    }
    
    return totalChars > 0 ? static_cast<float32>(specialChars) / totalChars : 0.0f;
}

bool ContentFilter::ContainsBlockedCharacters(const std::string& asciiArt) {
    const std::string blockedChars = "#@*&%$";
    
    for (char c : asciiArt) {
        if (blockedChars.find(c) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

std::string ContentFilter::ReplaceWithAlternatives(const std::string& text, const std::string& word) {
    std::string alternatives = "[content]";
    
    std::string result = text;
    size_t pos = 0;
    
    while ((pos = result.find(word, pos)) != std::string::npos) {
        result.replace(pos, word.length(), alternatives);
        pos += alternatives.length();
    }
    
    return result;
}

bool ContentFilter::IsWhitelisted(const std::string& word) {
    return std::find(m_whitelist.begin(), m_whitelist.end(), word) != m_whitelist.end();
}

float32 ContentFilter::GetKeywordWeight(const std::string& keyword) {
    for (const auto& [kw, weight] : m_keywords) {
        if (kw == keyword) {
            return weight;
        }
    }
    
    return 0.0f;
}

// CrashRecovery Implementation
CrashRecovery::CrashRecovery() 
    : m_maxRestarts(3)
    , m_restartDelayMs(2000)
    , m_recoveryTimeoutMs(30000)
    , m_crashCount(0)
    , m_restartCount(0)
    , m_successfulOperations(0)
    , m_failedOperations(0)
    , m_recoveryMode(false)
    , m_crashDetected(false) {
}

CrashRecovery::~CrashRecovery() {
    Shutdown();
}

Result CrashRecovery::Initialize(uint32_t maxRestarts, uint32_t restartDelayMs) {
    m_maxRestarts = maxRestarts;
    m_restartDelayMs = restartDelayMs;
    
    std::cout << "Crash recovery initialized: max " << maxRestarts << " restarts, " 
              << restartDelayMs << "ms delay" << std::endl;
    
    return Result::Success;
}

void CrashRecovery::Shutdown() {
    // Cleanup crash history
    m_crashHistory.clear();
}

void CrashRecovery::SignalCrash(const std::string& crashInfo) {
    m_crashDetected = true;
    m_lastCrashTime = std::chrono::steady_clock::now();
    m_crashCount++;
    
    LogCrash(crashInfo);
    
    std::cout << "Crash detected! Count: " << m_crashCount << std::endl;
    std::cout << "Crash info: " << crashInfo << std::endl;
}

bool CrashRecovery::ShouldRecover() const {
    if (!m_crashDetected) {
        return false;
    }
    
    return IsTimeForRecovery() && !HasExceededRestartLimit() && !IsRecoveryTimeout();
}

bool CrashRecovery::CanRestart() const {
    return ShouldRecover() && IsApplicationRunning();
}

Result CrashRecovery::AttemptRecovery() {
    if (!ShouldRecover()) {
        return Result::Error;
    }
    
    m_recoveryMode = true;
    m_lastRestartTime = std::chrono::steady_clock::now();
    
    std::cout << "Attempting crash recovery..." << std::endl;
    
    // In a real implementation, would attempt various recovery strategies
    
    return RestartApplication();
}

Result CrashRecovery::RestartApplication() {
    if (!CanRestart()) {
        return Result::Error;
    }
    
    m_restartCount++;
    
    std::cout << "Restarting application (attempt " << m_restartCount << "/" << m_maxRestarts << ")" << std::endl;
    
    // In a real implementation, would restart the application process
    
    // For now, just reset crash state
    m_crashDetected = false;
    m_recoveryMode = false;
    
    return Result::Success;
}

Result CrashRecovery::SaveCrashDump(const std::string& crashInfo) {
    std::string dump = GenerateCrashDump(crashInfo);
    
    std::ofstream dumpFile("crash_dump_" + std::to_string(m_crashCount) + ".log");
    if (dumpFile.is_open()) {
        dumpFile << dump;
        dumpFile.close();
        return Result::Success;
    }
    
    return Result::FileNotFound;
}

void CrashRecovery::MarkSuccessfulOperation() {
    m_successfulOperations++;
    
    // Reset recovery mode on successful operation
    if (m_recoveryMode) {
        ResetRecoveryState();
    }
}

void CrashRecovery::MarkFailedOperation() {
    m_failedOperations++;
    
    // Consider marking as crash if too many consecutive failures
    if (m_failedOperations > 10 && m_successfulOperations == 0) {
        SignalCrash("Multiple consecutive failures detected");
    }
}

bool CrashRecovery::IsTimeForRecovery() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastCrashTime);
    
    return elapsed.count() >= m_restartDelayMs;
}

bool CrashRecovery::HasExceededRestartLimit() const {
    return m_restartCount >= m_maxRestarts;
}

bool CrashRecovery::IsRecoveryTimeout() const {
    if (!m_recoveryMode) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastRestartTime);
    
    return elapsed.count() >= m_recoveryTimeoutMs;
}

void CrashRecovery::LogCrash(const std::string& crashInfo) {
    std::lock_guard<std::mutex> lock(m_crashMutex);
    
    m_crashHistory.push_back(crashInfo);
    
    if (m_crashHistory.size() > 10) {
        m_crashHistory.erase(m_crashHistory.begin());
    }
    
    SaveCrashDump(crashInfo);
}

void CrashRecovery::ResetRecoveryState() {
    m_recoveryMode = false;
    m_crashDetected = false;
    m_successfulOperations = 0;
    m_failedOperations = 0;
}

Result CrashRecovery::TerminateApplication() {
    // In a real implementation, would terminate the application process
    std::cout << "Terminating application..." << std::endl;
    return Result::Success;
}

Result CrashRecovery::LaunchApplication() {
    // In a real implementation, would launch the application process
    std::cout << "Launching application..." << std::endl;
    return Result::Success;
}

bool CrashRecovery::IsApplicationRunning() const {
    // In a real implementation, would check if application is running
    return true; // Placeholder
}

std::string CrashRecovery::GenerateCrashDump(const std::string& crashInfo) {
    std::stringstream dump;
    
    dump << "=== NEON-GLYPH CRASH DUMP ===" << std::endl;
    dump << "Crash Count: " << m_crashCount << std::endl;
    dump << "Restart Count: " << m_restartCount << std::endl;
    dump << "Time: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    dump << "Info: " << crashInfo << std::endl;
    dump << "System Info: " << GetSystemInfo() << std::endl;
    dump << "Memory Info: " << GetMemoryInfo() << std::endl;
    dump << "Application State: " << GetApplicationState() << std::endl;
    dump << "=============================" << std::endl;
    
    return dump.str();
}

std::string CrashRecovery::GetSystemInfo() const {
    std::stringstream info;
    
    #ifdef _WIN32
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    info << "Windows " << sysInfo.dwNumberOfProcessors << " cores";
    #else
    info << "Non-Windows system";
    #endif
    
    return info.str();
}

std::string CrashRecovery::GetMemoryInfo() const {
    std::stringstream info;
    
    #ifdef _WIN32
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        info << "Physical: " << (memStatus.ullTotalPhys / (1024*1024*1024)) << "GB, ";
        info << "Available: " << (memStatus.ullAvailPhys / (1024*1024*1024)) << "GB";
    }
    #else
    info << "Memory info unavailable";
    #endif
    
    return info.str();
}

std::string CrashRecovery::GetApplicationState() const {
    std::stringstream state;
    
    state << "Successful operations: " << m_successfulOperations << ", ";
    state << "Failed operations: " << m_failedOperations << ", ";
    state << "Recovery mode: " << (m_recoveryMode ? "Yes" : "No");
    
    return state.str();
}

} // namespace NeonGlyph
