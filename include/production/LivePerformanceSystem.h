#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>

namespace NeonGlyph {
namespace Production {

enum class PerformanceState {
    IDLE,
    WARMUP,
    LIVE,
    DEGRADED,
    EMERGENCY,
    RECOVERY,
    SHUTDOWN
};

enum class FallbackLevel {
    NONE = 0,
    LEVEL_1_OPTIMIZATION = 1,    // Reduce quality slightly
    LEVEL_2_SIMPLIFICATION = 2,  // Simplify effects
    LEVEL_3_MINIMAL = 3,           // Minimal viable visuals
    LEVEL_4_EMERGENCY = 4,         // Emergency mode
    LEVEL_5_CRITICAL = 5            // Critical shutdown
};

enum class PerformanceMetric {
    FRAME_TIME,
    RENDER_TIME,
    AUDIO_LATENCY,
    GPU_UTILIZATION,
    CPU_UTILIZATION,
    MEMORY_USAGE,
    DRAW_CALLS,
    SHADER_SWITCHES,
    TEXTURE_BINDS,
    BUFFER_UPLOADS
};

struct PerformanceThreshold {
    PerformanceMetric metric;
    double warning_threshold;      // Yellow alert
    double critical_threshold;     // Red alert
    double emergency_threshold;    // Immediate action required
    std::string description;
};

struct PerformanceSnapshot {
    std::chrono::steady_clock::time_point timestamp;
    std::unordered_map<PerformanceMetric, double> metrics;
    PerformanceState state;
    FallbackLevel current_fallback;
    std::string active_theme;
    size_t active_effects_count;
    double estimated_frame_time;
};

struct FallbackAction {
    FallbackLevel level;
    std::string description;
    std::function<void()> action;
    bool is_reversible;
    std::string recovery_action;
};

struct EmergencyProcedure {
    std::string name;
    std::vector<FallbackAction> actions;
    std::chrono::milliseconds execution_time;
    bool auto_recover;
};

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    void StartMonitoring();
    void StopMonitoring();
    
    void AddThreshold(const PerformanceThreshold& threshold);
    void UpdateMetric(PerformanceMetric metric, double value);
    
    PerformanceSnapshot GetCurrentSnapshot() const;
    std::vector<PerformanceSnapshot> GetRecentHistory(size_t count = 100) const;
    
    bool IsPerformanceAcceptable() const;
    bool IsCriticalPerformance() const;
    bool IsEmergencySituation() const;
    
    void SetAlertCallback(std::function<void(PerformanceMetric, double, double)> callback);

private:
    void MonitoringThread();
    void CheckThresholds();
    void RecordSnapshot();
    
    std::atomic<bool> monitoring_active_;
    std::thread monitoring_thread_;
    mutable std::mutex metrics_mutex_;
    
    std::unordered_map<PerformanceMetric, double> current_metrics_;
    std::vector<PerformanceThreshold> thresholds_;
    std::vector<PerformanceSnapshot> history_;
    
    std::function<void(PerformanceMetric, double, double)> alert_callback_;
    std::chrono::milliseconds monitoring_interval_;
};

class FallbackManager {
public:
    FallbackManager();
    ~FallbackManager();

    void RegisterFallbackAction(const FallbackAction& action);
    void RegisterEmergencyProcedure(const EmergencyProcedure& procedure);
    
    bool ActivateFallback(FallbackLevel level, const std::string& reason = "");
    bool DeactivateFallback(FallbackLevel level);
    bool RecoverToLevel(FallbackLevel target_level);
    
    FallbackLevel GetCurrentFallbackLevel() const;
    std::vector<FallbackAction> GetActiveFallbacks() const;
    std::string GetFallbackStatus() const;
    
    void ExecuteEmergencyProcedure(const std::string& procedure_name);
    void AutoRecover();

private:
    void ExecuteFallbackAction(const FallbackAction& action);
    bool CanDeactivateFallback(FallbackLevel level) const;
    
    std::atomic<FallbackLevel> current_level_;
    std::vector<FallbackAction> registered_actions_;
    std::unordered_map<std::string, EmergencyProcedure> emergency_procedures_;
    std::vector<FallbackAction> active_fallbacks_;
    
    mutable std::mutex fallback_mutex_;
    std::chrono::steady_clock::time_point last_fallback_time_;
};

class LivePerformanceSystem {
public:
    LivePerformanceSystem(const std::string& performance_name = "Live Performance");
    ~LivePerformanceSystem();

    // Performance Lifecycle
    bool Initialize();
    bool StartPerformance();
    bool EndPerformance();
    void EmergencyShutdown();
    
    // Real-time Management
    void Update();
    void ProcessFrame();
    
    // Performance Monitoring
    PerformanceMonitor& GetPerformanceMonitor() { return performance_monitor_; }
    FallbackManager& GetFallbackManager() { return fallback_manager_; }
    
    // State Management
    PerformanceState GetCurrentState() const;
    std::string GetStateDescription() const;
    bool IsPerformanceActive() const;
    
    // Configuration
    void SetTargetFrameTime(double milliseconds);
    void SetPerformanceName(const std::string& name);
    void EnableAutoFallback(bool enabled);
    void SetEmergencyMode(bool emergency);
    
    // Reporting
    std::string GetPerformanceReport() const;
    std::vector<std::string> GetRecommendations() const;
    void LogPerformanceEvent(const std::string& event);
    double GetElapsedTime() const;

private:
    void InitializeDefaultThresholds();
    void InitializeFallbackActions();
    void InitializeEmergencyProcedures();
    
    void HandlePerformanceAlert(PerformanceMetric metric, double value, double threshold);
    void HandleStateTransition(PerformanceState new_state);
    void PerformAutomaticFallback();
    
    PerformanceState current_state_;
    std::string performance_name_;
    
    PerformanceMonitor performance_monitor_;
    FallbackManager fallback_manager_;
    
    std::atomic<bool> auto_fallback_enabled_;
    std::atomic<bool> emergency_mode_;
    double target_frame_time_ms_;
    
    std::chrono::steady_clock::time_point performance_start_time_;
    std::chrono::steady_clock::time_point last_frame_time_;
    
    std::vector<std::string> performance_log_;
    mutable std::mutex log_mutex_;
    
    // Performance tracking
    size_t total_frames_;
    size_t dropped_frames_;
    double average_frame_time_;
    double max_frame_time_;
    
    // Emergency recovery
    bool recovery_in_progress_;
    std::chrono::steady_clock::time_point emergency_start_time_;
};

// Utility functions
std::string PerformanceMetricToString(PerformanceMetric metric);
std::string PerformanceStateToString(PerformanceState state);
std::string FallbackLevelToString(FallbackLevel level);
double GetRecommendedThreshold(PerformanceMetric metric, const std::string& scenario = "live_vj");

} // namespace Production
} // namespace NeonGlyph