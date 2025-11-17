#pragma once

#include "ResilienceDashboard.h"
#include "CircuitBreaker.h"
#include "RedundancyManager.h"
#include "ChaosExperimentEngine.h"
#include <memory>
#include <functional>

namespace NeonGlyph {
namespace Chaos {

class ResilienceMonitoringIntegration {
public:
    static ResilienceMonitoringIntegration& GetInstance();
    
    void Initialize();
    void Shutdown();
    
    void RegisterCircuitBreaker(const std::string& name, CircuitBreaker* breaker);
    void UnregisterCircuitBreaker(const std::string& name);
    
    void RegisterRedundancySystem(const std::string& systemType, RedundancyManager* manager);
    void UnregisterRedundancySystem(const std::string& systemType);
    
    void StartDashboardWebServer(uint16_t port = 8080);
    void StopDashboardWebServer();
    
    void UpdateAllMetrics();
    
private:
    ResilienceMonitoringIntegration() = default;
    
    void RegisterDefaultMetrics();
    void RegisterCircuitBreakerMetrics(const std::string& name, CircuitBreaker* breaker);
    void RegisterRedundancyMetrics(const std::string& systemType, RedundancyManager* manager);
    void RegisterChaosExperimentMetrics();
    
    std::map<std::string, CircuitBreaker*> m_circuitBreakers;
    std::map<std::string, RedundancyManager*> m_redundancyManagers;
    
    bool m_initialized{false};
};

// Helper class to automatically register circuit breakers with monitoring
class MonitoredCircuitBreaker {
public:
    MonitoredCircuitBreaker(const std::string& name, 
                          size_t failureThreshold = 5,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds(60000),
                          std::chrono::milliseconds resetTimeout = std::chrono::milliseconds(30000))
        : m_circuitBreaker(failureThreshold, timeout, resetTimeout)
        , m_name(name) {
        
        // Register with monitoring
        ResilienceMonitoringIntegration::GetInstance().RegisterCircuitBreaker(name, &m_circuitBreaker);
        
        // Set up state change callback to record metrics
        m_circuitBreaker.SetStateChangeCallback([this](CircuitBreakerState oldState, CircuitBreakerState newState) {
            auto& collector = MetricsCollector::GetInstance();
            
            collector.RecordMetric("circuit_breaker_state_change", 1.0, MetricType::COUNTER, {
                {"breaker_name", m_name},
                {"old_state", CircuitBreakerStateToString(oldState)},
                {"new_state", CircuitBreakerStateToString(newState)}
            });
            
            collector.RecordMetric("circuit_breaker_state", static_cast<double>(newState), MetricType::GAUGE, {
                {"breaker_name", m_name}
            });
        });
    }
    
    ~MonitoredCircuitBreaker() {
        ResilienceMonitoringIntegration::GetInstance().UnregisterCircuitBreaker(m_name);
    }
    
    template<typename Func>
    auto Execute(Func&& func) -> decltype(func()) {
        auto start = std::chrono::steady_clock::now();
        
        try {
            auto result = m_circuitBreaker.Execute(std::forward<Func>(func));
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            
            // Record success metrics
            auto& collector = MetricsCollector::GetInstance();
            collector.RecordMetric("circuit_breaker_execution_time_ms", duration, MetricType::TIMER, {
                {"breaker_name", m_name},
                {"result", "success"}
            });
            
            return result;
        } catch (const std::exception& e) {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            
            // Record failure metrics
            auto& collector = MetricsCollector::GetInstance();
            collector.RecordMetric("circuit_breaker_execution_time_ms", duration, MetricType::TIMER, {
                {"breaker_name", m_name},
                {"result", "failure"}
            });
            
            throw;
        }
    }
    
    CircuitBreakerState GetState() const {
        return m_circuitBreaker.GetState();
    }
    
    CircuitBreakerMetrics GetMetrics() const {
        return m_circuitBreaker.GetMetrics();
    }
    
private:
    std::string CircuitBreakerStateToString(CircuitBreakerState state) const {
        switch (state) {
            case CircuitBreakerState::CLOSED:
                return "CLOSED";
            case CircuitBreakerState::OPEN:
                return "OPEN";
            case CircuitBreakerState::HALF_OPEN:
                return "HALF_OPEN";
            default:
                return "UNKNOWN";
        }
    }
    
    CircuitBreaker m_circuitBreaker;
    std::string m_name;
};

// Helper class to automatically register redundancy systems with monitoring
class MonitoredRedundancyManager {
public:
    MonitoredRedundancyManager(const std::string& systemType)
        : m_systemType(systemType) {
        
        // Register with monitoring
        ResilienceMonitoringIntegration::GetInstance().RegisterRedundancySystem(systemType, &m_redundancyManager);
        
        // Set up health monitoring for this redundancy system
        HealthMonitor::GetInstance().RegisterHealthCheck(systemType + "_redundancy", [this]() {
            return CheckRedundancyHealth();
        });
    }
    
    ~MonitoredRedundancyManager() {
        ResilienceMonitoringIntegration::GetInstance().UnregisterRedundancySystem(m_systemType);
        HealthMonitor::GetInstance().UnregisterHealthCheck(m_systemType + "_redundancy");
    }
    
    void SetRedundancyLevel(const std::string& systemType, RedundancyLevel level) {
        m_redundancyManager.SetRedundancyLevel(systemType, level);
        
        // Record metric
        auto& collector = MetricsCollector::GetInstance();
        collector.RecordMetric("redundancy_level_change", 1.0, MetricType::COUNTER, {
            {"system_type", systemType},
            {"redundancy_level", RedundancyLevelToString(level)}
        });
    }
    
    bool TriggerFailover(const std::string& systemType, const std::string& failedInstanceId) {
        auto start = std::chrono::steady_clock::now();
        
        bool result = m_redundancyManager.TriggerFailover(systemType, failedInstanceId);
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record metrics
        auto& collector = MetricsCollector::GetInstance();
        collector.RecordMetric("failover_triggered", 1.0, MetricType::COUNTER, {
            {"system_type", systemType},
            {"result", result ? "success" : "failure"}
        });
        
        collector.RecordMetric("failover_duration_ms", duration, MetricType::TIMER, {
            {"system_type", systemType}
        });
        
        return result;
    }
    
    RedundancyManager* GetManager() {
        return &m_redundancyManager;
    }
    
private:
    HealthStatus CheckRedundancyHealth() const {
        auto metrics = m_redundancyManager.GetRedundancyMetrics(m_systemType);
        
        if (metrics.healthyInstances == 0) {
            return HealthStatus::CRITICAL;
        } else if (metrics.failedInstances > 0) {
            return HealthStatus::DEGRADED;
        } else {
            return HealthStatus::HEALTHY;
        }
    }
    
    std::string RedundancyLevelToString(RedundancyLevel level) const {
        switch (level) {
            case RedundancyLevel::NONE:
                return "NONE";
            case RedundancyLevel::WARM_STANDBY:
                return "WARM_STANDBY";
            case RedundancyLevel::HOT_STANDBY:
                return "HOT_STANDBY";
            case RedundancyLevel::ACTIVE_ACTIVE:
                return "ACTIVE_ACTIVE";
            case RedundancyLevel::N_PLUS_1:
                return "N_PLUS_1";
            case RedundancyLevel::GEOGRAPHIC:
                return "GEOGRAPHIC";
            default:
                return "UNKNOWN";
        }
    }
    
    RedundancyManager m_redundancyManager;
    std::string m_systemType;
};

// Implementation of ResilienceMonitoringIntegration
ResilienceMonitoringIntegration& ResilienceMonitoringIntegration::GetInstance() {
    static ResilienceMonitoringIntegration instance;
    return instance;
}

void ResilienceMonitoringIntegration::Initialize() {
    if (m_initialized) {
        return;
    }
    
    // Initialize the main dashboard
    ResilienceDashboard::GetInstance().Initialize();
    
    // Register default metrics
    RegisterDefaultMetrics();
    
    m_initialized = true;
}

void ResilienceMonitoringIntegration::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    StopDashboardWebServer();
    ResilienceDashboard::GetInstance().Shutdown();
    
    m_circuitBreakers.clear();
    m_redundancyManagers.clear();
    
    m_initialized = false;
}

void ResilienceMonitoringIntegration::RegisterCircuitBreaker(const std::string& name, CircuitBreaker* breaker) {
    m_circuitBreakers[name] = breaker;
    RegisterCircuitBreakerMetrics(name, breaker);
}

void ResilienceMonitoringIntegration::UnregisterCircuitBreaker(const std::string& name) {
    m_circuitBreakers.erase(name);
    CircuitBreakerMonitor::GetInstance().UnregisterCircuitBreaker(name);
}

void ResilienceMonitoringIntegration::RegisterRedundancySystem(const std::string& systemType, RedundancyManager* manager) {
    m_redundancyManagers[systemType] = manager;
    RegisterRedundancyMetrics(systemType, manager);
}

void ResilienceMonitoringIntegration::UnregisterRedundancySystem(const std::string& systemType) {
    m_redundancyManagers.erase(systemType);
    RedundancyMonitor::GetInstance().UnregisterRedundancySystem(systemType);
}

void ResilienceMonitoringIntegration::StartDashboardWebServer(uint16_t port) {
    ResilienceDashboard::GetInstance().StartWebServer(port);
}

void ResilienceMonitoringIntegration::StopDashboardWebServer() {
    ResilienceDashboard::GetInstance().StopWebServer();
}

void ResilienceMonitoringIntegration::UpdateAllMetrics() {
    // Update system-level metrics
    auto& collector = MetricsCollector::GetInstance();
    
    // System uptime
    static auto startTime = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime).count();
    collector.RecordMetric("system_uptime_seconds", static_cast<double>(uptime), MetricType::GAUGE);
    
    // Update circuit breaker metrics
    for (const auto& cb : m_circuitBreakers) {
        auto metrics = cb.second->GetMetrics();
        collector.RecordMetric("circuit_breaker_success_count", static_cast<double>(metrics.successCount), MetricType::COUNTER, {
            {"breaker_name", cb.first}
        });
        collector.RecordMetric("circuit_breaker_failure_count", static_cast<double>(metrics.failureCount), MetricType::COUNTER, {
            {"breaker_name", cb.first}
        });
    }
    
    // Update redundancy metrics
    for (const auto& rm : m_redundancyManagers) {
        auto metrics = rm.second->GetRedundancyMetrics(rm.first);
        collector.RecordMetric("redundancy_total_instances", static_cast<double>(metrics.totalInstances), MetricType::GAUGE, {
            {"system_type", rm.first}
        });
        collector.RecordMetric("redundancy_healthy_instances", static_cast<double>(metrics.healthyInstances), MetricType::GAUGE, {
            {"system_type", rm.first}
        });
    }
}

void ResilienceMonitoringIntegration::RegisterDefaultMetrics() {
    auto& dashboard = ResilienceDashboard::GetInstance();
    
    // Register custom panels
    dashboard.RegisterCustomPanel("System Performance", []() {
        std::stringstream html;
        html << "<div class=\"metric\">\n";
        html << "<div class=\"metric-value\">" << MetricsCollector::GetInstance().GetMetricValue("system_uptime_seconds") << "</div>\n";
        html << "<div class=\"metric-label\">System Uptime (seconds)</div>\n";
        html << "</div>\n";
        return html.str();
    });
    
    dashboard.RegisterCustomPanel("Chaos Engineering Stats", []() {
        auto& monitor = ChaosExperimentMonitor::GetInstance();
        auto typeCounts = monitor.GetExperimentTypeCounts();
        double successRate = monitor.GetExperimentSuccessRate();
        
        std::stringstream html;
        html << "<div class=\"metric\">\n";
        html << "<div class=\"metric-value\">" << successRate << "%</div>\n";
        html << "<div class=\"metric-label\">Overall Success Rate</div>\n";
        html << "</div>\n";
        
        html << "<h4>Experiment Type Counts:</h4>\n";
        html << "<ul>\n";
        for (const auto& type : typeCounts) {
            html << "<li>" << type.first << ": " << type.second << "</li>\n";
        }
        html << "</ul>\n";
        
        return html.str();
    });
}

void ResilienceMonitoringIntegration::RegisterCircuitBreakerMetrics(const std::string& name, CircuitBreaker* breaker) {
    // Register with circuit breaker monitor
    CircuitBreakerMonitor::GetInstance().RegisterCircuitBreaker(name, [breaker, name]() {
        auto metrics = breaker->GetMetrics();
        CircuitBreakerMetrics cbMetrics;
        cbMetrics.name = name;
        cbMetrics.state = (metrics.state == CircuitBreakerState::CLOSED ? "CLOSED" : 
                          metrics.state == CircuitBreakerState::OPEN ? "OPEN" : "HALF_OPEN");
        cbMetrics.successCount = metrics.successCount;
        cbMetrics.failureCount = metrics.failureCount;
        cbMetrics.timeoutCount = metrics.timeoutCount;
        cbMetrics.failureRate = metrics.failureRate;
        cbMetrics.lastStateChange = metrics.lastStateChange;
        return cbMetrics;
    });
}

void ResilienceMonitoringIntegration::RegisterRedundancyMetrics(const std::string& systemType, RedundancyManager* manager) {
    // Register with redundancy monitor
    RedundancyMonitor::GetInstance().RegisterRedundancySystem(systemType, [manager, systemType]() {
        return manager->GetRedundancyMetrics(systemType);
    });
}

} // namespace Chaos
} // namespace NeonGlyph