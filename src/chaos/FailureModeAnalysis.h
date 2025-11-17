#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>

namespace NeonGlyph {
namespace Chaos {

enum class FailureSeverity {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

enum class FailureType {
    // Infrastructure Failures
    MEMORY_LEAK,
    RESOURCE_EXHAUSTION,
    NETWORK_PARTITION,
    HARDWARE_FAILURE,
    
    // Software Failures
    INFINITE_LOOP,
    DEADLOCK,
    RACE_CONDITION,
    UNHANDLED_EXCEPTION,
    
    // Performance Failures
    PERFORMANCE_DEGRADATION,
    MEMORY_PRESSURE,
    CPU_SATURATION,
    
    // Security Failures
    BUFFER_OVERFLOW,
    INJECTION_ATTACK,
    PRIVILEGE_ESCALATION,
    
    // Data Failures
    DATA_CORRUPTION,
    INCONSISTENT_STATE,
    LOST_TRANSACTION
};

struct FailureImpact {
    FailureSeverity severity;
    std::string description;
    std::vector<std::string> affectedComponents;
    std::chrono::milliseconds recoveryTime;
    bool isRecoverable;
    bool causesCascadingFailure;
};

struct SystemComponent {
    std::string name;
    std::string type;
    std::vector<std::string> dependencies;
    std::vector<std::string> dependents;
    bool isCritical;
    bool hasRedundancy;
    std::chrono::milliseconds maxAcceptableDowntime;
};

struct FailureScenario {
    std::string id;
    std::string name;
    FailureType type;
    FailureSeverity severity;
    std::string targetComponent;
    std::string description;
    std::function<void()> injectionFunction;
    std::function<bool()> detectionFunction;
    std::function<void()> recoveryFunction;
    std::chrono::seconds duration;
    bool requiresManualIntervention;
};

class FailureModeAnalysis {
public:
    FailureModeAnalysis();
    ~FailureModeAnalysis();

    // System mapping and analysis
    void MapSystemComponents();
    void IdentifyFailurePoints();
    void AnalyzeCascadingFailures();
    void AssessRecoveryTimes();
    void EvaluateBusinessImpact();

    // Failure scenario management
    void RegisterFailureScenario(const FailureScenario& scenario);
    std::vector<FailureScenario> GetScenariosForComponent(const std::string& component) const;
    std::vector<FailureScenario> GetScenariosBySeverity(FailureSeverity severity) const;
    std::vector<FailureScenario> GetScenariosByType(FailureType type) const;

    // Risk assessment
    FailureImpact AssessFailureImpact(const std::string& component, FailureType type) const;
    std::vector<std::string> GetSinglePointsOfFailure() const;
    std::map<std::string, double> CalculateRiskScores() const;
    
    // System health monitoring
    void StartHealthMonitoring();
    void StopHealthMonitoring();
    bool IsSystemHealthy() const;
    std::map<std::string, std::string> GetSystemHealthStatus() const;

    // Reporting and documentation
    void GenerateFailureModeReport(const std::string& filename) const;
    void GenerateRiskAssessmentReport(const std::string& filename) const;
    std::string GetFailureModeSummary() const;

private:
    std::map<std::string, SystemComponent> m_components;
    std::vector<FailureScenario> m_scenarios;
    std::map<std::string, std::vector<std::string>> m_failurePropagationGraph;
    std::atomic<bool> m_monitoringActive;
    std::mutex m_componentMutex;
    std::mutex m_scenarioMutex;

    void BuildFailurePropagationGraph();
    std::vector<std::string> GetCascadingImpact(const std::string& component) const;
    double CalculateComponentRiskScore(const SystemComponent& component) const;
    void MonitorComponentHealth(const std::string& component);
    
    // NeonGlyph-specific component mapping
    void MapVulkanComponents();
    void MapAudioComponents();
    void MapAIComponents();
    void MapASCIIComponents();
    void MapWindowComponents();
    void MapConfigurationComponents();
};

} // namespace Chaos
} // namespace NeonGlyph