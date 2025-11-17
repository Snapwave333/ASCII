#include "RedundancyManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>

namespace NeonGlyph {
namespace Chaos {

RedundancyManager::RedundancyManager() 
    : m_healthMonitoringActive(false) {
    
    // Set default failover configuration
    FailoverConfig defaultConfig;
    defaultConfig.strategy = FailoverStrategy::AUTOMATIC;
    defaultConfig.detectionTimeout = std::chrono::milliseconds(5000);
    defaultConfig.failoverTimeout = std::chrono::milliseconds(10000);
    defaultConfig.recoveryTimeout = std::chrono::milliseconds(30000);
    defaultConfig.maxRetries = 3;
    defaultConfig.enableHealthMonitoring = true;
    defaultConfig.enableAutomaticRecovery = true;
    defaultConfig.enableGracefulDegradation = true;
    defaultConfig.cpuThreshold = 90.0;
    defaultConfig.memoryThreshold = 85.0;
    defaultConfig.responseTimeThreshold = 5000.0;
    
    SetDefaultConfiguration(defaultConfig);
}

RedundancyManager::~RedundancyManager() {
    StopHealthMonitoring();
}

void RedundancyManager::RegisterSystem(const std::string& systemType, std::shared_ptr<SystemInstance> instance) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    m_systems[systemType].push_back(instance);
    
    // Set default redundancy level if not configured
    if (m_redundancyLevels.find(systemType) == m_redundancyLevels.end()) {
        m_redundancyLevels[systemType] = RedundancyLevel::WARM_STANDBY;
    }
    
    // Set default failover config if not configured
    if (m_failoverConfigs.find(systemType) == m_failoverConfigs.end()) {
        m_failoverConfigs[systemType] = m_failoverConfigs["default"];
    }
    
    // Set as primary if it's the first instance
    if (instance->isPrimary || m_systems[systemType].size() == 1) {
        for (auto& inst : m_systems[systemType]) {
            inst->isPrimary = (inst == instance);
        }
        m_currentPrimary[systemType] = instance->id;
    }
    
    std::cout << "Registered system instance: " << instance->name << " (" << systemType << ")" << std::endl;
}

void RedundancyManager::UnregisterSystem(const std::string& systemType, const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto& instances = m_systems[systemType];
    instances.erase(
        std::remove_if(instances.begin(), instances.end(),
            [&instanceId](const std::shared_ptr<SystemInstance>& instance) {
                return instance->id == instanceId;
            }
        ),
        instances.end()
    );
    
    // Update primary if removed instance was primary
    if (m_currentPrimary[systemType] == instanceId) {
        // Find a new primary from healthy instances
        for (const auto& instance : instances) {
            if (instance->healthStatus == HealthStatus::HEALTHY) {
                instance->isPrimary = true;
                m_currentPrimary[systemType] = instance->id;
                break;
            }
        }
    }
    
    std::cout << "Unregistered system instance: " << instanceId << " (" << systemType << ")" << std::endl;
}

std::vector<std::shared_ptr<SystemInstance>> RedundancyManager::GetSystemInstances(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto it = m_systems.find(systemType);
    if (it != m_systems.end()) {
        return it->second;
    }
    
    return {};
}

std::shared_ptr<SystemInstance> RedundancyManager::GetPrimaryInstance(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto it = m_currentPrimary.find(systemType);
    if (it != m_currentPrimary.end()) {
        const std::string& primaryId = it->second;
        
        auto instancesIt = m_systems.find(systemType);
        if (instancesIt != m_systems.end()) {
            for (const auto& instance : instancesIt->second) {
                if (instance->id == primaryId) {
                    return instance;
                }
            }
        }
    }
    
    return nullptr;
}

std::shared_ptr<SystemInstance> RedundancyManager::GetHealthyInstance(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto instancesIt = m_systems.find(systemType);
    if (instancesIt != m_systems.end()) {
        for (const auto& instance : instancesIt->second) {
            if (instance->healthStatus == HealthStatus::HEALTHY) {
                return instance;
            }
        }
    }
    
    return nullptr;
}

void RedundancyManager::SetRedundancyLevel(const std::string& systemType, RedundancyLevel level) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    m_redundancyLevels[systemType] = level;
    
    std::cout << "Set redundancy level for " << systemType << " to " << static_cast<int>(level) << std::endl;
}

RedundancyLevel RedundancyManager::GetRedundancyLevel(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto it = m_redundancyLevels.find(systemType);
    if (it != m_redundancyLevels.end()) {
        return it->second;
    }
    
    return RedundancyLevel::NONE;
}

void RedundancyManager::SetFailoverConfig(const std::string& systemType, const FailoverConfig& config) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    m_failoverConfigs[systemType] = config;
    
    std::cout << "Updated failover configuration for " << systemType << std::endl;
}

FailoverConfig RedundancyManager::GetFailoverConfig(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto it = m_failoverConfigs.find(systemType);
    if (it != m_failoverConfigs.end()) {
        return it->second;
    }
    
    return m_failoverConfigs.at("default");
}

void RedundancyManager::StartHealthMonitoring() {
    if (m_healthMonitoringActive) {
        return;
    }
    
    m_healthMonitoringActive = true;
    m_healthMonitoringThread = std::thread(&RedundancyManager::HealthMonitoringLoop, this);
    
    std::cout << "Health monitoring started" << std::endl;
}

void RedundancyManager::StopHealthMonitoring() {
    m_healthMonitoringActive = false;
    
    if (m_healthMonitoringThread.joinable()) {
        m_healthMonitoringThread.join();
    }
    
    std::cout << "Health monitoring stopped" << std::endl;
}

void RedundancyManager::HealthMonitoringLoop() {
    while (m_healthMonitoringActive) {
        PerformHealthCheckAll();
        
        // Check for automatic failover opportunities
        for (const auto& [systemType, instances] : m_systems) {
            auto config = GetFailoverConfig(systemType);
            if (config.enableHealthMonitoring && config.enableAutomaticRecovery) {
                TriggerAutomaticFailover(systemType);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
    }
}

void RedundancyManager::PerformHealthCheck(const std::string& systemType, const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto instancesIt = m_systems.find(systemType);
    if (instancesIt == m_systems.end()) {
        return;
    }
    
    for (auto& instance : instancesIt->second) {
        if (instance->id == instanceId) {
            // Perform health check
            HealthStatus newStatus = HealthStatus::UNKNOWN;
            
            auto healthCheckIt = m_healthCheckFunctions.find(systemType);
            if (healthCheckIt != m_healthCheckFunctions.end() && healthCheckIt->second) {
                newStatus = healthCheckIt->second(*instance);
            } else {
                // Default health check
                newStatus = PerformBasicHealthCheck(*instance);
            }
            
            // Update health status
            HealthStatus oldStatus = instance->healthStatus;
            instance->healthStatus = newStatus;
            instance->lastHealthCheck = std::chrono::steady_clock::now();
            
            if (newStatus != oldStatus) {
                std::cout << "Health status changed for " << instance->name 
                         << ": " << static_cast<int>(oldStatus) << " -> " << static_cast<int>(newStatus) << std::endl;
            }
            
            break;
        }
    }
}

void RedundancyManager::PerformHealthCheckAll() {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    for (const auto& [systemType, instances] : m_systems) {
        for (const auto& instance : instances) {
            PerformHealthCheck(systemType, instance->id);
        }
    }
}

HealthStatus RedundancyManager::PerformBasicHealthCheck(const SystemInstance& instance) const {
    // Simulate basic health check logic
    
    // Check response time
    if (instance.responseTime > 10000.0) { // 10 seconds
        return HealthStatus::FAILED;
    }
    
    // Check resource usage
    if (instance.cpuUsage > 95.0 || instance.memoryUsage > 95.0) {
        return HealthStatus::UNHEALTHY;
    }
    
    if (instance.cpuUsage > 80.0 || instance.memoryUsage > 80.0) {
        return HealthStatus::DEGRADED;
    }
    
    // Check failure count
    if (instance.failureCount > 5) {
        return HealthStatus::UNHEALTHY;
    }
    
    return HealthStatus::HEALTHY;
}

bool RedundancyManager::TriggerFailover(const std::string& systemType, const std::string& failedInstanceId) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto config = GetFailoverConfig(systemType);
    
    if (config.strategy == FailoverStrategy::MANUAL) {
        std::cout << "Manual failover required for " << systemType << std::endl;
        return false;
    }
    
    // Find the failed instance
    std::shared_ptr<SystemInstance> failedInstance = nullptr;
    auto instancesIt = m_systems.find(systemType);
    if (instancesIt != m_systems.end()) {
        for (const auto& instance : instancesIt->second) {
            if (instance->id == failedInstanceId) {
                failedInstance = instance;
                break;
            }
        }
    }
    
    if (!failedInstance) {
        std::cerr << "Failed instance not found: " << failedInstanceId << std::endl;
        return false;
    }
    
    // Select failover target
    auto targetInstance = SelectFailoverTarget(systemType, failedInstanceId);
    if (!targetInstance) {
        std::cerr << "No failover target available for " << systemType << std::endl;
        return false;
    }
    
    // Perform failover
    PerformFailoverInternal(systemType, failedInstance, targetInstance);
    
    return true;
}

bool RedundancyManager::TriggerAutomaticFailover(const std::string& systemType) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto config = GetFailoverConfig(systemType);
    if (!config.enableAutomaticRecovery || config.strategy != FailoverStrategy::AUTOMATIC) {
        return false;
    }
    
    // Check if failover is already in progress
    if (m_failoverInProgress[systemType]) {
        return false;
    }
    
    // Find unhealthy instances that need failover
    auto instancesIt = m_systems.find(systemType);
    if (instancesIt == m_systems.end()) {
        return false;
    }
    
    for (const auto& instance : instancesIt->second) {
        if (instance->healthStatus == HealthStatus::FAILED || 
            instance->healthStatus == HealthStatus::UNHEALTHY) {
            
            // Check if this instance is primary
            if (instance->isPrimary) {
                // Trigger automatic failover
                return TriggerFailover(systemType, instance->id);
            }
        }
    }
    
    return false;
}

std::shared_ptr<SystemInstance> RedundancyManager::SelectFailoverTarget(const std::string& systemType, const std::string& failedInstance) const {
    auto instancesIt = m_systems.find(systemType);
    if (instancesIt == m_systems.end()) {
        return nullptr;
    }
    
    // Find the healthiest available instance
    std::shared_ptr<SystemInstance> bestTarget = nullptr;
    double bestHealthScore = -1.0;
    
    for (const auto& instance : instancesIt->second) {
        if (instance->id == failedInstance) {
            continue; // Skip the failed instance
        }
        
        if (instance->healthStatus == HealthStatus::HEALTHY) {
            double healthScore = CalculateInstanceHealthScore(*instance);
            if (healthScore > bestHealthScore) {
                bestHealthScore = healthScore;
                bestTarget = instance;
            }
        }
    }
    
    return bestTarget;
}

void RedundancyManager::PerformFailoverInternal(const std::string& systemType, const std::shared_ptr<SystemInstance>& from, const std::shared_ptr<SystemInstance>& to) {
    m_failoverInProgress[systemType] = true;
    m_lastFailoverTime[systemType] = std::chrono::steady_clock::now();
    m_failoverCount[systemType]++;
    
    std::cout << "Performing failover for " << systemType 
             << " from " << from->name << " to " << to->name << std::endl;
    
    // Update primary status
    from->isPrimary = false;
    to->isPrimary = true;
    m_currentPrimary[systemType] = to->id;
    
    // Call failover callback
    auto callbackIt = m_failoverCallbacks.find(systemType);
    if (callbackIt != m_failoverCallbacks.end() && callbackIt->second) {
        callbackIt->second(*from, *to);
    }
    
    // Record failover event
    RecordFailoverEvent(systemType, from->id, to->id);
    
    m_failoverInProgress[systemType] = false;
    
    std::cout << "Failover completed for " << systemType << std::endl;
}

double RedundancyManager::CalculateInstanceHealthScore(const SystemInstance& instance) const {
    double score = 100.0;
    
    // Penalize based on response time
    if (instance.responseTime > 0) {
        score -= std::min(30.0, instance.responseTime / 100.0); // -30 max for slow response
    }
    
    // Penalize based on CPU usage
    score -= (instance.cpuUsage / 100.0) * 20.0; // -20 max for high CPU
    
    // Penalize based on memory usage
    score -= (instance.memoryUsage / 100.0) * 20.0; // -20 max for high memory
    
    // Penalize based on failure count
    score -= std::min(30.0, static_cast<double>(instance.failureCount) * 5.0); // -30 max for failures
    
    // Bonus for being active and healthy
    if (instance.healthStatus == HealthStatus::HEALTHY) {
        score += 10.0;
    }
    
    return std::max(0.0, score);
}

std::shared_ptr<SystemInstance> RedundancyManager::SelectInstanceForLoadBalancing(const std::string& systemType) const {
    auto redundancyLevel = GetRedundancyLevel(systemType);
    
    switch (redundancyLevel) {
        case RedundancyLevel::ACTIVE_ACTIVE:
            return SelectRoundRobin(systemType);
            
        case RedundancyLevel::N_PLUS_1:
            return SelectHealthBased(systemType);
            
        default:
            // For other redundancy levels, just return the primary
            return GetPrimaryInstance(systemType);
    }
}

std::shared_ptr<SystemInstance> RedundancyManager::SelectRoundRobin(const std::string& systemType) const {
    auto healthyInstances = GetHealthyInstances(systemType);
    if (healthyInstances.empty()) {
        return nullptr;
    }
    
    size_t& counter = m_roundRobinCounters[systemType];
    counter = (counter + 1) % healthyInstances.size();
    
    return healthyInstances[counter];
}

std::shared_ptr<SystemInstance> RedundancyManager::SelectHealthBased(const std::string& systemType) const {
    auto healthyInstances = GetHealthyInstances(systemType);
    if (healthyInstances.empty()) {
        return nullptr;
    }
    
    // Select the healthiest instance
    std::shared_ptr<SystemInstance> bestInstance = healthyInstances[0];
    double bestScore = CalculateInstanceHealthScore(*bestInstance);
    
    for (size_t i = 1; i < healthyInstances.size(); ++i) {
        double score = CalculateInstanceHealthScore(*healthyInstances[i]);
        if (score > bestScore) {
            bestScore = score;
            bestInstance = healthyInstances[i];
        }
    }
    
    return bestInstance;
}

std::vector<std::shared_ptr<SystemInstance>> RedundancyManager::GetHealthyInstances(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    std::vector<std::shared_ptr<SystemInstance>> healthy;
    
    auto instancesIt = m_systems.find(systemType);
    if (instancesIt != m_systems.end()) {
        for (const auto& instance : instancesIt->second) {
            if (instance->healthStatus == HealthStatus::HEALTHY) {
                healthy.push_back(instance);
            }
        }
    }
    
    return healthy;
}

void RedundancyManager::ConfigureVulkanRedundancy() {
    // Configure redundancy for Vulkan graphics system
    SetRedundancyLevel("VulkanContext", RedundancyLevel::HOT_STANDBY);
    
    FailoverConfig vulkanConfig;
    vulkanConfig.strategy = FailoverStrategy::AUTOMATIC;
    vulkanConfig.detectionTimeout = std::chrono::milliseconds(1000); // 1 second
    vulkanConfig.failoverTimeout = std::chrono::milliseconds(2000);  // 2 seconds
    vulkanConfig.enableHealthMonitoring = true;
    vulkanConfig.cpuThreshold = 95.0;
    vulkanConfig.memoryThreshold = 90.0;
    vulkanConfig.responseTimeThreshold = 1000.0; // 1 second
    
    SetFailoverConfig("VulkanContext", vulkanConfig);
    
    std::cout << "Configured Vulkan redundancy with hot standby" << std::endl;
}

void RedundancyManager::ConfigureAudioRedundancy() {
    // Configure redundancy for audio system
    SetRedundancyLevel("AudioEngine", RedundancyLevel::WARM_STANDBY);
    
    FailoverConfig audioConfig;
    audioConfig.strategy = FailoverStrategy::GRACEFUL;
    audioConfig.detectionTimeout = std::chrono::milliseconds(3000); // 3 seconds
    audioConfig.failoverTimeout = std::chrono::milliseconds(5000);  // 5 seconds
    audioConfig.enableGracefulDegradation = true;
    audioConfig.cpuThreshold = 85.0;
    audioConfig.memoryThreshold = 80.0;
    
    SetFailoverConfig("AudioEngine", audioConfig);
    
    std::cout << "Configured Audio redundancy with warm standby and graceful degradation" << std::endl;
}

void RedundancyManager::ConfigureHeadlessFallback() {
    // Configure headless mode as fallback for window system
    SetRedundancyLevel("Window", RedundancyLevel::HOT_STANDBY);
    
    FailoverConfig headlessConfig;
    headlessConfig.strategy = FailoverStrategy::AUTOMATIC;
    headlessConfig.detectionTimeout = std::chrono::milliseconds(500); // 500ms
    headlessConfig.failoverTimeout = std::chrono::milliseconds(1000); // 1 second
    headlessConfig.enableAutomaticRecovery = true;
    
    SetFailoverConfig("Window", headlessConfig);
    
    std::cout << "Configured Headless fallback for window system" << std::endl;
}

std::string RedundancyManager::GenerateRedundancyReport() const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    std::stringstream report;
    report << "NEONGLYPH REDUNDANCY MANAGER REPORT\n";
    report << "====================================\n\n";
    
    report << "SYSTEM REDUNDANCY STATUS:\n";
    for (const auto& [systemType, instances] : m_systems) {
        report << "- " << systemType << ":\n";
        report << "  Redundancy Level: " << static_cast<int>(m_redundancyLevels.at(systemType)) << "\n";
        report << "  Total Instances: " << instances.size() << "\n";
        report << "  Healthy Instances: " << GetHealthyInstances(systemType).size() << "\n";
        report << "  Primary Instance: " << m_currentPrimary.at(systemType) << "\n";
        report << "  Failover Count: " << m_failoverCount.at(systemType) << "\n\n";
    }
    
    report << "OVERALL AVAILABILITY: " << GetOverallAvailability() << "%\n";
    
    return report.str();
}

void RedundancyManager::RecordFailoverEvent(const std::string& systemType, const std::string& fromInstance, const std::string& toInstance) {
    std::cout << "Recorded failover event: " << systemType << " from " << fromInstance << " to " << toInstance << std::endl;
}

} // namespace Chaos
} // namespace NeonGlyph