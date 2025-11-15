#include "production/PerformanceManagementSystem.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <iostream>

namespace NeonGlyph {
namespace Production {

// PerformanceMonitor Implementation
PerformanceMonitor::PerformanceMonitor() 
    : monitoring_active_(false), sampling_interval_(std::chrono::milliseconds(100)) {
    // Initialize current metrics
    current_metrics_ = {
        0.0,  // frame_rate
        0.0,  // latency_ms
        0,    // memory_usage_mb
        0.0,  // cpu_usage_percent
        0,    // gpu_memory_usage_mb
        0,    // draw_calls
        0,    // texture_switches
        0,    // shader_switches
        std::chrono::steady_clock::now()
    };
    
    // Set default thresholds
    thresholds_ = QualityThresholds::TARGET_QUALITY;
}

PerformanceMonitor::~PerformanceMonitor() {
    StopMonitoring();
}

void PerformanceMonitor::StartMonitoring() {
    if (!monitoring_active_) {
        monitoring_active_ = true;
        monitoring_thread_ = std::thread(&PerformanceMonitor::MonitoringLoop, this);
    }
}

void PerformanceMonitor::StopMonitoring() {
    monitoring_active_ = false;
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

void PerformanceMonitor::UpdateFrameRate(double fps) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.frame_rate = fps;
}

void PerformanceMonitor::UpdateLatency(double latency_ms) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.latency_ms = latency_ms;
}

void PerformanceMonitor::UpdateMemoryUsage(size_t memory_mb) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.memory_usage_mb = memory_mb;
}

void PerformanceMonitor::UpdateCPUUsage(double cpu_percent) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.cpu_usage_percent = cpu_percent;
}

void PerformanceMonitor::UpdateGPUMemoryUsage(size_t gpu_memory_mb) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.gpu_memory_usage_mb = gpu_memory_mb;
}

void PerformanceMonitor::UpdateDrawCalls(int draw_calls) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.draw_calls = draw_calls;
}

void PerformanceMonitor::UpdateTextureSwitches(int texture_switches) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.texture_switches = texture_switches;
}

void PerformanceMonitor::UpdateShaderSwitches(int shader_switches) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    current_metrics_.shader_switches = shader_switches;
}

PerformanceMetrics PerformanceMonitor::GetCurrentMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

std::vector<PerformanceMetrics> PerformanceMonitor::GetMetricsHistory() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_history_;
}

void PerformanceMonitor::SetThresholds(const PerformanceThresholds& thresholds) {
    thresholds_ = thresholds;
}

void PerformanceMonitor::RegisterThresholdCallback(
    const std::function<void(const PerformanceMetrics&, const std::string&)>& callback) {
    threshold_callbacks_.push_back(callback);
}

bool PerformanceMonitor::IsPerformanceAcceptable() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    return current_metrics_.frame_rate >= thresholds_.min_frame_rate &&
           current_metrics_.latency_ms <= thresholds_.max_latency_ms &&
           current_metrics_.memory_usage_mb <= thresholds_.max_memory_usage_mb &&
           current_metrics_.cpu_usage_percent <= thresholds_.max_cpu_usage_percent &&
           current_metrics_.gpu_memory_usage_mb <= thresholds_.max_gpu_memory_usage_mb &&
           current_metrics_.draw_calls <= thresholds_.max_draw_calls &&
           current_metrics_.texture_switches <= thresholds_.max_texture_switches &&
           current_metrics_.shader_switches <= thresholds_.max_shader_switches;
}

std::vector<std::string> PerformanceMonitor::GetPerformanceIssues() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    std::vector<std::string> issues;
    
    if (current_metrics_.frame_rate < thresholds_.min_frame_rate) {
        issues.push_back("Frame rate below minimum threshold");
    }
    if (current_metrics_.latency_ms > thresholds_.max_latency_ms) {
        issues.push_back("Latency above maximum threshold");
    }
    if (current_metrics_.memory_usage_mb > thresholds_.max_memory_usage_mb) {
        issues.push_back("Memory usage above maximum threshold");
    }
    if (current_metrics_.cpu_usage_percent > thresholds_.max_cpu_usage_percent) {
        issues.push_back("CPU usage above maximum threshold");
    }
    if (current_metrics_.gpu_memory_usage_mb > thresholds_.max_gpu_memory_usage_mb) {
        issues.push_back("GPU memory usage above maximum threshold");
    }
    if (current_metrics_.draw_calls > thresholds_.max_draw_calls) {
        issues.push_back("Draw calls above maximum threshold");
    }
    if (current_metrics_.texture_switches > thresholds_.max_texture_switches) {
        issues.push_back("Texture switches above maximum threshold");
    }
    if (current_metrics_.shader_switches > thresholds_.max_shader_switches) {
        issues.push_back("Shader switches above maximum threshold");
    }
    
    return issues;
}

void PerformanceMonitor::MonitoringLoop() {
    while (monitoring_active_) {
        auto start_time = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            current_metrics_.timestamp = start_time;
            StoreMetrics(current_metrics_);
            CheckThresholds(current_metrics_);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (elapsed < sampling_interval_) {
            std::this_thread::sleep_for(sampling_interval_ - elapsed);
        }
    }
}

void PerformanceMonitor::CheckThresholds(const PerformanceMetrics& metrics) {
    std::vector<std::string> violations;
    
    if (metrics.frame_rate < thresholds_.min_frame_rate) {
        violations.push_back("Frame rate violation");
    }
    if (metrics.latency_ms > thresholds_.max_latency_ms) {
        violations.push_back("Latency violation");
    }
    if (metrics.memory_usage_mb > thresholds_.max_memory_usage_mb) {
        violations.push_back("Memory usage violation");
    }
    if (metrics.cpu_usage_percent > thresholds_.max_cpu_usage_percent) {
        violations.push_back("CPU usage violation");
    }
    if (metrics.gpu_memory_usage_mb > thresholds_.max_gpu_memory_usage_mb) {
        violations.push_back("GPU memory violation");
    }
    
    for (const auto& violation : violations) {
        for (const auto& callback : threshold_callbacks_) {
            callback(metrics, violation);
        }
    }
}

void PerformanceMonitor::StoreMetrics(const PerformanceMetrics& metrics) {
    metrics_history_.push_back(metrics);
    
    // Keep only last 1000 metrics to prevent memory bloat
    if (metrics_history_.size() > 1000) {
        metrics_history_.erase(metrics_history_.begin());
    }
}

// AutomatedBenchmark Implementation
AutomatedBenchmark::AutomatedBenchmark(const std::string& name, 
                                     std::chrono::seconds duration,
                                     PerformanceMonitor* monitor)
    : benchmark_name_(name), duration_(duration), monitor_(monitor) {
}

void AutomatedBenchmark::AddTestScenario(const std::function<void()>& scenario) {
    test_scenarios_.push_back(scenario);
}

BenchmarkResult AutomatedBenchmark::RunBenchmark() {
    BenchmarkResult result;
    result.test_name = benchmark_name_;
    result.duration = duration_;
    
    // Initialize metrics tracking
    std::vector<PerformanceMetrics> metrics_collected;
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + duration_;
    
    std::cout << "Starting benchmark: " << benchmark_name_ << std::endl;
    
    // Run test scenarios
    while (std::chrono::steady_clock::now() < end_time) {
        // Execute test scenarios
        for (const auto& scenario : test_scenarios_) {
            if (scenario) {
                scenario();
            }
        }
        
        // Collect metrics
        auto metrics = monitor_->GetCurrentMetrics();
        metrics_collected.push_back(metrics);
        
        // Small delay to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Calculate result metrics
    if (!metrics_collected.empty()) {
        // Calculate averages
        double total_fps = 0.0, total_latency = 0.0, total_memory = 0.0;
        double total_cpu = 0.0, total_gpu_memory = 0.0;
        int total_draw_calls = 0, total_texture_switches = 0, total_shader_switches = 0;
        
        for (const auto& metrics : metrics_collected) {
            total_fps += metrics.frame_rate;
            total_latency += metrics.latency_ms;
            total_memory += metrics.memory_usage_mb;
            total_cpu += metrics.cpu_usage_percent;
            total_gpu_memory += metrics.gpu_memory_usage_mb;
            total_draw_calls += metrics.draw_calls;
            total_texture_switches += metrics.texture_switches;
            total_shader_switches += metrics.shader_switches;
        }
        
        size_t count = metrics_collected.size();
        result.average_metrics = {
            total_fps / count,
            total_latency / count,
            static_cast<size_t>(total_memory / count),
            total_cpu / count,
            static_cast<size_t>(total_gpu_memory / count),
            total_draw_calls / static_cast<int>(count),
            total_texture_switches / static_cast<int>(count),
            total_shader_switches / static_cast<int>(count),
            std::chrono::steady_clock::now()
        };
        
        // Find peak and minimum metrics
        auto max_element = std::max_element(metrics_collected.begin(), metrics_collected.end(),
            [](const PerformanceMetrics& a, const PerformanceMetrics& b) {
                return a.memory_usage_mb < b.memory_usage_mb;
            });
        result.peak_metrics = *max_element;
        
        auto min_element = std::min_element(metrics_collected.begin(), metrics_collected.end(),
            [](const PerformanceMetrics& a, const PerformanceMetrics& b) {
                return a.frame_rate > b.frame_rate;
            });
        result.minimum_metrics = *min_element;
    }
    
    // Determine status
    result.status = monitor_->IsPerformanceAcceptable() ? "PASS" : "FAIL";
    
    // Generate issues and recommendations
    result.issues_found = monitor_->GetPerformanceIssues();
    
    // Add recommendations based on issues
    for (const auto& issue : result.issues_found) {
        if (issue.find("Frame rate") != std::string::npos) {
            result.recommendations.push_back("Consider optimizing rendering pipeline");
            result.recommendations.push_back("Reduce draw calls or texture switches");
        } else if (issue.find("Memory") != std::string::npos) {
            result.recommendations.push_back("Implement memory pooling or garbage collection");
            result.recommendations.push_back("Optimize texture memory usage");
        } else if (issue.find("CPU") != std::string::npos) {
            result.recommendations.push_back("Profile CPU usage and optimize hot paths");
            result.recommendations.push_back("Consider multi-threading optimizations");
        }
    }
    
    std::cout << "Benchmark completed: " << benchmark_name_ << " - Status: " << result.status << std::endl;
    
    return result;
}

std::string AutomatedBenchmark::GenerateBenchmarkReport(const BenchmarkResult& result) {
    std::ostringstream report;
    
    report << "Benchmark Report: " << result.test_name << "\n";
    report << "=====================================\n";
    report << "Duration: " << result.duration.count() << " seconds\n";
    report << "Status: " << result.status << "\n\n";
    
    report << "Average Metrics:\n";
    report << "  Frame Rate: " << std::fixed << std::setprecision(2) 
           << result.average_metrics.frame_rate << " FPS\n";
    report << "  Latency: " << result.average_metrics.latency_ms << " ms\n";
    report << "  Memory Usage: " << result.average_metrics.memory_usage_mb << " MB\n";
    report << "  CPU Usage: " << std::fixed << std::setprecision(1) 
           << (result.average_metrics.cpu_usage_percent * 100) << "%\n";
    report << "  GPU Memory: " << result.average_metrics.gpu_memory_usage_mb << " MB\n";
    report << "  Draw Calls: " << result.average_metrics.draw_calls << "\n";
    report << "  Texture Switches: " << result.average_metrics.texture_switches << "\n";
    report << "  Shader Switches: " << result.average_metrics.shader_switches << "\n\n";
    
    if (!result.issues_found.empty()) {
        report << "Issues Found:\n";
        for (const auto& issue : result.issues_found) {
            report << "  - " << issue << "\n";
        }
        report << "\n";
    }
    
    if (!result.recommendations.empty()) {
        report << "Recommendations:\n";
        for (const auto& rec : result.recommendations) {
            report << "  - " << rec << "\n";
        }
    }
    
    return report.str();
}

// PerformanceProfiler Implementation
PerformanceProfiler::FunctionProfiler::FunctionProfiler(PerformanceProfiler* profiler, 
                                                     const std::string& function_name)
    : profiler_(profiler), function_name_(function_name), start_time_(std::chrono::steady_clock::now()) {
}

PerformanceProfiler::FunctionProfiler::~FunctionProfiler() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    profiler_->RecordFunctionTiming(function_name_, duration);
}

PerformanceProfiler::PerformanceProfiler(const std::string& session_name)
    : profiling_session_name_(session_name), session_start_(std::chrono::steady_clock::now()) {
}

PerformanceProfiler::~PerformanceProfiler() {
    std::cout << "Profiling session '" << profiling_session_name_ << "' completed" << std::endl;
}

PerformanceProfiler::FunctionProfiler PerformanceProfiler::ProfileFunction(const std::string& function_name) {
    return FunctionProfiler(this, function_name);
}

void PerformanceProfiler::RecordFunctionTiming(const std::string& function_name, 
                                              std::chrono::microseconds duration) {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    function_timings_[function_name] += duration;
}

std::map<std::string, std::chrono::microseconds> PerformanceProfiler::GetFunctionTimings() const {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    return function_timings_;
}

std::string PerformanceProfiler::GenerateProfilingReport() const {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    std::ostringstream report;
    
    report << "Performance Profiling Report: " << profiling_session_name_ << "\n";
    report << "============================================\n";
    
    auto total_time = std::chrono::steady_clock::now() - session_start_;
    report << "Session Duration: " 
           << std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count() 
           << " ms\n\n";
    
    if (!function_timings_.empty()) {
        report << "Function Timings:\n";
        
        // Sort by timing (descending)
        std::vector<std::pair<std::string, std::chrono::microseconds>> sorted_timings(
            function_timings_.begin(), function_timings_.end());
        std::sort(sorted_timings.begin(), sorted_timings.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (const auto& [function_name, timing] : sorted_timings) {
            report << "  " << function_name << ": " << timing.count() << " μs\n";
        }
    } else {
        report << "No function timings recorded.\n";
    }
    
    return report.str();
}

std::vector<std::pair<std::string, std::chrono::microseconds>> 
PerformanceProfiler::IdentifyBottlenecks(int top_n) const {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    
    std::vector<std::pair<std::string, std::chrono::microseconds>> bottlenecks(
        function_timings_.begin(), function_timings_.end());
    
    std::sort(bottlenecks.begin(), bottlenecks.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (bottlenecks.size() > static_cast<size_t>(top_n)) {
        bottlenecks.resize(top_n);
    }
    
    return bottlenecks;
}

} // namespace Production
} // namespace NeonGlyph