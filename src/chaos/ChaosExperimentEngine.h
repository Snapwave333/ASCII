#pragma once

#include "FailureModeAnalysis.h"
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <random>

namespace NeonGlyph {
namespace Chaos {

enum class ExperimentState {
    IDLE,
    RUNNING,
    PAUSED,
    COMPLETED,
    FAILED,
    ABORTED
};

enum class InjectionStrategy {
    RANDOM,
    SEQUENTIAL,
    TARGETED,
    GRADUAL,
    BURST,
    DISTRIBUTED
};

struct ChaosExperiment {
    std::string id;
    std::string name;
    std::string description;
    std::vector<FailureScenario> scenarios;
    InjectionStrategy strategy;
    std::chrono::seconds duration;
    std::chrono::seconds interval;
    double intensity; // 0.0 to 1.0
    bool allowRecovery;
    bool monitorSystemHealth;
    std::map<std::string, std::string> parameters;
};

struct ExperimentResult {
    std::string experimentId;
    ExperimentState finalState;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::vector<std::string> executedScenarios;
    std::vector<std::string> detectedFailures;
    std::vector<std::string> recoveryEvents;
    std::map<std::string, double> systemMetricsBefore;
    std::map<std::string, double> systemMetricsAfter;
    std::string summaryReport;
    bool systemRecovered;
    double maxDegradation;
};

class ChaosExperimentEngine {
public:
    ChaosExperimentEngine(std::shared_ptr<FailureModeAnalysis> failureAnalysis);
    ~ChaosExperimentEngine();

    // Experiment management
    std::string CreateExperiment(const ChaosExperiment& experiment);
    void StartExperiment(const std::string& experimentId);
    void PauseExperiment(const std::string& experimentId);
    void ResumeExperiment(const std::string& experimentId);
    void AbortExperiment(const std::string& experimentId);
    ExperimentResult GetExperimentResult(const std::string& experimentId) const;
    
    // Predefined experiment templates
    std::string CreateNetworkChaosExperiment();
    std::string CreateMemoryPressureExperiment();
    std::string CreateCPUSaturationExperiment();
    std::string CreateGraphicsFailureExperiment();
    std::string CreateAudioDisruptionExperiment();
    std::string CreateAIFailureExperiment();
    std::string CreateComprehensiveSystemStressExperiment();
    
    // Advanced chaos patterns
    std::string CreateCascadingFailureExperiment();
    std::string CreateGradualDegradationExperiment();
    std::string CreateSuddenSpikeExperiment();
    std::string CreateResourceExhaustionExperiment();
    std::string CreateDependencyFailureExperiment();
    
    // Safety and monitoring
    void SetSafetyThresholds(const std::map<std::string, double>& thresholds);
    void EnableAutomaticRecovery(bool enabled);
    void SetMaxConcurrentFailures(int maxFailures);
    bool IsSystemStable() const;
    std::map<std::string, std::string> GetCurrentSystemStatus() const;
    
    // Configuration and reporting
    void LoadExperimentConfiguration(const std::string& configFile);
    void SaveExperimentResults(const std::string& experimentId, const std::string& filename) const;
    std::vector<std::string> GetRunningExperiments() const;
    std::vector<std::string> GetCompletedExperiments() const;
    std::string GenerateChaosReport() const;

private:
    std::shared_ptr<FailureModeAnalysis> m_failureAnalysis;
    std::map<std::string, ChaosExperiment> m_experiments;
    std::map<std::string, ExperimentResult> m_results;
    std::map<std::string, ExperimentState> m_experimentStates;
    std::map<std::string, std::thread> m_experimentThreads;
    std::atomic<bool> m_engineActive;
    
    // Safety controls
    std::map<std::string, double> m_safetyThresholds;
    bool m_automaticRecoveryEnabled;
    int m_maxConcurrentFailures;
    std::atomic<int> m_currentFailureCount;
    
    // Random number generation
    std::random_device m_randomDevice;
    std::mt19937 m_randomGenerator;
    
    // Monitoring and metrics
    mutable std::mutex m_metricsMutex;
    std::map<std::string, double> m_currentMetrics;
    std::chrono::steady_clock::time_point m_engineStartTime;
    
    // Experiment execution
    void ExecuteExperiment(const std::string& experimentId);
    void InjectFailure(const FailureScenario& scenario);
    bool ShouldTriggerRecovery(const std::string& experimentId) const;
    void MonitorExperimentProgress(const std::string& experimentId);
    void RecordExperimentMetrics(const std::string& experimentId);
    
    // Failure injection strategies
    void InjectRandomFailures(const ChaosExperiment& experiment);
    void InjectSequentialFailures(const ChaosExperiment& experiment);
    void InjectTargetedFailures(const ChaosExperiment& experiment);
    void InjectGradualFailures(const ChaosExperiment& experiment);
    void InjectBurstFailures(const ChaosExperiment& experiment);
    void InjectDistributedFailures(const ChaosExperiment& experiment);
    
    // System metrics collection
    std::map<std::string, double> CollectSystemMetrics() const;
    double CalculateSystemStabilityScore() const;
    bool AreSafetyThresholdsExceeded() const;
    
    // Recovery mechanisms
    void TriggerAutomaticRecovery(const std::string& experimentId);
    void PerformGracefulDegradation(const std::string& component);
    void ActivateBackupSystems(const std::string& component);
    
    // NeonGlyph-specific failure injection
    void InjectVulkanFailure();
    void InjectAudioFailure();
    void InjectAIFailure();
    void InjectGraphicsDegradation();
    void InjectMemoryLeak();
    void InjectResourceExhaustion();
    void InjectNetworkPartition();
    void InjectDeadlock();
    void InjectRaceCondition();
};

} // namespace Chaos
} // namespace NeonGlyph