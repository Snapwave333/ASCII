#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <map>
#include <queue>
#include <future>
#include "concurrency/WorkStealingThreadPool.h"

namespace NeonGlyph {
namespace Chaos {

enum class LoadType {
    CPU_INTENSIVE,
    MEMORY_INTENSIVE,
    IO_INTENSIVE,
    NETWORK_INTENSIVE,
    GRAPHICS_INTENSIVE,
    MIXED_WORKLOAD
};

enum class DegradationPattern {
    LINEAR,
    EXPONENTIAL,
    STEP_FUNCTION,
    SINE_WAVE,
    RANDOM_WALK,
    SPIKE_AND_RECOVER,
    GRADUAL_DECLINE,
    SUDDEN_DROP
};

enum class PerformanceMetric {
    RESPONSE_TIME,
    THROUGHPUT,
    CPU_USAGE,
    MEMORY_USAGE,
    GPU_USAGE,
    NETWORK_BANDWIDTH,
    DISK_IO_RATE,
    ERROR_RATE,
    SATURATION_LEVEL
};

struct LoadProfile {
    LoadType type;
    double intensity; // 0.0 to 1.0
    std::chrono::seconds duration;
    std::chrono::seconds rampUpTime;
    std::chrono::seconds steadyStateTime;
    std::chrono::seconds rampDownTime;
    size_t concurrentUsers;
    size_t requestsPerSecond;
    std::string targetComponent;
};

struct PerformanceTarget {
    PerformanceMetric metric;
    double targetValue;
    double acceptableThreshold; // Percentage deviation allowed
    std::chrono::seconds measurementWindow;
    bool isUpperBound; // true if target is maximum allowed, false if minimum required
};

struct DegradationScenario {
    std::string name;
    std::string description;
    DegradationPattern pattern;
    double degradationRate; // Percentage per time unit
    std::chrono::seconds duration;
    std::map<PerformanceMetric, double> impactFactors; // Metric -> impact factor
    std::vector<std::string> affectedComponents;
    bool recoverable;
    std::chrono::seconds recoveryTime;
};

struct LoadTestResult {
    std::string testId;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    LoadProfile profile;
    std::map<PerformanceMetric, std::vector<double>> metrics; // Metric -> time series data
    std::map<std::string, double> componentMetrics; // Component-specific metrics
    double averageResponseTime;
    double peakResponseTime;
    double throughput;
    double errorRate;
    bool passed;
    std::vector<std::string> failures;
    std::string summaryReport;
    double cacheHitRatio;
    double iocpDispersionScore;
    double queueBalanceScore;
    int breakerEvents;
    std::vector<std::string> validationIssues;
};

struct PerformanceSnapshot {
    std::chrono::system_clock::time_point timestamp;
    std::map<PerformanceMetric, double> metrics;
    std::map<std::string, double> componentMetrics;
    std::string systemState;
    size_t activeConnections;
    size_t queueLength;
    double cpuUsage;
    double memoryUsage;
    double gpuUsage;
};

class LoadGenerator {
public:
    LoadGenerator();
    ~LoadGenerator();

    void SetLoadProfile(const LoadProfile& profile);
    void SetNetworkEndpoint(const std::string& endpoint);
    void StartGeneration();
    void StopGeneration();
    bool IsGenerating() const;
    
    void GenerateCPUIntensiveLoad();
    void GenerateMemoryIntensiveLoad();
    void GenerateIOIntensiveLoad();
    void GenerateNetworkIntensiveLoad();
    void GenerateGraphicsIntensiveLoad();
    void GenerateMixedWorkload();

    size_t GetActiveThreads() const;
    double GetCurrentLoadIntensity() const;
    PerformanceSnapshot GetCurrentSnapshot() const;
    std::vector<int> GetExecutorQueueDepths() const;

private:
    LoadProfile m_profile;
    std::atomic<bool> m_isGenerating;
    std::vector<std::thread> m_workerThreads;
    std::atomic<size_t> m_activeThreads;
    std::mutex m_snapshotMutex;
    PerformanceSnapshot m_currentSnapshot;
    std::wstring m_host;
    unsigned short m_port{0};
    std::wstring m_path;
    bool m_useHttps{false};
    std::atomic<uint64_t> m_requestsCompleted{0};
    std::atomic<uint64_t> m_requestsFailed{0};
    std::atomic<uint64_t> m_bytesTransferred{0};
    std::atomic<int64_t> m_lastResponseMicros{0};
    std::unique_ptr<NeonGlyph::Concurrency::WorkStealingThreadPool> m_pool;
    
    void WorkerThread();
    void CPUIntensiveTask();
    void MemoryIntensiveTask();
    void IOIntensiveTask();
    void NetworkIntensiveTask();
    void GraphicsIntensiveTask();
    void UpdateSnapshot();
};

class PerformanceDegradationSimulator {
public:
    PerformanceDegradationSimulator();
    ~PerformanceDegradationSimulator();

    void SetDegradationScenario(const DegradationScenario& scenario);
    void StartDegradation();
    void StopDegradation();
    bool IsDegrading() const;
    
    void ApplyLinearDegradation();
    void ApplyExponentialDegradation();
    void ApplyStepFunctionDegradation();
    void ApplySineWaveDegradation();
    void ApplyRandomWalkDegradation();
    void ApplySpikeAndRecoverDegradation();
    void ApplyGradualDeclineDegradation();
    void ApplySuddenDropDegradation();

    PerformanceSnapshot GetCurrentPerformance() const;
    std::vector<PerformanceSnapshot> GetPerformanceHistory() const;
    double GetDegradationLevel() const;

private:
    DegradationScenario m_scenario;
    std::atomic<bool> m_isDegrading;
    std::thread m_degradationThread;
    std::mutex m_historyMutex;
    std::vector<PerformanceSnapshot> m_performanceHistory;
    std::atomic<double> m_degradationLevel;
    std::chrono::system_clock::time_point m_startTime;
    
    void DegradationThread();
    void UpdatePerformanceMetrics(double degradationFactor);
    double CalculateDegradationFactor(std::chrono::seconds elapsedTime);
    void RecordSnapshot();
};

class LoadTestingOrchestrator {
public:
    LoadTestingOrchestrator();
    ~LoadTestingOrchestrator();

    void AddLoadProfile(const std::string& name, const LoadProfile& profile);
    void AddDegradationScenario(const std::string& name, const DegradationScenario& scenario);
    void AddPerformanceTarget(const PerformanceTarget& target);
    
    std::string RunLoadTest(const std::string& profileName, const std::vector<std::string>& degradationScenarios = {});
    std::string RunStressTest(const std::string& profileName, size_t durationMinutes);
    std::string RunSpikeTest(const std::string& profileName, double spikeMultiplier, std::chrono::seconds spikeDuration);
    std::string RunEnduranceTest(const std::string& profileName, size_t durationHours);
    std::string RunScalabilityTest(const std::vector<std::string>& profileNames);
    
    LoadTestResult GetTestResult(const std::string& testId) const;
    std::vector<std::string> GetAllTestIds() const;
    std::vector<PerformanceSnapshot> GetPerformanceHistory(const std::string& testId) const;
    
    void GenerateReport(const std::string& testId, const std::string& format = "json");
    void GenerateComparisonReport(const std::vector<std::string>& testIds, const std::string& format = "json");
    
    bool IsTestRunning() const;
    void StopAllTests();

private:
    std::map<std::string, LoadProfile> m_loadProfiles;
    std::map<std::string, DegradationScenario> m_degradationScenarios;
    std::vector<PerformanceTarget> m_performanceTargets;
    std::map<std::string, LoadTestResult> m_testResults;
    std::unique_ptr<LoadGenerator> m_loadGenerator;
    std::unique_ptr<PerformanceDegradationSimulator> m_degradationSimulator;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_shouldStop;
    std::mutex m_resultsMutex;
    
    std::string GenerateTestId();
    bool ValidatePerformanceTargets(const LoadTestResult& result);
    void RecordPerformanceSnapshot(const std::string& testId);
    void GenerateDetailedReport(const LoadTestResult& result, const std::string& format);
    void ExecuteLoadTest(const std::string& profileName, const std::string& testId);
    void ExecuteStressTest(const std::string& profileName, size_t durationMinutes, const std::string& testId);
    void RunValidations(LoadTestResult& result);
};

class PerformanceAnalyzer {
public:
    PerformanceAnalyzer();
    ~PerformanceAnalyzer();

    void AddPerformanceData(const std::string& testId, const std::vector<PerformanceSnapshot>& snapshots);
    void AnalyzeTrends(const std::string& testId);
    void IdentifyBottlenecks(const std::string& testId);
    void CalculateScalabilityMetrics(const std::string& testId);
    void PredictPerformance(const std::string& testId, std::chrono::seconds futureTime);
    
    std::string GeneratePerformanceInsights(const std::string& testId);
    std::string GenerateOptimizationRecommendations(const std::string& testId);
    std::string GenerateCapacityPlanningReport(const std::string& testId);
    
    std::map<std::string, double> GetBottleneckAnalysis(const std::string& testId) const;
    std::map<std::string, double> GetScalabilityMetrics(const std::string& testId) const;
    std::vector<std::string> GetOptimizationSuggestions(const std::string& testId) const;

private:
    std::map<std::string, std::vector<PerformanceSnapshot>> m_testData;
    std::map<std::string, std::map<std::string, double>> m_bottleneckAnalysis;
    std::map<std::string, std::map<std::string, double>> m_scalabilityMetrics;
    std::map<std::string, std::vector<std::string>> m_optimizationSuggestions;
    std::mutex m_dataMutex;
    
    double CalculateTrend(const std::vector<double>& values);
    double CalculateVolatility(const std::vector<double>& values);
    std::string IdentifyComponentBottleneck(const std::vector<PerformanceSnapshot>& snapshots);
    double CalculateResponseTimePercentile(const std::vector<double>& responseTimes, double percentile);
    double CalculateThroughputTrend(const std::vector<PerformanceSnapshot>& snapshots);
};

    class NeonGlyphLoadTester {
public:
    NeonGlyphLoadTester();
    ~NeonGlyphLoadTester();

    void ConfigureForNeonGlyph();
    void TestVulkanRenderingLoad(const LoadProfile& profile);
    void TestAudioEngineLoad(const LoadProfile& profile);
    void TestASCIIProcessingLoad(const LoadProfile& profile);
    void TestAIComponentLoad(const LoadProfile& profile);
    void TestHeadlessModeLoad(const LoadProfile& profile);
    
    void SimulateNeonGlyphDegradation(const DegradationScenario& scenario);
    void TestNeonGlyphRecovery(const std::string& component);
    
    LoadTestResult GetNeonGlyphSpecificResults() const;
    std::string GenerateNeonGlyphPerformanceReport() const;

private:
    std::unique_ptr<LoadTestingOrchestrator> m_orchestrator;
    std::unique_ptr<PerformanceAnalyzer> m_analyzer;
    std::map<std::string, LoadTestResult> m_neonGlyphResults;
    std::mutex m_resultsMutex;
    
    void ConfigureNeonGlyphLoadProfiles();
    void ConfigureNeonGlyphDegradationScenarios();
    void MonitorNeonGlyphSpecificMetrics();
};

} // namespace Chaos
} // namespace NeonGlyph