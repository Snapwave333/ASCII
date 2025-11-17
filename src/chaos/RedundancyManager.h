#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>
#include <map>
#include <queue>

namespace NeonGlyph {
namespace Chaos {

enum class RedundancyLevel {
    NONE,           // No redundancy
    WARM_STANDBY,   // Standby system ready but not active
    HOT_STANDBY,    // Standby system synchronized and ready
    ACTIVE_ACTIVE,  // Multiple active systems load balancing
    N_PLUS_1,       // N+1 redundancy configuration
    GEOGRAPHIC      // Geographic distribution redundancy
};

enum class FailoverStrategy {
    MANUAL,         // Manual failover only
    AUTOMATIC,      // Automatic failover on failure detection
    GRACEFUL,       // Graceful degradation with reduced capacity
    IMMEDIATE,      // Immediate failover without waiting
    CONDITIONAL     // Failover based on specific conditions
};

enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    FAILED,
    UNKNOWN
};

struct SystemInstance {
    std::string id;
    std::string name;
    std::string endpoint;
    std::string location;
    HealthStatus healthStatus;
    std::chrono::steady_clock::time_point lastHealthCheck;
    std::chrono::steady_clock::time_point lastFailure;
    int failureCount;
    bool isPrimary;
    bool isActive;
    double responseTime;
    double cpuUsage;
    double memoryUsage;
    std::map<std::string, std::string> metadata;
};

struct FailoverConfig {
    FailoverStrategy strategy;
    std::chrono::milliseconds detectionTimeout;
    std::chrono::milliseconds failoverTimeout;
    std::chrono::milliseconds recoveryTimeout;
    int maxRetries;
    bool enableHealthMonitoring;
    bool enableAutomaticRecovery;
    bool enableGracefulDegradation;
    double cpuThreshold;
    double memoryThreshold;
    double responseTimeThreshold;
};

class RedundancyManager {
public:
    using HealthCheckFunction = std::function<HealthStatus(const SystemInstance&)>;
    using FailoverCallback = std::function<void(const SystemInstance&, const SystemInstance&)>;
    using RecoveryCallback = std::function<void(const SystemInstance&)>;
    
    RedundancyManager();
    ~RedundancyManager();
    
    // System registration and management
    void RegisterSystem(const std::string& systemType, std::shared_ptr<SystemInstance> instance);
    void UnregisterSystem(const std::string& systemType, const std::string& instanceId);
    std::vector<std::shared_ptr<SystemInstance>> GetSystemInstances(const std::string& systemType) const;
    std::shared_ptr<SystemInstance> GetPrimaryInstance(const std::string& systemType) const;
    std::shared_ptr<SystemInstance> GetHealthyInstance(const std::string& systemType) const;
    
    // Redundancy configuration
    void SetRedundancyLevel(const std::string& systemType, RedundancyLevel level);
    RedundancyLevel GetRedundancyLevel(const std::string& systemType) const;
    void SetFailoverConfig(const std::string& systemType, const FailoverConfig& config);
    FailoverConfig GetFailoverConfig(const std::string& systemType) const;
    
    // Health monitoring
    void StartHealthMonitoring();
    void StopHealthMonitoring();
    void PerformHealthCheck(const std::string& systemType, const std::string& instanceId);
    void PerformHealthCheckAll();
    HealthStatus GetInstanceHealth(const std::string& instanceId) const;
    std::map<std::string, HealthStatus> GetAllHealthStatus() const;
    
    // Failover operations
    bool TriggerFailover(const std::string& systemType, const std::string& failedInstanceId);
    bool TriggerAutomaticFailover(const std::string& systemType);
    bool PerformGracefulFailover(const std::string& systemType, const std::string& fromInstance, const std::string& toInstance);
    bool IsFailoverInProgress(const std::string& systemType) const;
    std::string GetCurrentPrimary(const std::string& systemType) const;
    
    // Recovery operations
    bool TriggerRecovery(const std::string& systemType, const std::string& instanceId);
    bool PerformAutomaticRecovery(const std::string& systemType);
    void SetInstanceHealthy(const std::string& instanceId);
    void SetInstanceUnhealthy(const std::string& instanceId, const std::string& reason);
    
    // Load balancing (for active-active configurations)
    std::shared_ptr<SystemInstance> SelectInstanceForLoadBalancing(const std::string& systemType) const;
    std::vector<std::shared_ptr<SystemInstance>> GetLoadBalancedInstances(const std::string& systemType, int count) const;
    void UpdateLoadBalancingWeights(const std::string& systemType, const std::map<std::string, double>& weights);
    
    // Callbacks and notifications
    void SetHealthCheckFunction(const std::string& systemType, HealthCheckFunction func);
    void SetFailoverCallback(const std::string& systemType, FailoverCallback callback);
    void SetRecoveryCallback(const std::string& systemType, RecoveryCallback callback);
    
    // Statistics and reporting
    std::map<std::string, double> GetSystemAvailability(const std::string& systemType) const;
    double GetOverallAvailability() const;
    std::map<std::string, int> GetFailureStatistics() const;
    std::string GenerateRedundancyReport() const;
    std::string GenerateFailoverReport() const;
    
    // Configuration management
    void LoadConfiguration(const std::string& configFile);
    void SaveConfiguration(const std::string& configFile) const;
    void SetDefaultConfiguration(const FailoverConfig& config);
    
    // Advanced features
    void EnableGeographicRedundancy(bool enabled);
    void SetCrossRegionFailover(bool enabled);
    void ConfigureDisasterRecovery(const std::string& primaryRegion, const std::vector<std::string>& backupRegions);
    
    // Emergency operations
    void EmergencyFailover(const std::string& systemType);
    void EmergencyShutdown(const std::string& systemType);
    void EmergencyRestart(const std::string& systemType);
    
    // NeonGlyph-specific redundancy configurations
    void ConfigureVulkanRedundancy();
    void ConfigureAudioRedundancy();
    void ConfigureAIRedundancy();
    void ConfigureGraphicsRedundancy();
    void ConfigureHeadlessFallback();

private:
    std::map<std::string, std::vector<std::shared_ptr<SystemInstance>>> m_systems;
    std::map<std::string, RedundancyLevel> m_redundancyLevels;
    std::map<std::string, FailoverConfig> m_failoverConfigs;
    std::map<std::string, HealthCheckFunction> m_healthCheckFunctions;
    std::map<std::string, FailoverCallback> m_failoverCallbacks;
    std::map<std::string, RecoveryCallback> m_recoveryCallbacks;
    
    std::atomic<bool> m_healthMonitoringActive;
    std::thread m_healthMonitoringThread;
    mutable std::mutex m_systemsMutex;
    
    // Failover tracking
    std::map<std::string, bool> m_failoverInProgress;
    std::map<std::string, std::chrono::steady_clock::time_point> m_lastFailoverTime;
    std::map<std::string, int> m_failoverCount;
    std::map<std::string, std::string> m_currentPrimary;
    
    // Load balancing state
    std::map<std::string, std::map<std::string, double>> m_loadBalancingWeights;
    std::map<std::string, size_t> m_roundRobinCounters;
    
    // Internal methods
    void HealthMonitoringLoop();
    bool ShouldTriggerFailover(const std::string& systemType, const std::shared_ptr<SystemInstance>& instance) const;
    bool CanPerformFailover(const std::string& systemType) const;
    std::shared_ptr<SystemInstance> SelectFailoverTarget(const std::string& systemType, const std::string& failedInstance) const;
    void UpdateInstanceHealth(const std::string& instanceId, HealthStatus status);
    void PerformFailoverInternal(const std::string& systemType, const std::shared_ptr<SystemInstance>& from, const std::shared_ptr<SystemInstance>& to);
    
    // Health check implementations
    HealthStatus PerformBasicHealthCheck(const SystemInstance& instance) const;
    HealthStatus PerformAdvancedHealthCheck(const SystemInstance& instance) const;
    bool IsInstanceResponsive(const SystemInstance& instance) const;
    bool AreResourcesHealthy(const SystemInstance& instance) const;
    
    // Failover strategies
    bool PerformManualFailover(const std::string& systemType, const std::string& fromInstance, const std::string& toInstance);
    bool PerformAutomaticFailover(const std::string& systemType, const std::shared_ptr<SystemInstance>& failedInstance);
    bool PerformGracefulDegradation(const std::string& systemType);
    
    // Load balancing algorithms
    std::shared_ptr<SystemInstance> SelectRoundRobin(const std::string& systemType) const;
    std::shared_ptr<SystemInstance> SelectWeightedRoundRobin(const std::string& systemType) const;
    std::shared_ptr<SystemInstance> SelectLeastConnections(const std::string& systemType) const;
    std::shared_ptr<SystemInstance> SelectHealthBased(const std::string& systemType) const;
    
    // Utility methods
    std::vector<std::shared_ptr<SystemInstance>> GetHealthyInstances(const std::string& systemType) const;
    std::vector<std::shared_ptr<SystemInstance>> GetUnhealthyInstances(const std::string& systemType) const;
    bool IsSystemHealthy(const std::string& systemType) const;
    double CalculateInstanceHealthScore(const SystemInstance& instance) const;
    
    // Statistics
    void RecordFailoverEvent(const std::string& systemType, const std::string& fromInstance, const std::string& toInstance);
    void RecordRecoveryEvent(const std::string& systemType, const std::string& instanceId);
    std::map<std::string, std::vector<std::string>> GetFailoverHistory() const;
};

} // namespace Chaos
} // namespace NeonGlyph