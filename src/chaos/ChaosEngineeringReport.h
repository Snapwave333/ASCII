#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <json/json.h>

namespace NeonGlyph {
namespace Chaos {

enum class ReportSection {
    EXECUTIVE_SUMMARY,
    SYSTEM_OVERVIEW,
    FAILURE_MODE_ANALYSIS,
    CHAOS_EXPERIMENT_RESULTS,
    CIRCUIT_BREAKER_PERFORMANCE,
    REDUNDANCY_ASSESSMENT,
    MONITORING_INSIGHTS,
    RECOVERY_EFFECTIVENESS,
    SECURITY_TESTING_RESULTS,
    PERFORMANCE_TESTING,
    RECOMMENDATIONS,
    CONCLUSION
};

struct ChaosTestResult {
    std::string testId;
    std::string component;
    std::string failureType;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    bool success;
    std::string errorMessage;
    Json::Value metrics;
    Json::Value impactAssessment;
};

struct CircuitBreakerMetrics {
    std::string breakerName;
    int totalRequests;
    int successfulRequests;
    int failedRequests;
    int timeouts;
    double failureRate;
    double responseTimeAvg;
    double responseTimeP95;
    int stateTransitions;
    std::chrono::system_clock::time_point lastStateChange;
};

struct RedundancyAssessment {
    std::string component;
    std::string redundancyType;
    bool primaryHealthy;
    bool backupHealthy;
    double failoverTime;
    int failoverCount;
    std::string lastFailoverReason;
    double availabilityPercentage;
};

struct SecurityTestResult {
    std::string vulnerabilityType;
    std::string component;
    bool vulnerabilityFound;
    std::string severity;
    std::string description;
    std::string remediation;
    Json::Value testDetails;
};

struct PerformanceMetrics {
    std::string testType;
    double baselinePerformance;
    double degradedPerformance;
    double recoveryTime;
    double maxLoadSustained;
    std::vector<double> responseTimePercentiles;
    std::vector<double> throughputOverTime;
    std::vector<double> errorRateOverTime;
};

class ChaosEngineeringReport {
public:
    ChaosEngineeringReport();
    ~ChaosEngineeringReport();

    void addTestResult(const ChaosTestResult& result);
    void addCircuitBreakerMetrics(const CircuitBreakerMetrics& metrics);
    void addRedundancyAssessment(const RedundancyAssessment& assessment);
    void addSecurityTestResult(const SecurityTestResult& result);
    void addPerformanceMetrics(const PerformanceMetrics& metrics);
    
    void setExecutiveSummary(const std::string& summary);
    void setSystemOverview(const std::string& overview);
    void setRecommendations(const std::vector<std::string>& recommendations);
    
    Json::Value generateJsonReport() const;
    std::string generateMarkdownReport() const;
    std::string generateHtmlReport() const;
    
    void saveReport(const std::string& filename, const std::string& format = "json") const;
    
    std::string getOverallAssessment() const;
    double getSystemResilienceScore() const;
    std::map<std::string, double> getComponentScores() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class ReportGenerator {
public:
    static std::string generateExecutiveSummary(const std::vector<ChaosTestResult>& results);
    static std::string generateFailureAnalysis(const std::vector<ChaosTestResult>& results);
    static std::string generateCircuitBreakerAnalysis(const std::vector<CircuitBreakerMetrics>& metrics);
    static std::string generateRedundancyReport(const std::vector<RedundancyAssessment>& assessments);
    static std::string generateSecurityReport(const std::vector<SecurityTestResult>& results);
    static std::string generatePerformanceReport(const std::vector<PerformanceMetrics>& metrics);
    static std::string generateRecommendations(const std::map<std::string, double>& componentScores);
    
    static Json::Value createDashboardData(const ChaosEngineeringReport& report);
    static std::string createTrendAnalysis(const std::vector<ChaosTestResult>& results);
    static std::string createRiskMatrix(const std::vector<SecurityTestResult>& results);
};

class ReportValidator {
public:
    static bool validateReportCompleteness(const ChaosEngineeringReport& report);
    static bool validateDataConsistency(const Json::Value& reportData);
    static std::vector<std::string> getMissingSections(const ChaosEngineeringReport& report);
    static std::vector<std::string> getDataQualityIssues(const Json::Value& reportData);
};

} // namespace Chaos
} // namespace NeonGlyph