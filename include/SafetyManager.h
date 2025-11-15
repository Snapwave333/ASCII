#pragma once

#include "NeonGlyph.h"
#include <chrono>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace NeonGlyph {

// Safety manager for epilepsy protection, NSFW filtering, and crash recovery
class SafetyManager {
public:
    SafetyManager();
    ~SafetyManager();

    Result Initialize(const Config& config);
    void Shutdown();
    
    // Epilepsy protection
    bool ShouldClampBrightness(float32 currentLuminance, float32 previousLuminance);
    float32 CalculateLuminanceDelta(float32 current, float32 previous);
    float32 GetSafeBrightnessLevel(float32 targetBrightness);
    
    // NSFW content filtering
    bool IsContentSafe(const std::string& content);
    std::string FilterContent(const std::string& content);
    void AddNSFWKeyword(const std::string& keyword);
    void RemoveNSFWKeyword(const std::string& keyword);
    
    // Crash recovery and watchdog
    Result EnableCrashRecovery();
    void OnCrashDetected();
    bool ShouldRestartApplication();
    
    // Performance monitoring
    bool IsPerformanceAcceptable(const PerformanceMetrics& metrics);
    void LogSafetyViolation(const std::string& violation);
    std::vector<std::string> GetRecentViolations() const;
    void OnHeartbeat();
    
    // Safety thresholds and configuration
    void SetEpilepsyThreshold(float32 threshold);
    void SetPerformanceThresholds(float32 maxFrameTime, uint32_t maxMemoryMB);
    void SetContentFilterSensitivity(float32 sensitivity);
    
    // Safety status
    bool IsEpilepsyModeEnabled() const { return m_epilepsyModeEnabled; }
    bool IsContentFilteringEnabled() const { return m_contentFilteringEnabled; }
    bool IsCrashRecoveryEnabled() const { return m_crashRecoveryEnabled; }
    uint32_t GetViolationCount() const { return m_violationCount; }

private:
    Config m_config;
    
    // Epilepsy protection
    bool m_epilepsyModeEnabled;
    float32 m_luminanceThreshold;
    std::vector<float32> m_luminanceHistory;
    std::chrono::steady_clock::time_point m_lastLuminanceUpdate;
    
    // NSFW filtering
    bool m_contentFilteringEnabled;
    std::vector<std::string> m_nsfwKeywords;
    std::vector<std::string> m_safetyWhitelist;
    float32 m_filterSensitivity;
    
    // Crash recovery
    bool m_crashRecoveryEnabled;
    std::atomic<bool> m_crashDetected;
    std::chrono::steady_clock::time_point m_lastCrashTime;
    uint32_t m_crashCount;
    
    // Performance monitoring
    float32 m_maxFrameTimeMs;
    uint32_t m_maxMemoryMB;
    std::vector<PerformanceMetrics> m_performanceHistory;
    std::chrono::steady_clock::time_point m_lastHeartbeat;
    uint32_t m_memoryThresholdMB;
    
    // Safety violations
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>> m_violations;
    std::atomic<uint32_t> m_violationCount;
    mutable std::mutex m_violationMutex;
    
    // Watchdog thread
    std::thread m_watchdogThread;
    std::atomic<bool> m_watchdogShouldStop;
    
    // Private methods
    void InitializeNSFWKeywords();
    void InitializeSafetyWhitelist();
    bool ContainsNSFWContent(const std::string& content);
    std::string CleanNSFWContent(const std::string& content);
    float32 CalculateRollingAverageLuminanceDelta() const;
    bool IsLuminanceChangeDangerous(float32 delta) const;
    void WatchdogThreadFunc();
    void MonitorPerformance();
    void HandleSafetyViolation(const std::string& type, const std::string& details);
    void LogViolation(const std::string& violation);
    void CleanupOldViolations();
};

// Epilepsy protection utilities
class EpilepsyProtector {
public:
    EpilepsyProtector();
    ~EpilepsyProtector();
    
    void Initialize(float32 threshold = 0.3f, uint32_t historySize = 10);
    void Shutdown();
    
    // Luminance analysis
    float32 AnalyzeFrameLuminance(const uint8_t* frameData, uint32_t width, uint32_t height);
    float32 AnalyzeRegionLuminance(const uint8_t* regionData, uint32_t regionWidth, uint32_t regionHeight);
    
    // Flash detection
    bool DetectFlash(float32 currentLuminance, float32 previousLuminance);
    bool DetectPatternedFlash(const std::vector<float32>& luminanceHistory);
    
    // Safety clamping
    float32 ClampLuminanceChange(float32 targetLuminance, float32 previousLuminance);
    uint8_t ClampPixelValue(uint8_t current, uint8_t previous);
    
    // Pattern analysis
    bool IsPatternDangerous(const std::vector<float32>& pattern);
    float32 CalculatePatternFrequency(const std::vector<float32>& pattern);
    
    // Configuration
    void SetThreshold(float32 threshold) { m_threshold = threshold; }
    void SetHistorySize(uint32_t size);
    void SetFrameRate(uint32_t fps) { m_frameRate = fps; }
    
    // Status
    float32 GetThreshold() const { return m_threshold; }
    uint32_t GetHistorySize() const { return m_historySize; }
    uint32_t GetFlashCount() const { return m_flashCount; }
    bool IsProtectionActive() const { return m_protectionActive; }

private:
    float32 m_threshold;
    uint32_t m_historySize;
    uint32_t m_frameRate;
    uint32_t m_flashCount;
    bool m_protectionActive;
    
    std::vector<float32> m_luminanceHistory;
    std::vector<float32> m_flashHistory;
    std::chrono::steady_clock::time_point m_lastFlashTime;
    
    // Flash detection algorithms
    bool IsRapidChange(float32 delta) const;
    bool IsPatternedChange(const std::vector<float32>& changes) const;
    float32 CalculateChangeRate(float32 current, float32 previous) const;
    
    // Safety algorithms
    float32 ApplySmoothingFilter(float32 target, float32 previous);
    uint8_t ApplyPixelSmoothing(uint8_t current, uint8_t previous);
};

// Content filter for NSFW detection
class ContentFilter {
public:
    ContentFilter();
    ~ContentFilter();
    
    void Initialize(float32 sensitivity = 0.5f);
    void Shutdown();
    
    // Text content filtering
    bool ContainsNSFWText(const std::string& text);
    std::string FilterTextContent(const std::string& text);
    float GetTextSafetyScore(const std::string& text);
    
    // ASCII art content filtering
    bool ContainsNSFWASCII(const std::string& asciiArt);
    std::string FilterASCIIContent(const std::string& asciiArt);
    float GetASCIISafetyScore(const std::string& asciiArt);
    
    // Keyword management
    void AddKeyword(const std::string& keyword, float32 weight = 1.0f);
    void RemoveKeyword(const std::string& keyword);
    void AddWhitelistWord(const std::string& word);
    void RemoveWhitelistWord(const std::string& word);
    
    // Pattern matching
    void AddPattern(const std::string& pattern, float32 weight = 1.0f);
    bool MatchesPattern(const std::string& text, const std::string& pattern);
    
    // Configuration
    void SetSensitivity(float32 sensitivity) { m_sensitivity = sensitivity; }
    void SetMaxScore(float32 maxScore) { m_maxScore = maxScore; }
    float GetSensitivity() const { return m_sensitivity; }
    float GetMaxScore() const { return m_maxScore; }
    
    // Statistics
    uint32_t GetFilterCount() const { return m_filterCount; }
    uint32_t GetBlockedCount() const { return m_blockedCount; }
    void ResetStatistics() { m_filterCount = 0; m_blockedCount = 0; }

private:
    float32 m_sensitivity;
    float32 m_maxScore;
    uint32_t m_filterCount;
    uint32_t m_blockedCount;
    
    std::vector<std::pair<std::string, float32>> m_keywords;
    std::vector<std::string> m_whitelist;
    std::vector<std::pair<std::string, float32>> m_patterns;
    
    // Text processing
    std::string NormalizeText(const std::string& text);
    std::vector<std::string> TokenizeText(const std::string& text);
    float32 CalculateKeywordScore(const std::vector<std::string>& tokens);
    float32 CalculatePatternScore(const std::string& text);
    
    // ASCII analysis
    bool HasSuspiciousPatterns(const std::string& asciiArt);
    float32 CalculatePatternDensity(const std::string& asciiArt);
    bool ContainsBlockedCharacters(const std::string& asciiArt);
    
    // Safety utilities
    std::string ReplaceWithAlternatives(const std::string& text, const std::string& word);
    bool IsWhitelisted(const std::string& word);
    float32 GetKeywordWeight(const std::string& keyword);
};

// Crash recovery and watchdog system
class CrashRecovery {
public:
    CrashRecovery();
    ~CrashRecovery();
    
    Result Initialize(uint32_t maxRestarts = 3, uint32_t restartDelayMs = 2000);
    void Shutdown();
    
    // Crash detection
    void SignalCrash(const std::string& crashInfo);
    bool ShouldRecover() const;
    bool CanRestart() const;
    
    // Recovery actions
    Result AttemptRecovery();
    Result RestartApplication();
    Result SaveCrashDump(const std::string& crashInfo);
    
    // State management
    void MarkSuccessfulOperation();
    void MarkFailedOperation();
    bool IsInRecoveryMode() const { return m_recoveryMode; }
    
    // Configuration
    void SetMaxRestarts(uint32_t maxRestarts) { m_maxRestarts = maxRestarts; }
    void SetRestartDelay(uint32_t delayMs) { m_restartDelayMs = delayMs; }
    void SetRecoveryTimeout(uint32_t timeoutMs) { m_recoveryTimeoutMs = timeoutMs; }
    
    // Statistics
    uint32_t GetCrashCount() const { return m_crashCount; }
    uint32_t GetRestartCount() const { return m_restartCount; }
    uint32_t GetSuccessfulOperations() const { return m_successfulOperations; }
    uint32_t GetFailedOperations() const { return m_failedOperations; }
    std::chrono::steady_clock::time_point GetLastCrashTime() const { return m_lastCrashTime; }

private:
    uint32_t m_maxRestarts;
    uint32_t m_restartDelayMs;
    uint32_t m_recoveryTimeoutMs;
    uint32_t m_crashCount;
    uint32_t m_restartCount;
    uint32_t m_successfulOperations;
    uint32_t m_failedOperations;
    bool m_recoveryMode;
    
    std::atomic<bool> m_crashDetected;
    std::chrono::steady_clock::time_point m_lastCrashTime;
    std::chrono::steady_clock::time_point m_lastRestartTime;
    std::vector<std::string> m_crashHistory;
    std::mutex m_crashMutex;
    
    // Recovery utilities
    bool IsTimeForRecovery() const;
    bool HasExceededRestartLimit() const;
    bool IsRecoveryTimeout() const;
    void LogCrash(const std::string& crashInfo);
    void ResetRecoveryState();
    
    // Application management
    Result TerminateApplication();
    Result LaunchApplication();
    bool IsApplicationRunning() const;
    
    // System utilities
    std::string GenerateCrashDump(const std::string& crashInfo);
    std::string GetSystemInfo() const;
    std::string GetMemoryInfo() const;
    std::string GetApplicationState() const;
};

} // namespace NeonGlyph
    void OnHeartbeat();
