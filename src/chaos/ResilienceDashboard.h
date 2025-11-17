#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <deque>

namespace NeonGlyph {
namespace Chaos {

enum class MetricType {
    COUNTER,
    GAUGE,
    HISTOGRAM,
    TIMER
};

enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    CRITICAL
};

struct MetricData {
    std::string name;
    MetricType type;
    double value;
    std::chrono::steady_clock::time_point timestamp;
    std::map<std::string, std::string> labels;
};

struct HealthCheck {
    std::string component;
    HealthStatus status;
    std::string message;
    std::chrono::steady_clock::time_point lastCheck;
    double responseTimeMs;
};

struct CircuitBreakerMetrics {
    std::string name;
    std::string state;
    uint64_t successCount;
    uint64_t failureCount;
    uint64_t timeoutCount;
    double failureRate;
    std::chrono::steady_clock::time_point lastStateChange;
};

struct RedundancyMetrics {
    std::string systemType;
    size_t totalInstances;
    size_t healthyInstances;
    size_t failedInstances;
    std::string redundancyLevel;
    bool failoverActive;
    std::chrono::steady_clock::time_point lastFailover;
};

struct ChaosMetrics {
    std::string experimentId;
    std::string experimentType;
    std::string status;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    std::map<std::string, std::string> parameters;
    std::string impactAssessment;
};

class MetricsCollector {
public:
    static MetricsCollector& GetInstance();
    
    void RecordMetric(const std::string& name, double value, MetricType type, 
                     const std::map<std::string, std::string>& labels = {});
    
    std::vector<MetricData> GetMetrics(const std::string& name = "", 
                                       const std::map<std::string, std::string>& labels = {}) const;
    
    double GetMetricValue(const std::string& name, 
                         const std::map<std::string, std::string>& labels = {}) const;
    
    void ClearMetrics(const std::string& name = "");
    
private:
    MetricsCollector() = default;
    mutable std::mutex m_metricsMutex;
    std::map<std::string, std::vector<MetricData>> m_metrics;
    
    static constexpr size_t MAX_METRICS_PER_TYPE = 10000;
};

class HealthMonitor {
public:
    using HealthCheckFunction = std::function<HealthStatus()>;
    
    static HealthMonitor& GetInstance();
    
    void RegisterHealthCheck(const std::string& component, HealthCheckFunction checkFunc);
    void UnregisterHealthCheck(const std::string& component);
    
    HealthStatus CheckComponent(const std::string& component);
    std::vector<HealthCheck> GetAllHealthChecks() const;
    
    HealthStatus GetOverallHealth() const;
    double GetHealthScore() const;
    
    void StartMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(5000));
    void StopMonitoring();
    
private:
    HealthMonitor() = default;
    mutable std::mutex m_checksMutex;
    std::map<std::string, HealthCheckFunction> m_healthChecks;
    std::map<std::string, HealthCheck> m_lastResults;
    std::atomic<bool> m_monitoring{false};
    std::thread m_monitorThread;
    
    void MonitorLoop(std::chrono::milliseconds interval);
    HealthStatus AggregateHealthStatus(const std::vector<HealthStatus>& statuses) const;
};

class CircuitBreakerMonitor {
public:
    static CircuitBreakerMonitor& GetInstance();
    
    void RegisterCircuitBreaker(const std::string& name, 
                               std::function<CircuitBreakerMetrics()> metricsFunc);
    void UnregisterCircuitBreaker(const std::string& name);
    
    std::vector<CircuitBreakerMetrics> GetAllCircuitBreakerMetrics() const;
    CircuitBreakerMetrics GetCircuitBreakerMetrics(const std::string& name) const;
    
    std::vector<std::string> GetOpenCircuitBreakers() const;
    double GetOverallCircuitBreakerHealth() const;
    
private:
    CircuitBreakerMonitor() = default;
    mutable std::mutex m_breakersMutex;
    std::map<std::string, std::function<CircuitBreakerMetrics()>> m_breakers;
};

class RedundancyMonitor {
public:
    static RedundancyMonitor& GetInstance();
    
    void RegisterRedundancySystem(const std::string& systemType,
                                 std::function<RedundancyMetrics()> metricsFunc);
    void UnregisterRedundancySystem(const std::string& systemType);
    
    std::vector<RedundancyMetrics> GetAllRedundancyMetrics() const;
    RedundancyMetrics GetRedundancyMetrics(const std::string& systemType) const;
    
    std::vector<std::string> GetSystemsWithActiveFailover() const;
    bool IsRedundancyHealthy(const std::string& systemType) const;
    
private:
    RedundancyMonitor() = default;
    mutable std::mutex m_systemsMutex;
    std::map<std::string, std::function<RedundancyMetrics()>> m_systems;
};

class ChaosExperimentMonitor {
public:
    static ChaosExperimentMonitor& GetInstance();
    
    void RecordExperimentStart(const std::string& experimentId, 
                              const std::string& experimentType,
                              const std::map<std::string, std::string>& parameters);
    
    void RecordExperimentEnd(const std::string& experimentId, 
                            const std::string& status,
                            const std::string& impactAssessment);
    
    std::vector<ChaosMetrics> GetExperimentHistory(size_t limit = 100) const;
    std::vector<ChaosMetrics> GetActiveExperiments() const;
    
    ChaosMetrics GetExperimentMetrics(const std::string& experimentId) const;
    
    std::map<std::string, size_t> GetExperimentTypeCounts() const;
    double GetExperimentSuccessRate() const;
    
private:
    ChaosExperimentMonitor() = default;
    mutable std::mutex m_experimentsMutex;
    std::map<std::string, ChaosMetrics> m_experiments;
    std::deque<std::string> m_experimentOrder;
    
    static constexpr size_t MAX_HISTORY_SIZE = 1000;
};

class ResilienceDashboard {
public:
    static ResilienceDashboard& GetInstance();
    
    void Initialize();
    void Shutdown();
    
    void UpdateDashboard();
    std::string GenerateDashboardHTML() const;
    std::string GenerateDashboardJSON() const;
    
    void StartWebServer(uint16_t port = 8080);
    void StopWebServer();
    
    void RegisterCustomPanel(const std::string& name, 
                            std::function<std::string()> contentGenerator);
    
    std::vector<std::string> GetAvailablePanels() const;
    std::string GetPanelContent(const std::string& name) const;
    
private:
    ResilienceDashboard() = default;
    
    std::string GenerateOverviewPanel() const;
    std::string GenerateHealthPanel() const;
    std::string GenerateCircuitBreakerPanel() const;
    std::string GenerateRedundancyPanel() const;
    std::string GenerateChaosPanel() const;
    std::string GenerateMetricsPanel() const;
    
    mutable std::mutex m_panelsMutex;
    std::map<std::string, std::function<std::string()>> m_customPanels;
    
    std::atomic<bool> m_webServerRunning{false};
    std::thread m_webServerThread;
    uint16_t m_webServerPort{8080};
    
    void WebServerLoop();
    std::string HandleHTTPRequest(const std::string& request) const;
};

} // namespace Chaos
} // namespace NeonGlyph