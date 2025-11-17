#include "ResilienceDashboard.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace NeonGlyph {
namespace Chaos {

// MetricsCollector Implementation
MetricsCollector& MetricsCollector::GetInstance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::RecordMetric(const std::string& name, double value, MetricType type, 
                                   const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    MetricData data;
    data.name = name;
    data.type = type;
    data.value = value;
    data.timestamp = std::chrono::steady_clock::now();
    data.labels = labels;
    
    m_metrics[name].push_back(data);
    
    // Keep only recent metrics to prevent memory bloat
    if (m_metrics[name].size() > MAX_METRICS_PER_TYPE) {
        m_metrics[name].erase(m_metrics[name].begin(), 
                            m_metrics[name].begin() + (m_metrics[name].size() - MAX_METRICS_PER_TYPE));
    }
}

std::vector<MetricData> MetricsCollector::GetMetrics(const std::string& name, 
                                                     const std::map<std::string, std::string>& labels) const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    std::vector<MetricData> result;
    
    if (!name.empty()) {
        auto it = m_metrics.find(name);
        if (it != m_metrics.end()) {
            for (const auto& metric : it->second) {
                bool matches = true;
                for (const auto& label : labels) {
                    auto metricLabel = metric.labels.find(label.first);
                    if (metricLabel == metric.labels.end() || metricLabel->second != label.second) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    result.push_back(metric);
                }
            }
        }
    } else {
        // Return all metrics
        for (const auto& metricGroup : m_metrics) {
            for (const auto& metric : metricGroup.second) {
                bool matches = true;
                for (const auto& label : labels) {
                    auto metricLabel = metric.labels.find(label.first);
                    if (metricLabel == metric.labels.end() || metricLabel->second != label.second) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    result.push_back(metric);
                }
            }
        }
    }
    
    return result;
}

double MetricsCollector::GetMetricValue(const std::string& name, 
                                       const std::map<std::string, std::string>& labels) const {
    auto metrics = GetMetrics(name, labels);
    if (metrics.empty()) {
        return 0.0;
    }
    
    // Return the most recent value
    return metrics.back().value;
}

void MetricsCollector::ClearMetrics(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    if (!name.empty()) {
        m_metrics.erase(name);
    } else {
        m_metrics.clear();
    }
}

// HealthMonitor Implementation
HealthMonitor& HealthMonitor::GetInstance() {
    static HealthMonitor instance;
    return instance;
}

void HealthMonitor::RegisterHealthCheck(const std::string& component, HealthCheckFunction checkFunc) {
    std::lock_guard<std::mutex> lock(m_checksMutex);
    m_healthChecks[component] = checkFunc;
}

void HealthMonitor::UnregisterHealthCheck(const std::string& component) {
    std::lock_guard<std::mutex> lock(m_checksMutex);
    m_healthChecks.erase(component);
    m_lastResults.erase(component);
}

HealthStatus HealthMonitor::CheckComponent(const std::string& component) {
    std::lock_guard<std::mutex> lock(m_checksMutex);
    
    auto it = m_healthChecks.find(component);
    if (it == m_healthChecks.end()) {
        return HealthStatus::CRITICAL;
    }
    
    auto start = std::chrono::steady_clock::now();
    HealthStatus status = it->second();
    auto end = std::chrono::steady_clock::now();
    
    HealthCheck result;
    result.component = component;
    result.status = status;
    result.lastCheck = end;
    result.responseTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    
    switch (status) {
        case HealthStatus::HEALTHY:
            result.message = "Component is operating normally";
            break;
        case HealthStatus::DEGRADED:
            result.message = "Component is experiencing minor issues";
            break;
        case HealthStatus::UNHEALTHY:
            result.message = "Component is experiencing significant issues";
            break;
        case HealthStatus::CRITICAL:
            result.message = "Component is not responding or failed";
            break;
    }
    
    m_lastResults[component] = result;
    return status;
}

std::vector<HealthCheck> HealthMonitor::GetAllHealthChecks() const {
    std::lock_guard<std::mutex> lock(m_checksMutex);
    
    std::vector<HealthCheck> results;
    for (const auto& result : m_lastResults) {
        results.push_back(result.second);
    }
    
    return results;
}

HealthStatus HealthMonitor::GetOverallHealth() const {
    std::lock_guard<std::mutex> lock(m_checksMutex);
    
    if (m_lastResults.empty()) {
        return HealthStatus::CRITICAL;
    }
    
    std::vector<HealthStatus> statuses;
    for (const auto& result : m_lastResults) {
        statuses.push_back(result.second.status);
    }
    
    return AggregateHealthStatus(statuses);
}

double HealthMonitor::GetHealthScore() const {
    std::lock_guard<std::mutex> lock(m_checksMutex);
    
    if (m_lastResults.empty()) {
        return 0.0;
    }
    
    double totalScore = 0.0;
    for (const auto& result : m_lastResults) {
        switch (result.second.status) {
            case HealthStatus::HEALTHY:
                totalScore += 100.0;
                break;
            case HealthStatus::DEGRADED:
                totalScore += 75.0;
                break;
            case HealthStatus::UNHEALTHY:
                totalScore += 25.0;
                break;
            case HealthStatus::CRITICAL:
                totalScore += 0.0;
                break;
        }
    }
    
    return totalScore / m_lastResults.size();
}

void HealthMonitor::StartMonitoring(std::chrono::milliseconds interval) {
    if (m_monitoring.exchange(true)) {
        return; // Already monitoring
    }
    
    m_monitorThread = std::thread([this, interval]() {
        MonitorLoop(interval);
    });
}

void HealthMonitor::StopMonitoring() {
    if (!m_monitoring.exchange(false)) {
        return; // Not monitoring
    }
    
    if (m_monitorThread.joinable()) {
        m_monitorThread.join();
    }
}

void HealthMonitor::MonitorLoop(std::chrono::milliseconds interval) {
    while (m_monitoring.load()) {
        std::vector<std::string> components;
        {
            std::lock_guard<std::mutex> lock(m_checksMutex);
            for (const auto& check : m_healthChecks) {
                components.push_back(check.first);
            }
        }
        
        for (const auto& component : components) {
            CheckComponent(component);
        }
        
        std::this_thread::sleep_for(interval);
    }
}

HealthStatus HealthMonitor::AggregateHealthStatus(const std::vector<HealthStatus>& statuses) const {
    if (statuses.empty()) {
        return HealthStatus::CRITICAL;
    }
    
    size_t healthy = 0, degraded = 0, unhealthy = 0, critical = 0;
    
    for (const auto& status : statuses) {
        switch (status) {
            case HealthStatus::HEALTHY:
                healthy++;
                break;
            case HealthStatus::DEGRADED:
                degraded++;
                break;
            case HealthStatus::UNHEALTHY:
                unhealthy++;
                break;
            case HealthStatus::CRITICAL:
                critical++;
                break;
        }
    }
    
    // Determine overall health based on worst status present
    if (critical > 0) {
        return HealthStatus::CRITICAL;
    } else if (unhealthy > 0) {
        return HealthStatus::UNHEALTHY;
    } else if (degraded > 0) {
        return HealthStatus::DEGRADED;
    } else {
        return HealthStatus::HEALTHY;
    }
}

// CircuitBreakerMonitor Implementation
CircuitBreakerMonitor& CircuitBreakerMonitor::GetInstance() {
    static CircuitBreakerMonitor instance;
    return instance;
}

void CircuitBreakerMonitor::RegisterCircuitBreaker(const std::string& name, 
                                                  std::function<CircuitBreakerMetrics()> metricsFunc) {
    std::lock_guard<std::mutex> lock(m_breakersMutex);
    m_breakers[name] = metricsFunc;
}

void CircuitBreakerMonitor::UnregisterCircuitBreaker(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_breakersMutex);
    m_breakers.erase(name);
}

std::vector<CircuitBreakerMetrics> CircuitBreakerMonitor::GetAllCircuitBreakerMetrics() const {
    std::lock_guard<std::mutex> lock(m_breakersMutex);
    
    std::vector<CircuitBreakerMetrics> results;
    for (const auto& breaker : m_breakers) {
        results.push_back(breaker.second());
    }
    
    return results;
}

CircuitBreakerMetrics CircuitBreakerMonitor::GetCircuitBreakerMetrics(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_breakersMutex);
    
    auto it = m_breakers.find(name);
    if (it != m_breakers.end()) {
        return it->second();
    }
    
    return CircuitBreakerMetrics{};
}

std::vector<std::string> CircuitBreakerMonitor::GetOpenCircuitBreakers() const {
    std::lock_guard<std::mutex> lock(m_breakersMutex);
    
    std::vector<std::string> openBreakers;
    for (const auto& breaker : m_breakers) {
        auto metrics = breaker.second();
        if (metrics.state == "OPEN") {
            openBreakers.push_back(breaker.first);
        }
    }
    
    return openBreakers;
}

double CircuitBreakerMonitor::GetOverallCircuitBreakerHealth() const {
    std::lock_guard<std::mutex> lock(m_breakersMutex);
    
    if (m_breakers.empty()) {
        return 100.0;
    }
    
    double totalHealth = 0.0;
    for (const auto& breaker : m_breakers) {
        auto metrics = breaker.second();
        
        // Calculate health based on failure rate and state
        double health = 100.0;
        if (metrics.state == "OPEN") {
            health = 0.0;
        } else if (metrics.state == "HALF_OPEN") {
            health = 50.0;
        } else {
            // CLOSED state - health based on failure rate
            health = std::max(0.0, 100.0 - (metrics.failureRate * 100.0));
        }
        
        totalHealth += health;
    }
    
    return totalHealth / m_breakers.size();
}

// RedundancyMonitor Implementation
RedundancyMonitor& RedundancyMonitor::GetInstance() {
    static RedundancyMonitor instance;
    return instance;
}

void RedundancyMonitor::RegisterRedundancySystem(const std::string& systemType,
                                                std::function<RedundancyMetrics()> metricsFunc) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    m_systems[systemType] = metricsFunc;
}

void RedundancyMonitor::UnregisterRedundancySystem(const std::string& systemType) {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    m_systems.erase(systemType);
}

std::vector<RedundancyMetrics> RedundancyMonitor::GetAllRedundancyMetrics() const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    std::vector<RedundancyMetrics> results;
    for (const auto& system : m_systems) {
        results.push_back(system.second());
    }
    
    return results;
}

RedundancyMetrics RedundancyMonitor::GetRedundancyMetrics(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto it = m_systems.find(systemType);
    if (it != m_systems.end()) {
        return it->second();
    }
    
    return RedundancyMetrics{};
}

std::vector<std::string> RedundancyMonitor::GetSystemsWithActiveFailover() const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    std::vector<std::string> activeFailovers;
    for (const auto& system : m_systems) {
        auto metrics = system.second();
        if (metrics.failoverActive) {
            activeFailovers.push_back(system.first);
        }
    }
    
    return activeFailovers;
}

bool RedundancyMonitor::IsRedundancyHealthy(const std::string& systemType) const {
    std::lock_guard<std::mutex> lock(m_systemsMutex);
    
    auto it = m_systems.find(systemType);
    if (it != m_systems.end()) {
        auto metrics = it->second();
        return metrics.healthyInstances > 0;
    }
    
    return false;
}

// ChaosExperimentMonitor Implementation
ChaosExperimentMonitor& ChaosExperimentMonitor::GetInstance() {
    static ChaosExperimentMonitor instance;
    return instance;
}

void ChaosExperimentMonitor::RecordExperimentStart(const std::string& experimentId, 
                                                  const std::string& experimentType,
                                                  const std::map<std::string, std::string>& parameters) {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    ChaosMetrics metrics;
    metrics.experimentId = experimentId;
    metrics.experimentType = experimentType;
    metrics.status = "RUNNING";
    metrics.startTime = std::chrono::steady_clock::now();
    metrics.parameters = parameters;
    
    m_experiments[experimentId] = metrics;
    m_experimentOrder.push_back(experimentId);
    
    // Maintain history size limit
    if (m_experimentOrder.size() > MAX_HISTORY_SIZE) {
        std::string oldestId = m_experimentOrder.front();
        m_experimentOrder.pop_front();
        m_experiments.erase(oldestId);
    }
}

void ChaosExperimentMonitor::RecordExperimentEnd(const std::string& experimentId, 
                                                const std::string& status,
                                                const std::string& impactAssessment) {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    auto it = m_experiments.find(experimentId);
    if (it != m_experiments.end()) {
        it->second.status = status;
        it->second.endTime = std::chrono::steady_clock::now();
        it->second.impactAssessment = impactAssessment;
    }
}

std::vector<ChaosMetrics> ChaosExperimentMonitor::GetExperimentHistory(size_t limit) const {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    std::vector<ChaosMetrics> history;
    
    // Start from the most recent experiments
    for (auto it = m_experimentOrder.rbegin(); it != m_experimentOrder.rend() && history.size() < limit; ++it) {
        auto expIt = m_experiments.find(*it);
        if (expIt != m_experiments.end()) {
            history.push_back(expIt->second);
        }
    }
    
    return history;
}

std::vector<ChaosMetrics> ChaosExperimentMonitor::GetActiveExperiments() const {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    std::vector<ChaosMetrics> active;
    for (const auto& experiment : m_experiments) {
        if (experiment.second.status == "RUNNING") {
            active.push_back(experiment.second);
        }
    }
    
    return active;
}

ChaosMetrics ChaosExperimentMonitor::GetExperimentMetrics(const std::string& experimentId) const {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    auto it = m_experiments.find(experimentId);
    if (it != m_experiments.end()) {
        return it->second;
    }
    
    return ChaosMetrics{};
}

std::map<std::string, size_t> ChaosExperimentMonitor::GetExperimentTypeCounts() const {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    std::map<std::string, size_t> counts;
    for (const auto& experiment : m_experiments) {
        counts[experiment.second.experimentType]++;
    }
    
    return counts;
}

double ChaosExperimentMonitor::GetExperimentSuccessRate() const {
    std::lock_guard<std::mutex> lock(m_experimentsMutex);
    
    if (m_experiments.empty()) {
        return 0.0;
    }
    
    size_t completed = 0;
    size_t successful = 0;
    
    for (const auto& experiment : m_experiments) {
        if (experiment.second.status != "RUNNING") {
            completed++;
            if (experiment.second.status == "SUCCESS") {
                successful++;
            }
        }
    }
    
    return completed > 0 ? (static_cast<double>(successful) / completed) * 100.0 : 0.0;
}

// ResilienceDashboard Implementation
ResilienceDashboard& ResilienceDashboard::GetInstance() {
    static ResilienceDashboard instance;
    return instance;
}

void ResilienceDashboard::Initialize() {
    // Initialize monitoring systems
    HealthMonitor::GetInstance().StartMonitoring();
    
    // Register default health checks for NeonGlyph systems
    HealthMonitor::GetInstance().RegisterHealthCheck("VulkanContext", []() {
        // Check Vulkan context health
        return HealthStatus::HEALTHY; // Placeholder
    });
    
    HealthMonitor::GetInstance().RegisterHealthCheck("AudioEngine", []() {
        // Check audio engine health
        return HealthStatus::HEALTHY; // Placeholder
    });
    
    HealthMonitor::GetInstance().RegisterHealthCheck("AIEngine", []() {
        // Check AI engine health
        return HealthStatus::HEALTHY; // Placeholder
    });
}

void ResilienceDashboard::Shutdown() {
    StopWebServer();
    HealthMonitor::GetInstance().StopMonitoring();
}

void ResilienceDashboard::UpdateDashboard() {
    // Update all monitoring systems
    HealthMonitor::GetInstance().CheckComponent("VulkanContext");
    HealthMonitor::GetInstance().CheckComponent("AudioEngine");
    HealthMonitor::GetInstance().CheckComponent("AIEngine");
}

std::string ResilienceDashboard::GenerateDashboardHTML() const {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>NeonGlyph Resilience Dashboard</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    html << ".dashboard { max-width: 1200px; margin: 0 auto; }\n";
    html << ".panel { background: white; border-radius: 8px; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html << ".panel h2 { color: #333; border-bottom: 2px solid #007acc; padding-bottom: 10px; }\n";
    html << ".metric { display: inline-block; margin: 10px 20px 10px 0; }\n";
    html << ".metric-value { font-size: 24px; font-weight: bold; color: #007acc; }\n";
    html << ".metric-label { font-size: 14px; color: #666; }\n";
    html << ".status-healthy { color: #28a745; }\n";
    html << ".status-degraded { color: #ffc107; }\n";
    html << ".status-unhealthy { color: #fd7e14; }\n";
    html << ".status-critical { color: #dc3545; }\n";
    html << ".table { width: 100%; border-collapse: collapse; margin-top: 10px; }\n";
    html << ".table th, .table td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html << ".table th { background-color: #f8f9fa; font-weight: bold; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    
    html << "<div class=\"dashboard\">\n";
    html << "<h1>NeonGlyph Resilience Dashboard</h1>\n";
    html << "<p>Last updated: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() << "</p>\n";
    
    // Overview Panel
    html << GenerateOverviewPanel();
    
    // Health Panel
    html << GenerateHealthPanel();
    
    // Circuit Breaker Panel
    html << GenerateCircuitBreakerPanel();
    
    // Redundancy Panel
    html << GenerateRedundancyPanel();
    
    // Chaos Panel
    html << GenerateChaosPanel();
    
    // Metrics Panel
    html << GenerateMetricsPanel();
    
    // Custom Panels
    std::lock_guard<std::mutex> lock(m_panelsMutex);
    for (const auto& panel : m_customPanels) {
        html << "<div class=\"panel\">\n";
        html << "<h2>" << panel.first << "</h2>\n";
        html << panel.second();
        html << "</div>\n";
    }
    
    html << "</div>\n";
    html << "</body>\n</html>\n";
    
    return html.str();
}

std::string ResilienceDashboard::GenerateDashboardJSON() const {
    std::stringstream json;
    
    json << "{\n";
    json << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() << ",\n";
    
    // Overall health
    auto overallHealth = HealthMonitor::GetInstance().GetOverallHealth();
    json << "  \"overall_health\": \"";
    switch (overallHealth) {
        case HealthStatus::HEALTHY:
            json << "healthy";
            break;
        case HealthStatus::DEGRADED:
            json << "degraded";
            break;
        case HealthStatus::UNHEALTHY:
            json << "unhealthy";
            break;
        case HealthStatus::CRITICAL:
            json << "critical";
            break;
    }
    json << "\",\n";
    
    json << "  \"health_score\": " << HealthMonitor::GetInstance().GetHealthScore() << ",\n";
    json << "  \"circuit_breaker_health\": " << CircuitBreakerMonitor::GetInstance().GetOverallCircuitBreakerHealth() << ",\n";
    json << "  \"experiment_success_rate\": " << ChaosExperimentMonitor::GetInstance().GetExperimentSuccessRate() << "\n";
    
    json << "}\n";
    
    return json.str();
}

void ResilienceDashboard::StartWebServer(uint16_t port) {
    if (m_webServerRunning.exchange(true)) {
        return; // Already running
    }
    
    m_webServerPort = port;
    m_webServerThread = std::thread([this]() {
        WebServerLoop();
    });
}

void ResilienceDashboard::StopWebServer() {
    if (!m_webServerRunning.exchange(false)) {
        return; // Not running
    }
    
    if (m_webServerThread.joinable()) {
        m_webServerThread.join();
    }
}

void ResilienceDashboard::RegisterCustomPanel(const std::string& name, 
                                            std::function<std::string()> contentGenerator) {
    std::lock_guard<std::mutex> lock(m_panelsMutex);
    m_customPanels[name] = contentGenerator;
}

std::vector<std::string> ResilienceDashboard::GetAvailablePanels() const {
    std::lock_guard<std::mutex> lock(m_panelsMutex);
    
    std::vector<std::string> panels;
    for (const auto& panel : m_customPanels) {
        panels.push_back(panel.first);
    }
    
    return panels;
}

std::string ResilienceDashboard::GetPanelContent(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_panelsMutex);
    
    auto it = m_customPanels.find(name);
    if (it != m_customPanels.end()) {
        return it->second();
    }
    
    return "Panel not found";
}

std::string ResilienceDashboard::GenerateOverviewPanel() const {
    std::stringstream html;
    
    html << "<div class=\"panel\">\n";
    html << "<h2>System Overview</h2>\n";
    
    auto overallHealth = HealthMonitor::GetInstance().GetOverallHealth();
    double healthScore = HealthMonitor::GetInstance().GetHealthScore();
    double cbHealth = CircuitBreakerMonitor::GetInstance().GetOverallCircuitBreakerHealth();
    double expSuccessRate = ChaosExperimentMonitor::GetInstance().GetExperimentSuccessRate();
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value ";
    switch (overallHealth) {
        case HealthStatus::HEALTHY:
            html << "status-healthy";
            break;
        case HealthStatus::DEGRADED:
            html << "status-degraded";
            break;
        case HealthStatus::UNHEALTHY:
            html << "status-unhealthy";
            break;
        case HealthStatus::CRITICAL:
            html << "status-critical";
            break;
    }
    html << "\">";
    
    switch (overallHealth) {
        case HealthStatus::HEALTHY:
            html << "HEALTHY";
            break;
        case HealthStatus::DEGRADED:
            html << "DEGRADED";
            break;
        case HealthStatus::UNHEALTHY:
            html << "UNHEALTHY";
            break;
        case HealthStatus::CRITICAL:
            html << "CRITICAL";
            break;
    }
    html << "</div>\n";
    html << "<div class=\"metric-label\">Overall Health</div>\n";
    html << "</div>\n";
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << std::fixed << std::setprecision(1) << healthScore << "%</div>\n";
    html << "<div class=\"metric-label\">Health Score</div>\n";
    html << "</div>\n";
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << std::fixed << std::setprecision(1) << cbHealth << "%</div>\n";
    html << "<div class=\"metric-label\">Circuit Breaker Health</div>\n";
    html << "</div>\n";
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << std::fixed << std::setprecision(1) << expSuccessRate << "%</div>\n";
    html << "<div class=\"metric-label\">Experiment Success Rate</div>\n";
    html << "</div>\n";
    
    html << "</div>\n";
    
    return html.str();
}

std::string ResilienceDashboard::GenerateHealthPanel() const {
    std::stringstream html;
    
    html << "<div class=\"panel\">\n";
    html << "<h2>Component Health</h2>\n";
    
    auto healthChecks = HealthMonitor::GetInstance().GetAllHealthChecks();
    
    if (healthChecks.empty()) {
        html << "<p>No health checks registered.</p>\n";
    } else {
        html << "<table class=\"table\">\n";
        html << "<tr><th>Component</th><th>Status</th><th>Response Time</th><th>Message</th></tr>\n";
        
        for (const auto& check : healthChecks) {
            html << "<tr>\n";
            html << "<td>" << check.component << "</td>\n";
            html << "<td class=\"";
            switch (check.status) {
                case HealthStatus::HEALTHY:
                    html << "status-healthy\"">HEALTHY";
                    break;
                case HealthStatus::DEGRADED:
                    html << "status-degraded\"">DEGRADED";
                    break;
                case HealthStatus::UNHEALTHY:
                    html << "status-unhealthy\"">UNHEALTHY";
                    break;
                case HealthStatus::CRITICAL:
                    html << "status-critical\"">CRITICAL";
                    break;
            }
            html << "</td>\n";
            html << "<td>" << std::fixed << std::setprecision(2) << check.responseTimeMs << " ms</td>\n";
            html << "<td>" << check.message << "</td>\n";
            html << "</tr>\n";
        }
        
        html << "</table>\n";
    }
    
    html << "</div>\n";
    
    return html.str();
}

std::string ResilienceDashboard::GenerateCircuitBreakerPanel() const {
    std::stringstream html;
    
    html << "<div class=\"panel\">\n";
    html << "<h2>Circuit Breakers</h2>\n";
    
    auto cbMetrics = CircuitBreakerMonitor::GetInstance().GetAllCircuitBreakerMetrics();
    
    if (cbMetrics.empty()) {
        html << "<p>No circuit breakers registered.</p>\n";
    } else {
        html << "<table class=\"table\">\n";
        html << "<tr><th>Name</th><th>State</th><th>Success Count</th><th>Failure Count</th><th>Failure Rate</th></tr>\n";
        
        for (const auto& metrics : cbMetrics) {
            html << "<tr>\n";
            html << "<td>" << metrics.name << "</td>\n";
            html << "<td>" << metrics.state << "</td>\n";
            html << "<td>" << metrics.successCount << "</td>\n";
            html << "<td>" << metrics.failureCount << "</td>\n";
            html << "<td>" << std::fixed << std::setprecision(2) << (metrics.failureRate * 100) << "%</td>\n";
            html << "</tr>\n";
        }
        
        html << "</table>\n";
    }
    
    html << "</div>\n";
    
    return html.str();
}

std::string ResilienceDashboard::GenerateRedundancyPanel() const {
    std::stringstream html;
    
    html << "<div class=\"panel\">\n";
    html << "<h2>Redundancy Systems</h2>\n";
    
    auto redundancyMetrics = RedundancyMonitor::GetInstance().GetAllRedundancyMetrics();
    
    if (redundancyMetrics.empty()) {
        html << "<p>No redundancy systems registered.</p>\n";
    } else {
        html << "<table class=\"table\">\n";
        html << "<tr><th>System Type</th><th>Total Instances</th><th>Healthy</th><th>Failed</th><th>Redundancy Level</th><th>Failover Active</th></tr>\n";
        
        for (const auto& metrics : redundancyMetrics) {
            html << "<tr>\n";
            html << "<td>" << metrics.systemType << "</td>\n";
            html << "<td>" << metrics.totalInstances << "</td>\n";
            html << "<td class=\"status-healthy\">" << metrics.healthyInstances << "</td>\n";
            html << "<td class=\"";
            if (metrics.failedInstances > 0) {
                html << "status-critical";
            } else {
                html << "status-healthy";
            }
            html << "\">" << metrics.failedInstances << "</td>\n";
            html << "<td>" << metrics.redundancyLevel << "</td>\n";
            html << "<td>" << (metrics.failoverActive ? "Yes" : "No") << "</td>\n";
            html << "</tr>\n";
        }
        
        html << "</table>\n";
    }
    
    html << "</div>\n";
    
    return html.str();
}

std::string ResilienceDashboard::GenerateChaosPanel() const {
    std::stringstream html;
    
    html << "<div class=\"panel\">\n";
    html << "<h2>Chaos Experiments</h2>\n";
    
    auto activeExperiments = ChaosExperimentMonitor::GetInstance().GetActiveExperiments();
    auto experimentHistory = ChaosExperimentMonitor::GetInstance().GetExperimentHistory(10);
    
    if (!activeExperiments.empty()) {
        html << "<h3>Active Experiments</h3>\n";
        html << "<table class=\"table\">\n";
        html << "<tr><th>Experiment ID</th><th>Type</th><th>Status</th><th>Start Time</th></tr>\n";
        
        for (const auto& exp : activeExperiments) {
            html << "<tr>\n";
            html << "<td>" << exp.experimentId << "</td>\n";
            html << "<td>" << exp.experimentType << "</td>\n";
            html << "<td>" << exp.status << "</td>\n";
            html << "<td>" << std::chrono::duration_cast<std::chrono::seconds>(
                exp.startTime.time_since_epoch()).count() << "</td>\n";
            html << "</tr>\n";
        }
        
        html << "</table>\n";
    }
    
    if (!experimentHistory.empty()) {
        html << "<h3>Recent Experiments</h3>\n";
        html << "<table class=\"table\">\n";
        html << "<tr><th>Experiment ID</th><th>Type</th><th>Status</th><th>Impact Assessment</th></tr>\n";
        
        for (const auto& exp : experimentHistory) {
            html << "<tr>\n";
            html << "<td>" << exp.experimentId << "</td>\n";
            html << "<td>" << exp.experimentType << "</td>\n";
            html << "<td>" << exp.status << "</td>\n";
            html << "<td>" << exp.impactAssessment << "</td>\n";
            html << "</tr>\n";
        }
        
        html << "</table>\n";
    }
    
    if (activeExperiments.empty() && experimentHistory.empty()) {
        html << "<p>No chaos experiments recorded.</p>\n";
    }
    
    html << "</div>\n";
    
    return html.str();
}

std::string ResilienceDashboard::GenerateMetricsPanel() const {
    std::stringstream html;
    
    html << "<div class=\"panel\">\n";
    html << "<h2>Key Metrics</h2>\n";
    
    // Get some key metrics from MetricsCollector
    auto& collector = MetricsCollector::GetInstance();
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << collector.GetMetricValue("system_uptime_seconds") << "</div>\n";
    html << "<div class=\"metric-label\">System Uptime (seconds)</div>\n";
    html << "</div>\n";
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << collector.GetMetricValue("total_requests") << "</div>\n";
    html << "<div class=\"metric-label\">Total Requests</div>\n";
    html << "</div>\n";
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << collector.GetMetricValue("error_rate") << "%</div>\n";
    html << "<div class=\"metric-label\">Error Rate</div>\n";
    html << "</div>\n";
    
    html << "<div class=\"metric\">\n";
    html << "<div class=\"metric-value\">" << collector.GetMetricValue("average_response_time_ms") << "</div>\n";
    html << "<div class=\"metric-label\">Avg Response Time (ms)</div>\n";
    html << "</div>\n";
    
    html << "</div>\n";
    
    return html.str();
}

void ResilienceDashboard::WebServerLoop() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket for web server" << std::endl;
        return;
    }
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_webServerPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket for web server" << std::endl;
        closesocket(serverSocket);
        return;
    }
    
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Failed to listen on socket for web server" << std::endl;
        closesocket(serverSocket);
        return;
    }
    
    std::cout << "Resilience Dashboard Web Server started on port " << m_webServerPort << std::endl;
    
    while (m_webServerRunning.load()) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        
        // Read HTTP request
        char buffer[4096];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string request(buffer);
            
            std::string response = HandleHTTPRequest(request);
            std::string httpResponse = "HTTP/1.1 200 OK\r\n";
            httpResponse += "Content-Type: text/html\r\n";
            httpResponse += "Content-Length: " + std::to_string(response.length()) + "\r\n";
            httpResponse += "Connection: close\r\n\r\n";
            httpResponse += response;
            
            send(clientSocket, httpResponse.c_str(), static_cast<int>(httpResponse.length()), 0);
        }
        
        closesocket(clientSocket);
    }
    
    closesocket(serverSocket);
    
#ifdef _WIN32
    WSACleanup();
#endif
}

std::string ResilienceDashboard::HandleHTTPRequest(const std::string& request) const {
    // Simple HTTP request parsing
    if (request.find("GET /json") != std::string::npos) {
        return GenerateDashboardJSON();
    } else {
        return GenerateDashboardHTML();
    }
}

} // namespace Chaos
} // namespace NeonGlyph