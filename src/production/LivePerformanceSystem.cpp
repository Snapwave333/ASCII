#include "production/LivePerformanceSystem.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace NeonGlyph {
namespace Production {

// PerformanceMonitor Implementation
PerformanceMonitor::PerformanceMonitor() 
    : monitoring_active_(false), monitoring_interval_(16) {  // 16ms for 60fps
    current_metrics_[PerformanceMetric::FRAME_TIME] = 0.0;
    current_metrics_[PerformanceMetric::RENDER_TIME] = 0.0;
    current_metrics_[PerformanceMetric::AUDIO_LATENCY] = 0.0;
    current_metrics_[PerformanceMetric::GPU_UTILIZATION] = 0.0;
    current_metrics_[PerformanceMetric::CPU_UTILIZATION] = 0.0;
    current_metrics_[PerformanceMetric::MEMORY_USAGE] = 0.0;
    current_metrics_[PerformanceMetric::DRAW_CALLS] = 0.0;
    current_metrics_[PerformanceMetric::SHADER_SWITCHES] = 0.0;
    current_metrics_[PerformanceMetric::TEXTURE_BINDS] = 0.0;
    current_metrics_[PerformanceMetric::BUFFER_UPLOADS] = 0.0;
}

PerformanceMonitor::~PerformanceMonitor() {
    StopMonitoring();
}

void PerformanceMonitor::StartMonitoring() {
    if (monitoring_active_.exchange(true)) {
        return;  // Already monitoring
    }
    
    monitoring_thread_ = std::thread(&PerformanceMonitor::MonitoringThread, this);
}

void PerformanceMonitor::StopMonitoring() {
    if (!monitoring_active_.exchange(false)) {
        return;  // Not monitoring
    }
    
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

void PerformanceMonitor::AddThreshold(const PerformanceThreshold& threshold) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    thresholds_.push_back(threshold);
}

void PerformanceMonitor::UpdateMetric(PerformanceMetric metric, double value) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_[metric] = value;
}

PerformanceSnapshot PerformanceMonitor::GetCurrentSnapshot() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    PerformanceSnapshot snapshot;
    snapshot.timestamp = std::chrono::steady_clock::now();
    snapshot.metrics = current_metrics_;
    snapshot.estimated_frame_time = current_metrics_.at(PerformanceMetric::FRAME_TIME);
    
    return snapshot;
}

std::vector<PerformanceSnapshot> PerformanceMonitor::GetRecentHistory(size_t count) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::vector<PerformanceSnapshot> result;
    size_t start_idx = history_.size() > count ? history_.size() - count : 0;
    
    for (size_t i = start_idx; i < history_.size(); ++i) {
        result.push_back(history_[i]);
    }
    
    return result;
}

bool PerformanceMonitor::IsPerformanceAcceptable() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    for (const auto& threshold : thresholds_) {
        auto it = current_metrics_.find(threshold.metric);
        if (it != current_metrics_.end() && it->second > threshold.critical_threshold) {
            return false;
        }
    }
    return true;
}

bool PerformanceMonitor::IsCriticalPerformance() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    for (const auto& threshold : thresholds_) {
        auto it = current_metrics_.find(threshold.metric);
        if (it != current_metrics_.end() && it->second > threshold.critical_threshold) {
            return true;
        }
    }
    return false;
}

bool PerformanceMonitor::IsEmergencySituation() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    for (const auto& threshold : thresholds_) {
        auto it = current_metrics_.find(threshold.metric);
        if (it != current_metrics_.end() && it->second > threshold.emergency_threshold) {
            return true;
        }
    }
    return false;
}

void PerformanceMonitor::SetAlertCallback(std::function<void(PerformanceMetric, double, double)> callback) {
    alert_callback_ = callback;
}

void PerformanceMonitor::MonitoringThread() {
    while (monitoring_active_) {
        CheckThresholds();
        RecordSnapshot();
        std::this_thread::sleep_for(monitoring_interval_);
    }
}

void PerformanceMonitor::CheckThresholds() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    for (const auto& threshold : thresholds_) {
        auto it = current_metrics_.find(threshold.metric);
        if (it != current_metrics_.end()) {
            double value = it->second;
            
            if (value > threshold.emergency_threshold) {
                if (alert_callback_) {
                    alert_callback_(threshold.metric, value, threshold.emergency_threshold);
                }
            } else if (value > threshold.critical_threshold) {
                if (alert_callback_) {
                    alert_callback_(threshold.metric, value, threshold.critical_threshold);
                }
            } else if (value > threshold.warning_threshold) {
                if (alert_callback_) {
                    alert_callback_(threshold.metric, value, threshold.warning_threshold);
                }
            }
        }
    }
}

void PerformanceMonitor::RecordSnapshot() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    PerformanceSnapshot snapshot;
    snapshot.timestamp = std::chrono::steady_clock::now();
    snapshot.metrics = current_metrics_;
    
    history_.push_back(snapshot);
    
    // Keep only last 1000 snapshots to prevent memory growth
    if (history_.size() > 1000) {
        history_.erase(history_.begin());
    }
}

// FallbackManager Implementation
FallbackManager::FallbackManager() : current_level_(FallbackLevel::NONE) {}

FallbackManager::~FallbackManager() = default;

void FallbackManager::RegisterFallbackAction(const FallbackAction& action) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    registered_actions_.push_back(action);
}

void FallbackManager::RegisterEmergencyProcedure(const EmergencyProcedure& procedure) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    emergency_procedures_[procedure.name] = procedure;
}

bool FallbackManager::ActivateFallback(FallbackLevel level, const std::string& reason) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    
    if (static_cast<int>(level) <= static_cast<int>(current_level_.load())) {
        return false;  // Already at this level or higher
    }
    
    // Execute all actions for this level
    for (const auto& action : registered_actions_) {
        if (action.level == level) {
            ExecuteFallbackAction(action);
            active_fallbacks_.push_back(action);
        }
    }
    
    current_level_ = level;
    last_fallback_time_ = std::chrono::steady_clock::now();
    
    return true;
}

bool FallbackManager::DeactivateFallback(FallbackLevel level) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    
    if (!CanDeactivateFallback(level)) {
        return false;
    }
    
    // Remove actions for this level
    active_fallbacks_.erase(
        std::remove_if(active_fallbacks_.begin(), active_fallbacks_.end(),
            [level](const FallbackAction& action) { return action.level == level; }),
        active_fallbacks_.end()
    );
    
    current_level_ = static_cast<FallbackLevel>(static_cast<int>(level) - 1);
    
    return true;
}

bool FallbackManager::RecoverToLevel(FallbackLevel target_level) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    
    if (static_cast<int>(target_level) >= static_cast<int>(current_level_.load())) {
        return false;  // Already at target level or higher
    }
    
    // Deactivate fallbacks from current level down to target level
    for (int level = static_cast<int>(current_level_.load()); level > static_cast<int>(target_level); --level) {
        DeactivateFallback(static_cast<FallbackLevel>(level));
    }
    
    return true;
}

FallbackLevel FallbackManager::GetCurrentFallbackLevel() const {
    return current_level_;
}

std::vector<FallbackAction> FallbackManager::GetActiveFallbacks() const {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    return active_fallbacks_;
}

std::string FallbackManager::GetFallbackStatus() const {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    
    std::stringstream ss;
    ss << "Current Fallback Level: " << FallbackLevelToString(current_level_) << "\n";
    ss << "Active Fallbacks: " << active_fallbacks_.size() << "\n";
    
    for (const auto& action : active_fallbacks_) {
        ss << "  - " << action.description << " (Level " 
           << static_cast<int>(action.level) << ")\n";
    }
    
    return ss.str();
}

void FallbackManager::ExecuteEmergencyProcedure(const std::string& procedure_name) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    
    auto it = emergency_procedures_.find(procedure_name);
    if (it != emergency_procedures_.end()) {
        const EmergencyProcedure& procedure = it->second;
        
        for (const auto& action : procedure.actions) {
            ExecuteFallbackAction(action);
        }
        
        if (procedure.auto_recover) {
            // Schedule recovery after execution time
            // This would typically use a timer or scheduled task
        }
    }
}

void FallbackManager::AutoRecover() {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    
    // Simple auto-recovery: deactivate highest level fallback
    if (current_level_ != FallbackLevel::NONE) {
        DeactivateFallback(current_level_);
    }
}

void FallbackManager::ExecuteFallbackAction(const FallbackAction& action) {
    if (action.action) {
        try {
            action.action();
        } catch (const std::exception& e) {
            // Log error but don't crash
            std::cerr << "Error executing fallback action: " << e.what() << std::endl;
        }
    }
}

bool FallbackManager::CanDeactivateFallback(FallbackLevel level) const {
    // Check if any active fallback at this level is irreversible
    for (const auto& action : active_fallbacks_) {
        if (action.level == level && !action.is_reversible) {
            return false;
        }
    }
    return true;
}

// LivePerformanceSystem Implementation
LivePerformanceSystem::LivePerformanceSystem(const std::string& performance_name)
    : current_state_(PerformanceState::IDLE),
      performance_name_(performance_name),
      auto_fallback_enabled_(true),
      emergency_mode_(false),
      target_frame_time_ms_(16.67),  // 60fps target
      total_frames_(0),
      dropped_frames_(0),
      average_frame_time_(0.0),
      max_frame_time_(0.0),
      recovery_in_progress_(false) {
    
    InitializeDefaultThresholds();
    InitializeFallbackActions();
    InitializeEmergencyProcedures();
    
    // Set up performance alert callback
    performance_monitor_.SetAlertCallback(
        [this](PerformanceMetric metric, double value, double threshold) {
            HandlePerformanceAlert(metric, value, threshold);
        });
}

LivePerformanceSystem::~LivePerformanceSystem() {
    if (IsPerformanceActive()) {
        EndPerformance();
    }
}

bool LivePerformanceSystem::Initialize() {
    if (current_state_ != PerformanceState::IDLE) {
        return false;
    }
    
    try {
        performance_monitor_.StartMonitoring();
        current_state_ = PerformanceState::WARMUP;
        
        LogPerformanceEvent("Performance system initialized");
        return true;
    } catch (const std::exception& e) {
        LogPerformanceEvent("Failed to initialize: " + std::string(e.what()));
        return false;
    }
}

bool LivePerformanceSystem::StartPerformance() {
    if (current_state_ != PerformanceState::WARMUP) {
        return false;
    }
    
    performance_start_time_ = std::chrono::steady_clock::now();
    current_state_ = PerformanceState::LIVE;
    
    LogPerformanceEvent("Performance started: " + performance_name_);
    return true;
}

bool LivePerformanceSystem::EndPerformance() {
    if (!IsPerformanceActive()) {
        return false;
    }
    
    current_state_ = PerformanceState::SHUTDOWN;
    performance_monitor_.StopMonitoring();
    
    LogPerformanceEvent("Performance ended: " + performance_name_);
    return true;
}

void LivePerformanceSystem::EmergencyShutdown() {
    current_state_ = PerformanceState::EMERGENCY;
    emergency_mode_ = true;
    
    // Execute emergency procedures
    fallback_manager_.ExecuteEmergencyProcedure("emergency_shutdown");
    
    LogPerformanceEvent("EMERGENCY SHUTDOWN ACTIVATED");
}

void LivePerformanceSystem::Update() {
    if (!IsPerformanceActive()) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    
    // Check for emergency situations
    if (performance_monitor_.IsEmergencySituation() && !emergency_mode_) {
        EmergencyShutdown();
        return;
    }
    
    // Check for critical performance
    if (performance_monitor_.IsCriticalPerformance() && auto_fallback_enabled_) {
        PerformAutomaticFallback();
    }
    
    // Update performance tracking
    if (last_frame_time_.time_since_epoch().count() > 0) {
        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(now - last_frame_time_).count() / 1000.0;
        
        total_frames_++;
        if (frame_duration > target_frame_time_ms_ * 1.5) {
            dropped_frames_++;
        }
        
        // Update running averages
        average_frame_time_ = (average_frame_time_ * (total_frames_ - 1) + frame_duration) / total_frames_;
        max_frame_time_ = std::max(max_frame_time_, frame_duration);
    }
    
    last_frame_time_ = now;
}

void LivePerformanceSystem::ProcessFrame() {
    Update();
    
    // Record frame metrics
    auto snapshot = performance_monitor_.GetCurrentSnapshot();
    
    // Additional frame processing could go here
}

PerformanceState LivePerformanceSystem::GetCurrentState() const {
    return current_state_;
}

std::string LivePerformanceSystem::GetStateDescription() const {
    return PerformanceStateToString(current_state_);
}

bool LivePerformanceSystem::IsPerformanceActive() const {
    return current_state_ == PerformanceState::LIVE || 
           current_state_ == PerformanceState::DEGRADED ||
           current_state_ == PerformanceState::RECOVERY;
}

void LivePerformanceSystem::SetTargetFrameTime(double milliseconds) {
    target_frame_time_ms_ = milliseconds;
}

void LivePerformanceSystem::SetPerformanceName(const std::string& name) {
    performance_name_ = name;
}

void LivePerformanceSystem::EnableAutoFallback(bool enabled) {
    auto_fallback_enabled_ = enabled;
}

void LivePerformanceSystem::SetEmergencyMode(bool emergency) {
    emergency_mode_ = emergency;
    if (emergency) {
        current_state_ = PerformanceState::EMERGENCY;
    }
}

std::string LivePerformanceSystem::GetPerformanceReport() const {
    std::stringstream ss;
    ss << "=== Live Performance Report ===\n";
    ss << "Performance: " << performance_name_ << "\n";
    ss << "State: " << GetStateDescription() << "\n";
    ss << "Duration: " << GetElapsedTime() << " seconds\n";
    ss << "Total Frames: " << total_frames_ << "\n";
    ss << "Dropped Frames: " << dropped_frames_ << " (" 
       << (total_frames_ > 0 ? (dropped_frames_ * 100.0 / total_frames_) : 0.0) << "%\n";
    ss << "Average Frame Time: " << average_frame_time_ << "ms\n";
    ss << "Max Frame Time: " << max_frame_time_ << "ms\n";
    ss << "Target Frame Time: " << target_frame_time_ms_ << "ms\n";
    ss << "\nFallback Status:\n";
    ss << fallback_manager_.GetFallbackStatus();
    
    return ss.str();
}

std::vector<std::string> LivePerformanceSystem::GetRecommendations() const {
    std::vector<std::string> recommendations;
    
    if (average_frame_time_ > target_frame_time_ms_ * 1.2) {
        recommendations.push_back("Consider reducing visual complexity or enabling fallback modes");
    }
    
    if (dropped_frames_ > total_frames_ * 0.05) {  // More than 5% dropped
        recommendations.push_back("High frame drop rate detected - investigate performance bottlenecks");
    }
    
    if (max_frame_time_ > target_frame_time_ms_ * 2.0) {
        recommendations.push_back("Frame time spikes detected - check for resource contention");
    }
    
    return recommendations;
}

void LivePerformanceSystem::LogPerformanceEvent(const std::string& event) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - performance_start_time_).count();
    
    std::stringstream ss;
    ss << "[" << std::setw(8) << std::setfill('0') << elapsed << "ms] " << event;
    
    performance_log_.push_back(ss.str());
    
    // Keep only last 1000 log entries
    if (performance_log_.size() > 1000) {
        performance_log_.erase(performance_log_.begin());
    }
}

void LivePerformanceSystem::InitializeDefaultThresholds() {
    // Frame time thresholds for 60fps (16.67ms target)
    performance_monitor_.AddThreshold({
        PerformanceMetric::FRAME_TIME,
        18.0,   // Warning: 18ms (55fps)
        20.0,   // Critical: 20ms (50fps)
        25.0,   // Emergency: 25ms (40fps)
        "Frame time exceeds target"
    });
    
    // Render time thresholds
    performance_monitor_.AddThreshold({
        PerformanceMetric::RENDER_TIME,
        12.0,   // Warning: 12ms
        15.0,   // Critical: 15ms
        20.0,   // Emergency: 20ms
        "Render time too high"
    });
    
    // GPU utilization thresholds
    performance_monitor_.AddThreshold({
        PerformanceMetric::GPU_UTILIZATION,
        80.0,   // Warning: 80%
        90.0,   // Critical: 90%
        95.0,   // Emergency: 95%
        "GPU utilization critical"
    });
    
    // CPU utilization thresholds
    performance_monitor_.AddThreshold({
        PerformanceMetric::CPU_UTILIZATION,
        70.0,   // Warning: 70%
        85.0,   // Critical: 85%
        95.0,   // Emergency: 95%
        "CPU utilization critical"
    });
    
    // Memory usage thresholds
    performance_monitor_.AddThreshold({
        PerformanceMetric::MEMORY_USAGE,
        75.0,   // Warning: 75%
        85.0,   // Critical: 85%
        95.0,   // Emergency: 95%
        "Memory usage critical"
    });
    
    // Draw calls thresholds
    performance_monitor_.AddThreshold({
        PerformanceMetric::DRAW_CALLS,
        500.0,  // Warning: 500 draw calls
        700.0,  // Critical: 700 draw calls
        1000.0, // Emergency: 1000 draw calls
        "Too many draw calls"
    });
}

void LivePerformanceSystem::InitializeFallbackActions() {
    // Level 1: Optimization fallback
    fallback_manager_.RegisterFallbackAction({
        FallbackLevel::LEVEL_1_OPTIMIZATION,
        "Reduce shader quality and simplify lighting",
        []() {
            std::cout << "Activating Level 1 optimization fallback" << std::endl;
            // Implementation would reduce shader complexity
        },
        true,  // Reversible
        "Restore full shader quality"
    });
    
    // Level 2: Simplification fallback
    fallback_manager_.RegisterFallbackAction({
        FallbackLevel::LEVEL_2_SIMPLIFICATION,
        "Disable complex effects and reduce texture resolution",
        []() {
            std::cout << "Activating Level 2 simplification fallback" << std::endl;
            // Implementation would disable complex effects
        },
        true,  // Reversible
        "Restore complex effects"
    });
    
    // Level 3: Minimal fallback
    fallback_manager_.RegisterFallbackAction({
        FallbackLevel::LEVEL_3_MINIMAL,
        "Switch to minimal visual mode with basic ASCII rendering",
        []() {
            std::cout << "Activating Level 3 minimal fallback" << std::endl;
            // Implementation would switch to minimal rendering
        },
        true,  // Reversible
        "Restore full visual mode"
    });
    
    // Level 4: Emergency fallback
    fallback_manager_.RegisterFallbackAction({
        FallbackLevel::LEVEL_4_EMERGENCY,
        "Emergency mode: static visuals only",
        []() {
            std::cout << "Activating Level 4 emergency fallback" << std::endl;
            // Implementation would switch to static visuals
        },
        false, // Not easily reversible
        "Manual recovery required"
    });
    
    // Level 5: Critical fallback
    fallback_manager_.RegisterFallbackAction({
        FallbackLevel::LEVEL_5_CRITICAL,
        "Critical shutdown: display error message",
        []() {
            std::cout << "Activating Level 5 critical shutdown" << std::endl;
            // Implementation would show error and shutdown
        },
        false, // Not reversible
        "System restart required"
    });
}

void LivePerformanceSystem::InitializeEmergencyProcedures() {
    EmergencyProcedure emergency_shutdown;
    emergency_shutdown.name = "emergency_shutdown";
    emergency_shutdown.execution_time = std::chrono::milliseconds(100);
    emergency_shutdown.auto_recover = false;
    
    emergency_shutdown.actions.push_back({
        FallbackLevel::LEVEL_4_EMERGENCY,
        "Immediate performance degradation",
        []() {
            std::cout << "EMERGENCY: Immediate degradation activated" << std::endl;
        },
        false,
        "Manual intervention required"
    });
    
    fallback_manager_.RegisterEmergencyProcedure(emergency_shutdown);
}

void LivePerformanceSystem::HandlePerformanceAlert(PerformanceMetric metric, double value, double threshold) {
    std::string metric_name = PerformanceMetricToString(metric);
    std::stringstream ss;
    ss << "PERFORMANCE ALERT: " << metric_name << " = " << value 
       << " (threshold: " << threshold << ")";
    
    LogPerformanceEvent(ss.str());
}

void LivePerformanceSystem::HandleStateTransition(PerformanceState new_state) {
    if (current_state_ != new_state) {
        PerformanceState old_state = current_state_;
        current_state_ = new_state;
        
        std::stringstream ss;
        ss << "State transition: " << PerformanceStateToString(old_state) 
           << " -> " << PerformanceStateToString(new_state);
        
        LogPerformanceEvent(ss.str());
    }
}

void LivePerformanceSystem::PerformAutomaticFallback() {
    if (!auto_fallback_enabled_) {
        return;
    }
    
    auto snapshot = performance_monitor_.GetCurrentSnapshot();
    double frame_time = snapshot.metrics.at(PerformanceMetric::FRAME_TIME);
    
    // Determine appropriate fallback level based on frame time
    if (frame_time > 25.0) {
        fallback_manager_.ActivateFallback(FallbackLevel::LEVEL_4_EMERGENCY, "Frame time critical");
        HandleStateTransition(PerformanceState::EMERGENCY);
    } else if (frame_time > 20.0) {
        fallback_manager_.ActivateFallback(FallbackLevel::LEVEL_3_MINIMAL, "Frame time high");
        HandleStateTransition(PerformanceState::DEGRADED);
    } else if (frame_time > 18.0) {
        fallback_manager_.ActivateFallback(FallbackLevel::LEVEL_2_SIMPLIFICATION, "Frame time elevated");
        HandleStateTransition(PerformanceState::DEGRADED);
    } else if (frame_time > 16.0) {
        fallback_manager_.ActivateFallback(FallbackLevel::LEVEL_1_OPTIMIZATION, "Frame time above target");
    }
}

double LivePerformanceSystem::GetElapsedTime() const {
    if (performance_start_time_.time_since_epoch().count() == 0) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - performance_start_time_).count();
}

// Utility Functions
std::string PerformanceMetricToString(PerformanceMetric metric) {
    switch (metric) {
        case PerformanceMetric::FRAME_TIME: return "Frame Time";
        case PerformanceMetric::RENDER_TIME: return "Render Time";
        case PerformanceMetric::AUDIO_LATENCY: return "Audio Latency";
        case PerformanceMetric::GPU_UTILIZATION: return "GPU Utilization";
        case PerformanceMetric::CPU_UTILIZATION: return "CPU Utilization";
        case PerformanceMetric::MEMORY_USAGE: return "Memory Usage";
        case PerformanceMetric::DRAW_CALLS: return "Draw Calls";
        case PerformanceMetric::SHADER_SWITCHES: return "Shader Switches";
        case PerformanceMetric::TEXTURE_BINDS: return "Texture Binds";
        case PerformanceMetric::BUFFER_UPLOADS: return "Buffer Uploads";
        default: return "Unknown";
    }
}

std::string PerformanceStateToString(PerformanceState state) {
    switch (state) {
        case PerformanceState::IDLE: return "IDLE";
        case PerformanceState::WARMUP: return "WARMUP";
        case PerformanceState::LIVE: return "LIVE";
        case PerformanceState::DEGRADED: return "DEGRADED";
        case PerformanceState::EMERGENCY: return "EMERGENCY";
        case PerformanceState::RECOVERY: return "RECOVERY";
        case PerformanceState::SHUTDOWN: return "SHUTDOWN";
        default: return "UNKNOWN";
    }
}

std::string FallbackLevelToString(FallbackLevel level) {
    switch (level) {
        case FallbackLevel::NONE: return "None";
        case FallbackLevel::LEVEL_1_OPTIMIZATION: return "Level 1 - Optimization";
        case FallbackLevel::LEVEL_2_SIMPLIFICATION: return "Level 2 - Simplification";
        case FallbackLevel::LEVEL_3_MINIMAL: return "Level 3 - Minimal";
        case FallbackLevel::LEVEL_4_EMERGENCY: return "Level 4 - Emergency";
        case FallbackLevel::LEVEL_5_CRITICAL: return "Level 5 - Critical";
        default: return "Unknown";
    }
}

double GetRecommendedThreshold(PerformanceMetric metric, const std::string& scenario) {
    if (scenario == "live_vj") {
        switch (metric) {
            case PerformanceMetric::FRAME_TIME: return 16.67;  // 60fps
            case PerformanceMetric::RENDER_TIME: return 12.0;
            case PerformanceMetric::AUDIO_LATENCY: return 10.0;
            case PerformanceMetric::GPU_UTILIZATION: return 80.0;
            case PerformanceMetric::CPU_UTILIZATION: return 70.0;
            case PerformanceMetric::MEMORY_USAGE: return 75.0;
            case PerformanceMetric::DRAW_CALLS: return 500.0;
            case PerformanceMetric::SHADER_SWITCHES: return 100.0;
            case PerformanceMetric::TEXTURE_BINDS: return 200.0;
            case PerformanceMetric::BUFFER_UPLOADS: return 50.0;
            default: return 0.0;
        }
    }
    
    // Default thresholds
    return GetRecommendedThreshold(metric, "live_vj");
}

} // namespace Production
} // namespace NeonGlyph