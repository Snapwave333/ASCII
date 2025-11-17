#include "LoadTestingFramework.h"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace NeonGlyph {
namespace Chaos {

// PerformanceAnalyzer Implementation
PerformanceAnalyzer::PerformanceAnalyzer() {
}

PerformanceAnalyzer::~PerformanceAnalyzer() {
}

void PerformanceAnalyzer::AddPerformanceData(const std::string& testId, const std::vector<PerformanceSnapshot>& snapshots) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_testData[testId] = snapshots;
}

void PerformanceAnalyzer::AnalyzeTrends(const std::string& testId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.empty()) {
        return;
    }
    
    // Analyze trends for each metric
    std::map<PerformanceMetric, std::vector<double>> metricTimeSeries;
    
    for (const auto& snapshot : snapshots) {
        for (const auto& metric : snapshot.metrics) {
            metricTimeSeries[metric.first].push_back(metric.second);
        }
    }
    
    // Calculate trends
    for (const auto& pair : metricTimeSeries) {
        double trend = CalculateTrend(pair.second);
        double volatility = CalculateVolatility(pair.second);
        
        // Store trend analysis results
        // This could be stored in a separate data structure for later retrieval
    }
}

void PerformanceAnalyzer::IdentifyBottlenecks(const std::string& testId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.empty()) {
        return;
    }
    
    // Identify component bottlenecks
    std::string bottleneckComponent = IdentifyComponentBottleneck(snapshots);
    
    // Calculate bottleneck severity
    double maxCpuUsage = 0.0;
    double maxMemoryUsage = 0.0;
    double maxGpuUsage = 0.0;
    
    for (const auto& snapshot : snapshots) {
        maxCpuUsage = std::max(maxCpuUsage, snapshot.cpuUsage);
        maxMemoryUsage = std::max(maxMemoryUsage, snapshot.memoryUsage);
        maxGpuUsage = std::max(maxGpuUsage, snapshot.gpuUsage);
    }
    
    std::map<std::string, double> bottleneckAnalysis;
    bottleneckAnalysis["cpu_bottleneck"] = maxCpuUsage;
    bottleneckAnalysis["memory_bottleneck"] = maxMemoryUsage;
    bottleneckAnalysis["gpu_bottleneck"] = maxGpuUsage;
    bottleneckAnalysis["primary_bottleneck"] = bottleneckComponent == "CPU" ? maxCpuUsage : 
                                            bottleneckComponent == "MEMORY" ? maxMemoryUsage : maxGpuUsage;
    
    m_bottleneckAnalysis[testId] = bottleneckAnalysis;
}

void PerformanceAnalyzer::CalculateScalabilityMetrics(const std::string& testId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.size() < 2) {
        return;
    }
    
    // Calculate throughput trend
    double throughputTrend = CalculateThroughputTrend(snapshots);
    
    // Calculate response time trend
    std::vector<double> responseTimes;
    for (const auto& snapshot : snapshots) {
        auto it = snapshot.metrics.find(PerformanceMetric::RESPONSE_TIME);
        if (it != snapshot.metrics.end()) {
            responseTimes.push_back(it->second);
        }
    }
    
    double responseTimeTrend = CalculateTrend(responseTimes);
    
    // Calculate scalability metrics
    std::map<std::string, double> scalabilityMetrics;
    scalabilityMetrics["throughput_trend"] = throughputTrend;
    scalabilityMetrics["response_time_trend"] = responseTimeTrend;
    scalabilityMetrics["scalability_factor"] = throughputTrend / std::abs(responseTimeTrend + 0.001); // Avoid division by zero
    
    // Calculate 95th percentile response time
    if (!responseTimes.empty()) {
        scalabilityMetrics["p95_response_time"] = CalculateResponseTimePercentile(responseTimes, 0.95);
        scalabilityMetrics["p99_response_time"] = CalculateResponseTimePercentile(responseTimes, 0.99);
    }
    
    m_scalabilityMetrics[testId] = scalabilityMetrics;
}

void PerformanceAnalyzer::PredictPerformance(const std::string& testId, std::chrono::seconds futureTime) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.size() < 3) {
        return;
    }
    
    // Simple linear prediction based on recent trends
    auto lastSnapshot = snapshots.back();
    auto firstSnapshot = snapshots.front();
    auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(lastSnapshot.timestamp - firstSnapshot.timestamp);
    
    if (totalDuration.count() == 0) {
        return;
    }
    
    // Predict each metric
    for (const auto& metric : lastSnapshot.metrics) {
        std::vector<double> values;
        for (const auto& snapshot : snapshots) {
            auto it = snapshot.metrics.find(metric.first);
            if (it != snapshot.metrics.end()) {
                values.push_back(it->second);
            }
        }
        
        if (values.size() >= 2) {
            double trend = CalculateTrend(values);
            double predictedValue = metric.second + (trend * futureTime.count() / totalDuration.count());
            
            // Store prediction (implementation depends on requirements)
        }
    }
}

std::string PerformanceAnalyzer::GeneratePerformanceInsights(const std::string& testId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return "No data available for test: " + testId;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.empty()) {
        return "No snapshot data available for test: " + testId;
    }
    
    std::stringstream insights;
    insights << "Performance Insights for Test: " << testId << "\n";
    insights << "=" << std::string(50, '=') << "\n\n";
    
    // Analyze key metrics
    std::vector<double> responseTimes;
    std::vector<double> cpuUsages;
    std::vector<double> memoryUsages;
    std::vector<double> throughputs;
    
    for (const auto& snapshot : snapshots) {
        // Extract response times
        auto it = snapshot.metrics.find(PerformanceMetric::RESPONSE_TIME);
        if (it != snapshot.metrics.end()) {
            responseTimes.push_back(it->second);
        }
        
        // Extract CPU usage
        cpuUsages.push_back(snapshot.cpuUsage);
        memoryUsages.push_back(snapshot.memoryUsage);
        
        // Extract throughput
        it = snapshot.metrics.find(PerformanceMetric::THROUGHPUT);
        if (it != snapshot.metrics.end()) {
            throughputs.push_back(it->second);
        }
    }
    
    if (!responseTimes.empty()) {
        double avgResponseTime = std::accumulate(responseTimes.begin(), responseTimes.end(), 0.0) / responseTimes.size();
        double maxResponseTime = *std::max_element(responseTimes.begin(), responseTimes.end());
        double p95ResponseTime = CalculateResponseTimePercentile(responseTimes, 0.95);
        
        insights << "Response Time Analysis:\n";
        insights << "  Average: " << std::fixed << std::setprecision(2) << avgResponseTime << "ms\n";
        insights << "  Peak: " << maxResponseTime << "ms\n";
        insights << "  95th Percentile: " << p95ResponseTime << "ms\n";
        insights << "  Trend: " << (CalculateTrend(responseTimes) > 0 ? "Increasing" : "Decreasing") << "\n\n";
    }
    
    if (!cpuUsages.empty()) {
        double avgCpuUsage = std::accumulate(cpuUsages.begin(), cpuUsages.end(), 0.0) / cpuUsages.size();
        double maxCpuUsage = *std::max_element(cpuUsages.begin(), cpuUsages.end());
        
        insights << "CPU Usage Analysis:\n";
        insights << "  Average: " << std::fixed << std::setprecision(1) << avgCpuUsage << "%\n";
        insights << "  Peak: " << maxCpuUsage << "%\n";
        insights << "  Trend: " << (CalculateTrend(cpuUsages) > 0 ? "Increasing" : "Decreasing") << "\n\n";
    }
    
    if (!memoryUsages.empty()) {
        double avgMemoryUsage = std::accumulate(memoryUsages.begin(), memoryUsages.end(), 0.0) / memoryUsages.size();
        double maxMemoryUsage = *std::max_element(memoryUsages.begin(), memoryUsages.end());
        
        insights << "Memory Usage Analysis:\n";
        insights << "  Average: " << std::fixed << std::setprecision(1) << avgMemoryUsage << "%\n";
        insights << "  Peak: " << maxMemoryUsage << "%\n";
        insights << "  Trend: " << (CalculateTrend(memoryUsages) > 0 ? "Increasing" : "Decreasing") << "\n\n";
    }
    
    if (!throughputs.empty()) {
        double avgThroughput = std::accumulate(throughputs.begin(), throughputs.end(), 0.0) / throughputs.size();
        double maxThroughput = *std::max_element(throughputs.begin(), throughputs.end());
        
        insights << "Throughput Analysis:\n";
        insights << "  Average: " << std::fixed << std::setprecision(2) << avgThroughput << " req/s\n";
        insights << "  Peak: " << maxThroughput << " req/s\n";
        insights << "  Trend: " << (CalculateTrend(throughputs) > 0 ? "Increasing" : "Decreasing") << "\n\n";
    }
    
    // Bottleneck analysis
    auto bottleneckIt = m_bottleneckAnalysis.find(testId);
    if (bottleneckIt != m_bottleneckAnalysis.end()) {
        insights << "Bottleneck Analysis:\n";
        for (const auto& bottleneck : bottleneckIt->second) {
            insights << "  " << bottleneck.first << ": " << std::fixed << std::setprecision(1) << bottleneck.second << "%\n";
        }
        insights << "\n";
    }
    
    // Scalability metrics
    auto scalabilityIt = m_scalabilityMetrics.find(testId);
    if (scalabilityIt != m_scalabilityMetrics.end()) {
        insights << "Scalability Metrics:\n";
        for (const auto& metric : scalabilityIt->second) {
            insights << "  " << metric.first << ": " << std::fixed << std::setprecision(3) << metric.second << "\n";
        }
        insights << "\n";
    }
    
    return insights.str();
}

std::string PerformanceAnalyzer::GenerateOptimizationRecommendations(const std::string& testId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return "No data available for test: " + testId;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.empty()) {
        return "No snapshot data available for test: " + testId;
    }
    
    std::vector<std::string> recommendations;
    
    // Analyze CPU usage
    std::vector<double> cpuUsages;
    for (const auto& snapshot : snapshots) {
        cpuUsages.push_back(snapshot.cpuUsage);
    }
    
    if (!cpuUsages.empty()) {
        double avgCpuUsage = std::accumulate(cpuUsages.begin(), cpuUsages.end(), 0.0) / cpuUsages.size();
        double maxCpuUsage = *std::max_element(cpuUsages.begin(), cpuUsages.end());
        
        if (maxCpuUsage > 80.0) {
            recommendations.push_back("CPU bottleneck detected. Consider optimizing CPU-intensive operations or scaling horizontally.");
        }
        
        if (avgCpuUsage > 70.0) {
            recommendations.push_back("High average CPU usage. Consider code optimization or caching strategies.");
        }
    }
    
    // Analyze memory usage
    std::vector<double> memoryUsages;
    for (const auto& snapshot : snapshots) {
        memoryUsages.push_back(snapshot.memoryUsage);
    }
    
    if (!memoryUsages.empty()) {
        double maxMemoryUsage = *std::max_element(memoryUsages.begin(), memoryUsages.end());
        
        if (maxMemoryUsage > 85.0) {
            recommendations.push_back("High memory usage detected. Consider memory optimization or garbage collection tuning.");
        }
    }
    
    // Analyze response times
    std::vector<double> responseTimes;
    for (const auto& snapshot : snapshots) {
        auto it = snapshot.metrics.find(PerformanceMetric::RESPONSE_TIME);
        if (it != snapshot.metrics.end()) {
            responseTimes.push_back(it->second);
        }
    }
    
    if (!responseTimes.empty()) {
        double p95ResponseTime = CalculateResponseTimePercentile(responseTimes, 0.95);
        
        if (p95ResponseTime > 1000) { // 1 second
            recommendations.push_back("High 95th percentile response time. Consider database optimization or async processing.");
        }
        
        double responseTimeTrend = CalculateTrend(responseTimes);
        if (responseTimeTrend > 0.1) { // Increasing trend
            recommendations.push_back("Response time is trending upward. Monitor for memory leaks or resource exhaustion.");
        }
    }
    
    // Analyze throughput
    std::vector<double> throughputs;
    for (const auto& snapshot : snapshots) {
        auto it = snapshot.metrics.find(PerformanceMetric::THROUGHPUT);
        if (it != snapshot.metrics.end()) {
            throughputs.push_back(it->second);
        }
    }
    
    if (!throughputs.empty()) {
        double throughputTrend = CalculateTrend(throughputs);
        if (throughputTrend < -0.05) { // Decreasing trend
            recommendations.push_back("Throughput is declining. Investigate resource contention or scaling issues.");
        }
    }
    
    // Generate recommendations report
    std::stringstream report;
    report << "Optimization Recommendations for Test: " << testId << "\n";
    report << "=" << std::string(50, '=') << "\n\n";
    
    if (recommendations.empty()) {
        report << "No significant performance issues detected. System appears to be performing well.\n";
    } else {
        for (size_t i = 0; i < recommendations.size(); ++i) {
            report << (i + 1) << ". " << recommendations[i] << "\n";
        }
    }
    
    m_optimizationSuggestions[testId] = recommendations;
    
    return report.str();
}

std::string PerformanceAnalyzer::GenerateCapacityPlanningReport(const std::string& testId) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_testData.find(testId);
    if (it == m_testData.end()) {
        return "No data available for test: " + testId;
    }
    
    const auto& snapshots = it->second;
    if (snapshots.empty()) {
        return "No snapshot data available for test: " + testId;
    }
    
    // Calculate current capacity utilization
    std::vector<double> cpuUsages;
    std::vector<double> memoryUsages;
    std::vector<double> throughputs;
    
    for (const auto& snapshot : snapshots) {
        cpuUsages.push_back(snapshot.cpuUsage);
        memoryUsages.push_back(snapshot.memoryUsage);
        
        auto it = snapshot.metrics.find(PerformanceMetric::THROUGHPUT);
        if (it != snapshot.metrics.end()) {
            throughputs.push_back(it->second);
        }
    }
    
    if (cpuUsages.empty() || memoryUsages.empty() || throughputs.empty()) {
        return "Insufficient data for capacity planning analysis.";
    }
    
    double avgCpuUsage = std::accumulate(cpuUsages.begin(), cpuUsages.end(), 0.0) / cpuUsages.size();
    double avgMemoryUsage = std::accumulate(memoryUsages.begin(), memoryUsages.end(), 0.0) / memoryUsages.size();
    double avgThroughput = std::accumulate(throughputs.begin(), throughputs.end(), 0.0) / throughputs.size();
    double maxThroughput = *std::max_element(throughputs.begin(), throughputs.end());
    
    // Calculate capacity headroom
    double cpuHeadroom = 100.0 - avgCpuUsage;
    double memoryHeadroom = 100.0 - avgMemoryUsage;
    double throughputHeadroom = maxThroughput > 0 ? (maxThroughput - avgThroughput) / maxThroughput * 100.0 : 0.0;
    
    // Estimate capacity limits
    double cpuLimitedThroughput = avgThroughput * (100.0 / avgCpuUsage);
    double memoryLimitedThroughput = avgThroughput * (100.0 / avgMemoryUsage);
    double theoreticalMaxThroughput = std::min(cpuLimitedThroughput, memoryLimitedThroughput);
    
    // Generate capacity planning report
    std::stringstream report;
    report << "Capacity Planning Report for Test: " << testId << "\n";
    report << "=" << std::string(50, '=') << "\n\n";
    
    report << "Current Utilization:\n";
    report << "  CPU: " << std::fixed << std::setprecision(1) << avgCpuUsage << "%\n";
    report << "  Memory: " << avgMemoryUsage << "%\n";
    report << "  Throughput: " << std::setprecision(2) << avgThroughput << " req/s\n\n";
    
    report << "Capacity Headroom:\n";
    report << "  CPU: " << cpuHeadroom << "%\n";
    report << "  Memory: " << memoryHeadroom << "%\n";
    report << "  Throughput: " << std::setprecision(1) << throughputHeadroom << "%\n\n";
    
    report << "Theoretical Maximum Throughput:\n";
    report << "  CPU Limited: " << std::setprecision(2) << cpuLimitedThroughput << " req/s\n";
    report << "  Memory Limited: " << memoryLimitedThroughput << " req/s\n";
    report << "  Overall Maximum: " << theoreticalMaxThroughput << " req/s\n\n";
    
    report << "Scaling Recommendations:\n";
    if (cpuHeadroom < 20.0) {
        report << "  - CPU utilization is high. Consider horizontal scaling or CPU optimization.\n";
    }
    if (memoryHeadroom < 20.0) {
        report << "  - Memory utilization is high. Consider memory optimization or scaling.\n";
    }
    if (theoreticalMaxThroughput < avgThroughput * 1.5) {
        report << "  - System is approaching throughput limits. Consider scaling soon.\n";
    }
    
    report << "  - Current system can handle approximately " << std::setprecision(0) << (theoreticalMaxThroughput / avgThroughput * 100) << "% more load.\n";
    
    return report.str();
}

std::map<std::string, double> PerformanceAnalyzer::GetBottleneckAnalysis(const std::string& testId) const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_bottleneckAnalysis.find(testId);
    if (it != m_bottleneckAnalysis.end()) {
        return it->second;
    }
    return std::map<std::string, double>();
}

std::map<std::string, double> PerformanceAnalyzer::GetScalabilityMetrics(const std::string& testId) const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_scalabilityMetrics.find(testId);
    if (it != m_scalabilityMetrics.end()) {
        return it->second;
    }
    return std::map<std::string, double>();
}

std::vector<std::string> PerformanceAnalyzer::GetOptimizationSuggestions(const std::string& testId) const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto it = m_optimizationSuggestions.find(testId);
    if (it != m_optimizationSuggestions.end()) {
        return it->second;
    }
    return std::vector<std::string>();
}

// Helper methods
double PerformanceAnalyzer::CalculateTrend(const std::vector<double>& values) {
    if (values.size() < 2) {
        return 0.0;
    }
    
    // Simple linear trend calculation
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    size_t n = values.size();
    
    for (size_t i = 0; i < n; ++i) {
        sumX += i;
        sumY += values[i];
        sumXY += i * values[i];
        sumX2 += i * i;
    }
    
    if (sumX2 * n == sumX * sumX) {
        return 0.0; // Avoid division by zero
    }
    
    return (sumXY * n - sumX * sumY) / (sumX2 * n - sumX * sumX);
}

double PerformanceAnalyzer::CalculateVolatility(const std::vector<double>& values) {
    if (values.size() < 2) {
        return 0.0;
    }
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0.0;
    
    for (double value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    
    variance /= values.size();
    return std::sqrt(variance); // Standard deviation
}

std::string PerformanceAnalyzer::IdentifyComponentBottleneck(const std::vector<PerformanceSnapshot>& snapshots) {
    if (snapshots.empty()) {
        return "UNKNOWN";
    }
    
    double avgCpuUsage = 0.0;
    double avgMemoryUsage = 0.0;
    double avgGpuUsage = 0.0;
    
    for (const auto& snapshot : snapshots) {
        avgCpuUsage += snapshot.cpuUsage;
        avgMemoryUsage += snapshot.memoryUsage;
        avgGpuUsage += snapshot.gpuUsage;
    }
    
    avgCpuUsage /= snapshots.size();
    avgMemoryUsage /= snapshots.size();
    avgGpuUsage /= snapshots.size();
    
    // Identify the component with highest average usage
    if (avgCpuUsage >= avgMemoryUsage && avgCpuUsage >= avgGpuUsage) {
        return "CPU";
    } else if (avgMemoryUsage >= avgGpuUsage) {
        return "MEMORY";
    } else {
        return "GPU";
    }
}

double PerformanceAnalyzer::CalculateResponseTimePercentile(const std::vector<double>& responseTimes, double percentile) {
    if (responseTimes.empty()) {
        return 0.0;
    }
    
    std::vector<double> sortedTimes = responseTimes;
    std::sort(sortedTimes.begin(), sortedTimes.end());
    
    size_t index = static_cast<size_t>(percentile * (sortedTimes.size() - 1));
    return sortedTimes[index];
}

double PerformanceAnalyzer::CalculateThroughputTrend(const std::vector<PerformanceSnapshot>& snapshots) {
    std::vector<double> throughputs;
    
    for (const auto& snapshot : snapshots) {
        auto it = snapshot.metrics.find(PerformanceMetric::THROUGHPUT);
        if (it != snapshot.metrics.end()) {
            throughputs.push_back(it->second);
        }
    }
    
    return CalculateTrend(throughputs);
}

} // namespace Chaos
} // namespace NeonGlyph