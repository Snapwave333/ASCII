#include "ChaosEngineeringReport.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <fstream>

namespace NeonGlyph {
namespace Chaos {

class ChaosEngineeringReport::Impl {
public:
    std::vector<ChaosTestResult> testResults;
    std::vector<CircuitBreakerMetrics> circuitBreakerMetrics;
    std::vector<RedundancyAssessment> redundancyAssessments;
    std::vector<SecurityTestResult> securityTestResults;
    std::vector<PerformanceMetrics> performanceMetrics;
    
    std::string executiveSummary;
    std::string systemOverview;
    std::vector<std::string> recommendations;
    
    std::chrono::system_clock::time_point reportGenerated;
};

ChaosEngineeringReport::ChaosEngineeringReport() : pImpl(std::make_unique<Impl>()) {
    pImpl->reportGenerated = std::chrono::system_clock::now();
}

ChaosEngineeringReport::~ChaosEngineeringReport() = default;

void ChaosEngineeringReport::addTestResult(const ChaosTestResult& result) {
    pImpl->testResults.push_back(result);
}

void ChaosEngineeringReport::addCircuitBreakerMetrics(const CircuitBreakerMetrics& metrics) {
    pImpl->circuitBreakerMetrics.push_back(metrics);
}

void ChaosEngineeringReport::addRedundancyAssessment(const RedundancyAssessment& assessment) {
    pImpl->redundancyAssessments.push_back(assessment);
}

void ChaosEngineeringReport::addSecurityTestResult(const SecurityTestResult& result) {
    pImpl->securityTestResults.push_back(result);
}

void ChaosEngineeringReport::addPerformanceMetrics(const PerformanceMetrics& metrics) {
    pImpl->performanceMetrics.push_back(metrics);
}

void ChaosEngineeringReport::setExecutiveSummary(const std::string& summary) {
    pImpl->executiveSummary = summary;
}

void ChaosEngineeringReport::setSystemOverview(const std::string& overview) {
    pImpl->systemOverview = overview;
}

void ChaosEngineeringReport::setRecommendations(const std::vector<std::string>& recommendations) {
    pImpl->recommendations = recommendations;
}

Json::Value ChaosEngineeringReport::generateJsonReport() const {
    Json::Value report;
    
    // Metadata
    auto time_t_val = std::chrono::system_clock::to_time_t(pImpl->reportGenerated);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
    report["metadata"]["generated_at"] = ss.str();
    report["metadata"]["system"] = "NeonGlyph ASCII Visual Synthesis Engine";
    report["metadata"]["version"] = "1.0.0";
    
    // Executive Summary
    report["executive_summary"] = pImpl->executiveSummary;
    
    // System Overview
    report["system_overview"] = pImpl->systemOverview;
    
    // Overall Assessment
    report["overall_assessment"]["resilience_score"] = getSystemResilienceScore();
    report["overall_assessment"]["assessment"] = getOverallAssessment();
    
    // Component Scores
    auto componentScores = getComponentScores();
    Json::Value components(Json::arrayValue);
    for (const auto& [component, score] : componentScores) {
        Json::Value comp;
        comp["name"] = component;
        comp["score"] = score;
        comp["status"] = score >= 80.0 ? "HEALTHY" : score >= 60.0 ? "DEGRADED" : "CRITICAL";
        components.append(comp);
    }
    report["component_scores"] = components;
    
    // Test Results
    Json::Value testResults(Json::arrayValue);
    for (const auto& result : pImpl->testResults) {
        Json::Value test;
        test["test_id"] = result.testId;
        test["component"] = result.component;
        test["failure_type"] = result.failureType;
        test["success"] = result.success;
        test["error_message"] = result.errorMessage;
        test["metrics"] = result.metrics;
        test["impact_assessment"] = result.impactAssessment;
        testResults.append(test);
    }
    report["chaos_experiment_results"] = testResults;
    
    // Circuit Breaker Metrics
    Json::Value breakerMetrics(Json::arrayValue);
    for (const auto& metrics : pImpl->circuitBreakerMetrics) {
        Json::Value breaker;
        breaker["name"] = metrics.breakerName;
        breaker["total_requests"] = metrics.totalRequests;
        breaker["successful_requests"] = metrics.successfulRequests;
        breaker["failed_requests"] = metrics.failedRequests;
        breaker["failure_rate"] = metrics.failureRate;
        breaker["response_time_avg"] = metrics.responseTimeAvg;
        breaker["response_time_p95"] = metrics.responseTimeP95;
        breaker["state_transitions"] = metrics.stateTransitions;
        breakerMetrics.append(breaker);
    }
    report["circuit_breaker_metrics"] = breakerMetrics;
    
    // Redundancy Assessments
    Json::Value redundancyAssessments(Json::arrayValue);
    for (const auto& assessment : pImpl->redundancyAssessments) {
        Json::Value redundancy;
        redundancy["component"] = assessment.component;
        redundancy["redundancy_type"] = assessment.redundancyType;
        redundancy["primary_healthy"] = assessment.primaryHealthy;
        redundancy["backup_healthy"] = assessment.backupHealthy;
        redundancy["failover_time"] = assessment.failoverTime;
        redundancy["failover_count"] = assessment.failoverCount;
        redundancy["availability_percentage"] = assessment.availabilityPercentage;
        redundancyAssessments.append(redundancy);
    }
    report["redundancy_assessments"] = redundancyAssessments;
    
    // Security Test Results
    Json::Value securityResults(Json::arrayValue);
    for (const auto& result : pImpl->securityTestResults) {
        Json::Value security;
        security["vulnerability_type"] = result.vulnerabilityType;
        security["component"] = result.component;
        security["vulnerability_found"] = result.vulnerabilityFound;
        security["severity"] = result.severity;
        security["description"] = result.description;
        security["remediation"] = result.remediation;
        security["test_details"] = result.testDetails;
        securityResults.append(security);
    }
    report["security_test_results"] = securityResults;
    
    // Performance Metrics
    Json::Value performanceMetrics(Json::arrayValue);
    for (const auto& metrics : pImpl->performanceMetrics) {
        Json::Value perf;
        perf["test_type"] = metrics.testType;
        perf["baseline_performance"] = metrics.baselinePerformance;
        perf["degraded_performance"] = metrics.degradedPerformance;
        perf["recovery_time"] = metrics.recoveryTime;
        perf["max_load_sustained"] = metrics.maxLoadSustained;
        performanceMetrics.append(perf);
    }
    report["performance_metrics"] = performanceMetrics;
    
    // Recommendations
    Json::Value recommendations(Json::arrayValue);
    for (const auto& rec : pImpl->recommendations) {
        recommendations.append(rec);
    }
    report["recommendations"] = recommendations;
    
    return report;
}

std::string ChaosEngineeringReport::generateMarkdownReport() const {
    std::stringstream md;
    
    md << "# NeonGlyph Chaos Engineering Report\n\n";
    
    // Executive Summary
    md << "## Executive Summary\n\n";
    md << pImpl->executiveSummary << "\n\n";
    
    // Overall Assessment
    md << "## Overall System Assessment\n\n";
    md << "**Resilience Score:** " << std::fixed << std::setprecision(1) << getSystemResilienceScore() << "/100\n\n";
    md << getOverallAssessment() << "\n\n";
    
    // Component Scores
    md << "## Component Health Assessment\n\n";
    auto componentScores = getComponentScores();
    for (const auto& [component, score] : componentScores) {
        std::string status = score >= 80.0 ? "🟢 HEALTHY" : score >= 60.0 ? "🟡 DEGRADED" : "🔴 CRITICAL";
        md << "- **" << component << "**: " << std::fixed << std::setprecision(1) << score << "/100 " << status << "\n";
    }
    md << "\n";
    
    // Test Results Summary
    md << "## Chaos Experiment Results\n\n";
    int totalTests = pImpl->testResults.size();
    int successfulTests = std::count_if(pImpl->testResults.begin(), pImpl->testResults.end(),
        [](const ChaosTestResult& r) { return r.success; });
    md << "**Total Tests:** " << totalTests << "\n";
    md << "**Successful Tests:** " << successfulTests << "\n";
    md << "**Success Rate:** " << std::fixed << std::setprecision(1) 
       << (totalTests > 0 ? (successfulTests * 100.0 / totalTests) : 0.0) << "%\n\n";
    
    // Circuit Breaker Performance
    md << "## Circuit Breaker Performance\n\n";
    for (const auto& metrics : pImpl->circuitBreakerMetrics) {
        md << "### " << metrics.breakerName << "\n";
        md << "- **Total Requests:** " << metrics.totalRequests << "\n";
        md << "- **Success Rate:** " << std::fixed << std::setprecision(1) 
           << (metrics.totalRequests > 0 ? (metrics.successfulRequests * 100.0 / metrics.totalRequests) : 0.0) << "%\n";
        md << "- **Average Response Time:** " << std::fixed << std::setprecision(2) << metrics.responseTimeAvg << "ms\n";
        md << "- **95th Percentile:** " << std::fixed << std::setprecision(2) << metrics.responseTimeP95 << "ms\n\n";
    }
    
    // Redundancy Assessment
    md << "## Redundancy and Failover Assessment\n\n";
    for (const auto& assessment : pImpl->redundancyAssessments) {
        md << "### " << assessment.component << "\n";
        md << "- **Redundancy Type:** " << assessment.redundancyType << "\n";
        md << "- **Primary Health:** " << (assessment.primaryHealthy ? "🟢 Healthy" : "🔴 Unhealthy") << "\n";
        md << "- **Backup Health:** " << (assessment.backupHealthy ? "🟢 Healthy" : "🔴 Unhealthy") << "\n";
        md << "- **Availability:** " << std::fixed << std::setprecision(1) << assessment.availabilityPercentage << "%\n";
        md << "- **Failover Time:** " << std::fixed << std::setprecision(2) << assessment.failoverTime << "ms\n\n";
    }
    
    // Security Test Results
    md << "## Security Testing Results\n\n";
    int securityIssues = std::count_if(pImpl->securityTestResults.begin(), pImpl->securityTestResults.end(),
        [](const SecurityTestResult& r) { return r.vulnerabilityFound; });
    md << "**Security Issues Found:** " << securityIssues << "\n\n";
    
    for (const auto& result : pImpl->securityTestResults) {
        if (result.vulnerabilityFound) {
            md << "### " << result.vulnerabilityType << " (" << result.severity << ")\n";
            md << "**Component:** " << result.component << "\n";
            md << "**Description:** " << result.description << "\n";
            md << "**Remediation:** " << result.remediation << "\n\n";
        }
    }
    
    // Performance Metrics
    md << "## Performance Testing Results\n\n";
    for (const auto& metrics : pImpl->performanceMetrics) {
        md << "### " << metrics.testType << " Test\n";
        md << "- **Baseline Performance:** " << std::fixed << std::setprecision(1) << metrics.baselinePerformance << "\n";
        md << "- **Degraded Performance:** " << std::fixed << std::setprecision(1) << metrics.degradedPerformance << "\n";
        md << "- **Recovery Time:** " << std::fixed << std::setprecision(2) << metrics.recoveryTime << "s\n";
        md << "- **Max Load Sustained:** " << std::fixed << std::setprecision(1) << metrics.maxLoadSustained << "\n\n";
    }
    
    // Recommendations
    md << "## Recommendations\n\n";
    for (const auto& rec : pImpl->recommendations) {
        md << "- " << rec << "\n";
    }
    md << "\n";
    
    return md.str();
}

std::string ChaosEngineeringReport::generateHtmlReport() const {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>NeonGlyph Chaos Engineering Report</title>\n";
    html << "    <style>\n";
    html << "        body { font-family: Arial, sans-serif; margin: 40px; background-color: #f5f5f5; }\n";
    html << "        .container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << "        h1 { color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }\n";
    html << "        h2 { color: #34495e; margin-top: 30px; }\n";
    html << "        h3 { color: #7f8c8d; }\n";
    html << "        .score { font-size: 24px; font-weight: bold; color: #27ae60; }\n";
    html << "        .healthy { color: #27ae60; }\n";
    html << "        .degraded { color: #f39c12; }\n";
    html << "        .critical { color: #e74c3c; }\n";
    html << "        .metric { background-color: #ecf0f1; padding: 15px; margin: 10px 0; border-radius: 5px; }\n";
    html << "        table { width: 100%; border-collapse: collapse; margin: 20px 0; }\n";
    html << "        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    html << "        th { background-color: #3498db; color: white; }\n";
    html << "        .recommendation { background-color: #e8f6f3; padding: 10px; margin: 5px 0; border-left: 4px solid #3498db; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <div class=\"container\">\n";
    html << "        <h1>NeonGlyph Chaos Engineering Report</h1>\n";
    
    // Executive Summary
    html << "        <h2>Executive Summary</h2>\n";
    html << "        <p>" << pImpl->executiveSummary << "</p>\n";
    
    // Overall Assessment
    html << "        <h2>Overall System Assessment</h2>\n";
    html << "        <div class=\"metric\">\n";
    html << "            <strong>Resilience Score:</strong> <span class=\"score\">" 
         << std::fixed << std::setprecision(1) << getSystemResilienceScore() << "/100</span>\n";
    html << "        </div>\n";
    html << "        <p>" << getOverallAssessment() << "</p>\n";
    
    // Component Scores
    html << "        <h2>Component Health Assessment</h2>\n";
    html << "        <table>\n";
    html << "            <tr><th>Component</th><th>Score</th><th>Status</th></tr>\n";
    auto componentScores = getComponentScores();
    for (const auto& [component, score] : componentScores) {
        std::string statusClass = score >= 80.0 ? "healthy" : score >= 60.0 ? "degraded" : "critical";
        std::string statusText = score >= 80.0 ? "HEALTHY" : score >= 60.0 ? "DEGRADED" : "CRITICAL";
        html << "            <tr>\n";
        html << "                <td>" << component << "</td>\n";
        html << "                <td>" << std::fixed << std::setprecision(1) << score << "/100</td>\n";
        html << "                <td class=\"" << statusClass << "\">" << statusText << "</td>\n";
        html << "            </tr>\n";
    }
    html << "        </table>\n";
    
    // Recommendations
    html << "        <h2>Recommendations</h2>\n";
    for (const auto& rec : pImpl->recommendations) {
        html << "        <div class=\"recommendation\">" << rec << "</div>\n";
    }
    
    html << "    </div>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

void ChaosEngineeringReport::saveReport(const std::string& filename, const std::string& format) const {
    std::string content;
    
    if (format == "json") {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        content = Json::writeString(builder, generateJsonReport());
    } else if (format == "markdown") {
        content = generateMarkdownReport();
    } else if (format == "html") {
        content = generateHtmlReport();
    } else {
        throw std::invalid_argument("Unsupported format: " + format);
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    file << content;
    file.close();
}

std::string ChaosEngineeringReport::getOverallAssessment() const {
    double score = getSystemResilienceScore();
    
    if (score >= 90.0) {
        return "The NeonGlyph system demonstrates exceptional resilience with robust fault tolerance, comprehensive redundancy, and effective recovery mechanisms. All critical components are performing optimally with minimal risk of failure.";
    } else if (score >= 80.0) {
        return "The system shows strong resilience with good fault tolerance and redundancy. Minor improvements in specific areas could enhance overall system reliability.";
    } else if (score >= 70.0) {
        return "The system has moderate resilience with some areas requiring attention. Key components may benefit from enhanced redundancy or improved recovery mechanisms.";
    } else if (score >= 60.0) {
        return "The system shows signs of degradation in resilience. Immediate attention is recommended to address critical vulnerabilities and improve fault tolerance.";
    } else {
        return "The system resilience is critically low. Urgent intervention is required to prevent potential system failures and implement comprehensive hardening measures.";
    }
}

double ChaosEngineeringReport::getSystemResilienceScore() const {
    std::map<std::string, double> componentScores = getComponentScores();
    
    if (componentScores.empty()) {
        return 0.0;
    }
    
    double totalScore = 0.0;
    for (const auto& [component, score] : componentScores) {
        totalScore += score;
    }
    
    return totalScore / componentScores.size();
}

std::map<std::string, double> ChaosEngineeringReport::getComponentScores() const {
    std::map<std::string, double> scores;
    
    // Calculate scores for each component based on test results and metrics
    std::map<std::string, std::vector<double>> componentMetrics;
    
    // Process test results
    for (const auto& result : pImpl->testResults) {
        double testScore = result.success ? 100.0 : 0.0;
        componentMetrics[result.component].push_back(testScore);
    }
    
    // Process circuit breaker metrics
    for (const auto& metrics : pImpl->circuitBreakerMetrics) {
        double breakerScore = 100.0;
        if (metrics.failureRate > 0.1) breakerScore -= 30.0;
        if (metrics.failureRate > 0.2) breakerScore -= 30.0;
        if (metrics.responseTimeAvg > 1000) breakerScore -= 20.0;
        if (metrics.responseTimeP95 > 2000) breakerScore -= 20.0;
        
        componentMetrics[metrics.breakerName].push_back(std::max(0.0, breakerScore));
    }
    
    // Process redundancy assessments
    for (const auto& assessment : pImpl->redundancyAssessments) {
        double redundancyScore = assessment.availabilityPercentage;
        if (!assessment.primaryHealthy) redundancyScore -= 30.0;
        if (!assessment.backupHealthy) redundancyScore -= 30.0;
        if (assessment.failoverTime > 5000) redundancyScore -= 20.0;
        
        componentMetrics[assessment.component].push_back(std::max(0.0, redundancyScore));
    }
    
    // Process security test results
    for (const auto& result : pImpl->securityTestResults) {
        if (result.vulnerabilityFound) {
            double securityScore = 100.0;
            if (result.severity == "HIGH") securityScore -= 40.0;
            else if (result.severity == "MEDIUM") securityScore -= 25.0;
            else if (result.severity == "LOW") securityScore -= 10.0;
            
            componentMetrics[result.component].push_back(std::max(0.0, securityScore));
        } else {
            componentMetrics[result.component].push_back(100.0);
        }
    }
    
    // Process performance metrics
    for (const auto& metrics : pImpl->performanceMetrics) {
        double perfScore = 100.0;
        double degradation = (metrics.baselinePerformance - metrics.degradedPerformance) / metrics.baselinePerformance;
        perfScore -= (degradation * 50.0);
        if (metrics.recoveryTime > 30.0) perfScore -= 20.0;
        
        componentMetrics[metrics.testType].push_back(std::max(0.0, perfScore));
    }
    
    // Calculate average scores for each component
    for (const auto& [component, metrics] : componentMetrics) {
        if (!metrics.empty()) {
            double avgScore = std::accumulate(metrics.begin(), metrics.end(), 0.0) / metrics.size();
            scores[component] = std::min(100.0, std::max(0.0, avgScore));
        }
    }
    
    return scores;
}

// ReportGenerator Implementation
std::string ReportGenerator::generateExecutiveSummary(const std::vector<ChaosTestResult>& results) {
    std::stringstream summary;
    
    int totalTests = results.size();
    int successfulTests = std::count_if(results.begin(), results.end(),
        [](const ChaosTestResult& r) { return r.success; });
    
    summary << "The NeonGlyph ASCII Visual Synthesis Engine has undergone comprehensive chaos engineering testing. ";
    summary << "Out of " << totalTests << " chaos experiments conducted, " << successfulTests 
            << " tests demonstrated successful system resilience (" << std::fixed << std::setprecision(1)
            << (totalTests > 0 ? (successfulTests * 100.0 / totalTests) : 0.0) << "% success rate). ";
    
    if (successfulTests == totalTests) {
        summary << "All tests passed, indicating robust system resilience and effective fault tolerance mechanisms.";
    } else if (successfulTests > totalTests * 0.8) {
        summary << "The system shows strong resilience with most tests passing, though some areas may benefit from improvement.";
    } else {
        summary << "The system shows signs of resilience gaps that require attention to improve overall reliability.";
    }
    
    return summary.str();
}

std::string ReportGenerator::generateFailureAnalysis(const std::vector<ChaosTestResult>& results) {
    std::stringstream analysis;
    
    analysis << "## Failure Mode Analysis\n\n";
    
    std::map<std::string, int> failureTypes;
    std::map<std::string, int> componentFailures;
    
    for (const auto& result : results) {
        if (!result.success) {
            failureTypes[result.failureType]++;
            componentFailures[result.component]++;
        }
    }
    
    if (!failureTypes.empty()) {
        analysis << "### Most Common Failure Types\n";
        for (const auto& [failureType, count] : failureTypes) {
            analysis << "- " << failureType << ": " << count << " occurrences\n";
        }
        analysis << "\n";
        
        analysis << "### Components with Most Failures\n";
        for (const auto& [component, count] : componentFailures) {
            analysis << "- " << component << ": " << count << " failures\n";
        }
        analysis << "\n";
    } else {
        analysis << "No failures were observed during testing, indicating strong system resilience.\n\n";
    }
    
    return analysis.str();
}

std::string ReportGenerator::generateCircuitBreakerAnalysis(const std::vector<CircuitBreakerMetrics>& metrics) {
    std::stringstream analysis;
    
    analysis << "## Circuit Breaker Performance Analysis\n\n";
    
    for (const auto& metric : metrics) {
        analysis << "### " << metric.breakerName << "\n";
        analysis << "- **Total Requests:** " << metric.totalRequests << "\n";
        analysis << "- **Success Rate:** " << std::fixed << std::setprecision(1) 
                << (metric.totalRequests > 0 ? (metric.successfulRequests * 100.0 / metric.totalRequests) : 0.0) << "%\n";
        analysis << "- **Average Response Time:** " << std::fixed << std::setprecision(2) << metric.responseTimeAvg << "ms\n";
        analysis << "- **95th Percentile Response Time:** " << std::fixed << std::setprecision(2) << metric.responseTimeP95 << "ms\n";
        analysis << "- **State Transitions:** " << metric.stateTransitions << "\n\n";
    }
    
    return analysis.str();
}

std::string ReportGenerator::generateRedundancyReport(const std::vector<RedundancyAssessment>& assessments) {
    std::stringstream report;
    
    report << "## Redundancy and Failover Assessment\n\n";
    
    for (const auto& assessment : assessments) {
        report << "### " << assessment.component << "\n";
        report << "- **Redundancy Type:** " << assessment.redundancyType << "\n";
        report << "- **Primary System:** " << (assessment.primaryHealthy ? "✅ Healthy" : "❌ Unhealthy") << "\n";
        report << "- **Backup System:** " << (assessment.backupHealthy ? "✅ Healthy" : "❌ Unhealthy") << "\n";
        report << "- **Availability:** " << std::fixed << std::setprecision(1) << assessment.availabilityPercentage << "%\n";
        report << "- **Average Failover Time:** " << std::fixed << std::setprecision(2) << assessment.failoverTime << "ms\n";
        report << "- **Total Failovers:** " << assessment.failoverCount << "\n\n";
    }
    
    return report.str();
}

std::string ReportGenerator::generateSecurityReport(const std::vector<SecurityTestResult>& results) {
    std::stringstream report;
    
    report << "## Security Testing Results\n\n";
    
    int vulnerabilitiesFound = std::count_if(results.begin(), results.end(),
        [](const SecurityTestResult& r) { return r.vulnerabilityFound; });
    
    report << "**Total Vulnerabilities Found:** " << vulnerabilitiesFound << "\n\n";
    
    if (vulnerabilitiesFound > 0) {
        report << "### Vulnerability Details\n";
        for (const auto& result : results) {
            if (result.vulnerabilityFound) {
                report << "#### " << result.vulnerabilityType << " (" << result.severity << ")\n";
                report << "**Component:** " << result.component << "\n";
                report << "**Description:** " << result.description << "\n";
                report << "**Remediation:** " << result.remediation << "\n\n";
            }
        }
    } else {
        report << "No security vulnerabilities were identified during testing.\n\n";
    }
    
    return report.str();
}

std::string ReportGenerator::generatePerformanceReport(const std::vector<PerformanceMetrics>& metrics) {
    std::stringstream report;
    
    report << "## Performance Testing Results\n\n";
    
    for (const auto& metric : metrics) {
        report << "### " << metric.testType << " Performance Test\n";
        report << "- **Baseline Performance:** " << std::fixed << std::setprecision(1) << metric.baselinePerformance << "\n";
        report << "- **Performance Under Load:** " << std::fixed << std::setprecision(1) << metric.degradedPerformance << "\n";
        report << "- **Performance Degradation:** " << std::fixed << std::setprecision(1) 
                << ((metric.baselinePerformance - metric.degradedPerformance) / metric.baselinePerformance * 100.0) << "%\n";
        report << "- **Recovery Time:** " << std::fixed << std::setprecision(2) << metric.recoveryTime << " seconds\n";
        report << "- **Maximum Load Sustained:** " << std::fixed << std::setprecision(1) << metric.maxLoadSustained << "\n\n";
    }
    
    return report.str();
}

std::string ReportGenerator::generateRecommendations(const std::map<std::string, double>& componentScores) {
    std::stringstream recommendations;
    
    recommendations << "## Recommendations\n\n";
    
    for (const auto& [component, score] : componentScores) {
        if (score < 70.0) {
            recommendations << "### " << component << " (Score: " << std::fixed << std::setprecision(1) << score << "/100)\n";
            if (score < 60.0) {
                recommendations << "- **CRITICAL:** Immediate attention required. Consider implementing emergency hardening measures.\n";
            } else {
                recommendations << "- **PRIORITY:** Focus on improving fault tolerance and recovery mechanisms.\n";
            }
            recommendations << "- Review component architecture for single points of failure\n";
            recommendations << "- Implement additional monitoring and alerting\n";
            recommendations << "- Consider adding redundancy or backup systems\n\n";
        }
    }
    
    recommendations << "### General Recommendations\n";
    recommendations << "- Continue regular chaos engineering testing\n";
    recommendations << "- Implement automated recovery mechanisms where possible\n";
    recommendations << "- Monitor system performance and adjust thresholds as needed\n";
    recommendations << "- Conduct regular security assessments\n";
    recommendations << "- Maintain up-to-date documentation of system architecture\n";
    
    return recommendations.str();
}

Json::Value ReportGenerator::createDashboardData(const ChaosEngineeringReport& report) {
    Json::Value dashboard;
    
    dashboard["resilience_score"] = report.getSystemResilienceScore();
    dashboard["overall_assessment"] = report.getOverallAssessment();
    dashboard["component_scores"] = report.generateJsonReport()["component_scores"];
    
    return dashboard;
}

std::string ReportGenerator::createTrendAnalysis(const std::vector<ChaosTestResult>& results) {
    std::stringstream analysis;
    
    analysis << "## Trend Analysis\n\n";
    
    if (results.size() < 2) {
        analysis << "Insufficient data for trend analysis.\n";
        return analysis.str();
    }
    
    // Simple trend calculation based on success rate over time
    std::vector<double> successRates;
    for (size_t i = 0; i < results.size(); i += std::max(1UL, results.size() / 10)) {
        int successes = 0;
        int total = 0;
        for (size_t j = i; j < std::min(i + results.size() / 10, results.size()); j++) {
            if (results[j].success) successes++;
            total++;
        }
        if (total > 0) {
            successRates.push_back(successes * 100.0 / total);
        }
    }
    
    if (successRates.size() >= 2) {
        double firstRate = successRates.front();
        double lastRate = successRates.back();
        double trend = lastRate - firstRate;
        
        analysis << "**Resilience Trend:** ";
        if (trend > 5.0) {
            analysis << "📈 Improving (+" << std::fixed << std::setprecision(1) << trend << "% improvement)\n";
        } else if (trend < -5.0) {
            analysis << "📉 Declining (" << std::fixed << std::setprecision(1) << trend << "% decline)\n";
        } else {
            analysis << "➡️ Stable (±" << std::fixed << std::setprecision(1) << std::abs(trend) << "% change)\n";
        }
    }
    
    return analysis.str();
}

std::string ReportGenerator::createRiskMatrix(const std::vector<SecurityTestResult>& results) {
    std::stringstream matrix;
    
    matrix << "## Security Risk Matrix\n\n";
    
    std::map<std::string, std::map<std::string, int>> riskCounts;
    std::vector<std::string> severities = {"LOW", "MEDIUM", "HIGH", "CRITICAL"};
    std::vector<std::string> components = {"Vulkan Renderer", "Audio Engine", "ASCII Processor", "AI Component", "Headless Mode"};
    
    // Initialize matrix
    for (const auto& severity : severities) {
        for (const auto& component : components) {
            riskCounts[severity][component] = 0;
        }
    }
    
    // Count vulnerabilities
    for (const auto& result : results) {
        if (result.vulnerabilityFound) {
            riskCounts[result.severity][result.component]++;
        }
    }
    
    // Create matrix table
    matrix << "| Component | LOW | MEDIUM | HIGH | CRITICAL | Total |\n";
    matrix << "|-----------|-----|--------|------|----------|-------|\n";
    
    for (const auto& component : components) {
        int total = 0;
        matrix << "| " << component << " | ";
        for (const auto& severity : severities) {
            int count = riskCounts[severity][component];
            matrix << count << " | ";
            total += count;
        }
        matrix << total << " |\n";
    }
    
    return matrix.str();
}

// ReportValidator Implementation
bool ReportValidator::validateReportCompleteness(const ChaosEngineeringReport& report) {
    // Basic validation - check if report has minimum required data
    auto jsonReport = report.generateJsonReport();
    
    return !jsonReport["executive_summary"].asString().empty() &&
           !jsonReport["component_scores"].empty() &&
           !jsonReport["recommendations"].empty();
}

bool ReportValidator::validateDataConsistency(const Json::Value& reportData) {
    // Check for basic data consistency
    if (!reportData.isMember("component_scores") || !reportData["component_scores"].isArray()) {
        return false;
    }
    
    for (const auto& component : reportData["component_scores"]) {
        if (!component.isMember("name") || !component.isMember("score")) {
            return false;
        }
        
        double score = component["score"].asDouble();
        if (score < 0.0 || score > 100.0) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> ReportValidator::getMissingSections(const ChaosEngineeringReport& report) {
    std::vector<std::string> missing;
    
    auto jsonReport = report.generateJsonReport();
    
    if (jsonReport["executive_summary"].asString().empty()) {
        missing.push_back("Executive Summary");
    }
    
    if (jsonReport["system_overview"].asString().empty()) {
        missing.push_back("System Overview");
    }
    
    if (jsonReport["component_scores"].empty()) {
        missing.push_back("Component Scores");
    }
    
    if (jsonReport["recommendations"].empty()) {
        missing.push_back("Recommendations");
    }
    
    return missing;
}

std::vector<std::string> ReportValidator::getDataQualityIssues(const Json::Value& reportData) {
    std::vector<std::string> issues;
    
    if (reportData["resilience_score"].asDouble() < 0.0 || reportData["resilience_score"].asDouble() > 100.0) {
        issues.push_back("Invalid resilience score (must be between 0-100)");
    }
    
    for (const auto& component : reportData["component_scores"]) {
        double score = component["score"].asDouble();
        if (score < 0.0 || score > 100.0) {
            issues.push_back("Invalid component score for " + component["name"].asString());
        }
    }
    
    return issues;
}

} // namespace Chaos
} // namespace NeonGlyph