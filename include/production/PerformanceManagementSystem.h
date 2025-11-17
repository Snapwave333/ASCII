#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

namespace NeonGlyph {
namespace Production {

struct PerformanceMetrics {
    double frame_rate;
    double latency_ms;
    size_t memory_usage_mb;
    double cpu_usage_percent;
    size_t gpu_memory_usage_mb;
    int draw_calls;
    int texture_switches;
    int shader_switches;
    std::chrono::steady_clock::time_point timestamp;
};

struct PerformanceThresholds {
    double min_frame_rate;
    double max_latency_ms;
    size_t max_memory_usage_mb;
    double max_cpu_usage_percent;
    size_t max_gpu_memory_usage_mb;
    int max_draw_calls;
    int max_texture_switches;
    int max_shader_switches;
};

struct BenchmarkResult {
    std::string test_name;
    PerformanceMetrics average_metrics;
    PerformanceMetrics peak_metrics;
    PerformanceMetrics minimum_metrics;
    std::chrono::seconds duration;
    std::string status; // "PASS", "FAIL", "WARNING"
    std::vector<std::string> issues_found;
    std::vector<std::string> recommendations;
};

class PerformanceMonitor {
private:
    std::atomic<bool> monitoring_active_;
    std::thread monitoring_thread_;
    mutable std::mutex metrics_mutex_;
    std::vector<PerformanceMetrics> metrics_history_;
    PerformanceThresholds thresholds_;
    std::chrono::milliseconds sampling_interval_;
    
    // Current metrics
    PerformanceMetrics current_metrics_;
    std::map<std::string, std::vector<std::chrono::microseconds>> span_history_;
    mutable std::mutex span_mutex_;
    
    // Callbacks for threshold violations
    std::vector<std::function<void(const PerformanceMetrics&, const std::string&)>> 
        threshold_callbacks_;
    
public:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    // Start/stop monitoring
    void StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const { return monitoring_active_; }
    
    // Update current metrics
    void UpdateFrameRate(double fps);
    void UpdateLatency(double latency_ms);
    void UpdateMemoryUsage(size_t memory_mb);
    void UpdateCPUUsage(double cpu_percent);
    void UpdateGPUMemoryUsage(size_t gpu_memory_mb);
    void UpdateDrawCalls(int draw_calls);
    void UpdateTextureSwitches(int texture_switches);
    void UpdateShaderSwitches(int shader_switches);
    
    // Get current metrics
    PerformanceMetrics GetCurrentMetrics() const;
    
    // Get metrics history
    std::vector<PerformanceMetrics> GetMetricsHistory() const;
    
    // Set performance thresholds
    void SetThresholds(const PerformanceThresholds& thresholds);
    PerformanceThresholds GetThresholds() const { return thresholds_; }
    
    // Set sampling interval
    void SetSamplingInterval(std::chrono::milliseconds interval) {
        sampling_interval_ = interval;
    }
    
    // Threshold violation callbacks
    void RegisterThresholdCallback(
        const std::function<void(const PerformanceMetrics&, const std::string&)>& callback);
    
    // Check if performance is acceptable
    bool IsPerformanceAcceptable() const;
    std::vector<std::string> GetPerformanceIssues() const;
    void StartSpan(const std::string& name);
    void EndSpan(const std::string& name);
    std::map<std::string, std::tuple<double,double,double>> GetPercentiles() const;
    
private:
    void MonitoringLoop();
    void CheckThresholds(const PerformanceMetrics& metrics);
    void StoreMetrics(const PerformanceMetrics& metrics);
    static double Percentile(const std::vector<std::chrono::microseconds>& v, double p);
};

class AutomatedBenchmark {
private:
    std::string benchmark_name_;
    std::chrono::seconds duration_;
    PerformanceMonitor* monitor_;
    std::vector<std::function<void()>> test_scenarios_;
    
public:
    AutomatedBenchmark(const std::string& name, 
                      std::chrono::seconds duration,
                      PerformanceMonitor* monitor);
    
    // Add test scenario
    void AddTestScenario(const std::function<void()>& scenario);
    
    // Run benchmark
    BenchmarkResult RunBenchmark();
    
    // Run specific test
    BenchmarkResult RunSpecificTest(const std::string& test_name);
    
    // Generate benchmark report
    std::string GenerateBenchmarkReport(const BenchmarkResult& result);
    
    // Compare with baseline
    bool CompareWithBaseline(const BenchmarkResult& current, 
                           const BenchmarkResult& baseline);
};

class PerformanceProfiler {
private:
    std::string profiling_session_name_;
    std::chrono::steady_clock::time_point session_start_;
    std::map<std::string, std::chrono::microseconds> function_timings_;
    mutable std::mutex timing_mutex_;
    
public:
    PerformanceProfiler(const std::string& session_name);
    ~PerformanceProfiler();
    
    // Function profiling
    class FunctionProfiler {
    private:
        PerformanceProfiler* profiler_;
        std::string function_name_;
        std::chrono::steady_clock::time_point start_time_;
        
    public:
        FunctionProfiler(PerformanceProfiler* profiler, const std::string& function_name);
        ~FunctionProfiler();
    };
    
    // Start profiling a function
    FunctionProfiler ProfileFunction(const std::string& function_name);
    
    // Record function timing
    void RecordFunctionTiming(const std::string& function_name, 
                             std::chrono::microseconds duration);
    
    // Get profiling results
    std::map<std::string, std::chrono::microseconds> GetFunctionTimings() const;
    
    // Get profiling report
    std::string GenerateProfilingReport() const;
    
    // Identify bottlenecks
    std::vector<std::pair<std::string, std::chrono::microseconds>> 
        IdentifyBottlenecks(int top_n = 10) const;
};

class PerformanceOptimization {
private:
    PerformanceMonitor* monitor_;
    PerformanceProfiler* profiler_;
    std::vector<std::string> optimization_suggestions_;
    
public:
    PerformanceOptimization(PerformanceMonitor* monitor, PerformanceProfiler* profiler);
    
    // Analyze performance and generate suggestions
    void AnalyzePerformance();
    
    // Get optimization suggestions
    std::vector<std::string> GetOptimizationSuggestions() const;
    
    // Apply optimization
    bool ApplyOptimization(const std::string& optimization_name);
    
    // Generate optimization report
    std::string GenerateOptimizationReport();
    
    // Specific optimization strategies
    void SuggestGPUOptimizations();
    void SuggestCPUOptimizations();
    void SuggestMemoryOptimizations();
    void SuggestRenderingOptimizations();
};

class PerformanceDashboard {
private:
    PerformanceMonitor* monitor_;
    std::vector<BenchmarkResult> benchmark_history_;
    std::mutex dashboard_mutex_;
    
public:
    PerformanceDashboard(PerformanceMonitor* monitor);
    
    // Update dashboard with current metrics
    void UpdateDashboard();
    
    // Add benchmark result
    void AddBenchmarkResult(const BenchmarkResult& result);
    
    // Get current performance summary
    std::string GetPerformanceSummary() const;
    
    // Get performance trends
    std::vector<std::pair<std::chrono::system_clock::time_point, double>> 
        GetPerformanceTrend(const std::string& metric_name) const;
    
    // Generate dashboard report
    std::string GenerateDashboardReport() const;
    
    // Export data for external analysis
    void ExportPerformanceData(const std::string& filename) const;
};

// Predefined benchmark suites
namespace StandardBenchmarkSuites {
    // Basic performance benchmark
    std::vector<std::function<void()>> GetBasicPerformanceSuite();
    
    // Stress test benchmark
    std::vector<std::function<void()>> GetStressTestSuite();
    
    // Memory usage benchmark
    std::vector<std::function<void()>> GetMemoryBenchmarkSuite();
    
    // Real-world scenario benchmark
    std::vector<std::function<void()>> GetRealWorldScenarioSuite();
    
    // Comprehensive benchmark suite
    std::vector<std::function<void()>> GetComprehensiveBenchmarkSuite();
}

// Performance thresholds for different quality levels
namespace QualityThresholds {
    const PerformanceThresholds MINIMUM_QUALITY {
        30.0,   // min_frame_rate
        32.0,   // max_latency_ms
        4096,   // max_memory_usage_mb
        25.0,   // max_cpu_usage_percent
        6144,   // max_gpu_memory_usage_mb
        1200,   // max_draw_calls
        200,    // max_texture_switches
        100     // max_shader_switches
    };
    
    const PerformanceThresholds TARGET_QUALITY {
        60.0,   // min_frame_rate
        16.0,   // max_latency_ms
        2048,   // max_memory_usage_mb
        5.0,    // max_cpu_usage_percent
        4096,   // max_gpu_memory_usage_mb
        800,    // max_draw_calls
        100,    // max_texture_switches
        50      // max_shader_switches
    };
    
    const PerformanceThresholds OPTIMAL_QUALITY {
        144.0,  // min_frame_rate
        8.0,    // max_latency_ms
        1024,   // max_memory_usage_mb
        2.0,    // max_cpu_usage_percent
        2048,   // max_gpu_memory_usage_mb
        400,    // max_draw_calls
        50,     // max_texture_switches
        25      // max_shader_switches
    };
}

} // namespace Production
} // namespace NeonGlyph