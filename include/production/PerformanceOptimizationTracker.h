#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include "production/NamingConventions.h"

namespace NeonGlyph {
namespace Production {

enum class OptimizationType {
    Memory,
    CPU,
    GPU,
    I_O,
    Network,
    Algorithmic,
    Rendering,
    Audio,
    Storage,
    Cache
};

enum class OptimizationPriority {
    Critical,
    High,
    Medium,
    Low,
    Informational
};

enum class OptimizationStatus {
    Identified,
    Analyzed,
    Implemented,
    Tested,
    Deployed,
    Reverted,
    Rejected
};

struct PerformanceBaseline {
    std::string metric_name;
    double baseline_value;
    double target_value;
    double current_value;
    std::string unit;
    std::chrono::system_clock::time_point established_at;
    std::chrono::system_clock::time_point last_updated;
    std::map<std::string, std::string> metadata;
};

struct OptimizationOpportunity {
    std::string id;
    std::string title;
    std::string description;
    OptimizationType type;
    OptimizationPriority priority;
    OptimizationStatus status;
    double estimated_impact;
    double implementation_cost;
    std::string affected_component;
    std::vector<std::string> affected_metrics;
    std::chrono::system_clock::time_point identified_at;
    std::chrono::system_clock::time_point implemented_at;
    std::string assigned_to;
    std::map<std::string, std::string> metadata;
};

struct PerformanceImprovement {
    std::string optimization_id;
    std::string metric_name;
    double before_value;
    double after_value;
    double improvement_percentage;
    std::string unit;
    std::chrono::system_clock::time_point measured_at;
    std::map<std::string, std::string> context;
};

struct PerformanceMeasurement {
    std::string metric_name;
    double value;
    std::string unit;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> context;
};

struct RegressionAlert {
    std::string id;
    std::string metric_name;
    double baseline_value;
    double current_value;
    double regression_percentage;
    std::string severity;
    std::chrono::system_clock::time_point detected_at;
    std::string likely_cause;
    std::vector<std::string> suggested_actions;
};

class PerformanceOptimizationTracker {
public:
    PerformanceOptimizationTracker();
    ~PerformanceOptimizationTracker();

    // Baseline management
    void EstablishBaseline(const std::string& metric_name, double baseline_value, 
                          const std::string& unit = "", double target_value = 0.0);
    void UpdateBaseline(const std::string& metric_name, double new_baseline_value);
    PerformanceBaseline GetBaseline(const std::string& metric_name) const;
    std::vector<PerformanceBaseline> GetAllBaselines() const;
    bool HasBaseline(const std::string& metric_name) const;
    
    // Optimization opportunity management
    std::string IdentifyOptimization(const std::string& title, const std::string& description,
                                   OptimizationType type, OptimizationPriority priority,
                                   double estimated_impact, const std::string& affected_component);
    void UpdateOptimizationStatus(const std::string& optimization_id, OptimizationStatus status);
    void AssignOptimization(const std::string& optimization_id, const std::string& assignee);
    void UpdateOptimizationPriority(const std::string& optimization_id, OptimizationPriority priority);
    
    std::vector<OptimizationOpportunity> GetOptimizations(OptimizationStatus status = OptimizationStatus::Identified) const;
    std::vector<OptimizationOpportunity> GetOptimizationsByType(OptimizationType type) const;
    std::vector<OptimizationOpportunity> GetOptimizationsByPriority(OptimizationPriority priority) const;
    std::vector<OptimizationOpportunity> GetOptimizationsByComponent(const std::string& component) const;
    
    OptimizationOpportunity GetOptimization(const std::string& optimization_id) const;
    bool OptimizationExists(const std::string& optimization_id) const;
    
    // Performance measurement and tracking
    void RecordPerformanceMeasurement(const std::string& metric_name, double value, 
                                    const std::string& unit = "", 
                                    const std::map<std::string, std::string>& context = {});
    
    void RecordPerformanceImprovement(const std::string& optimization_id, 
                                    const std::string& metric_name,
                                    double before_value, double after_value,
                                    const std::string& unit = "");
    
    std::vector<PerformanceImprovement> GetPerformanceImprovements(
        const std::string& optimization_id = "", 
        const std::string& metric_name = "") const;
    
    // Regression detection and alerts
    void CheckForRegressions();
    std::vector<RegressionAlert> GetRegressionAlerts(
        const std::string& severity = "") const;
    void AcknowledgeRegression(const std::string& regression_id);
    void DismissRegression(const std::string& regression_id);
    
    // Optimization recommendations
    std::vector<OptimizationOpportunity> GetRecommendations(
        OptimizationType type = OptimizationType::Memory,
        size_t max_recommendations = 10) const;
    
    std::vector<OptimizationOpportunity> GetQuickWins(size_t max_quick_wins = 5) const;
    std::vector<OptimizationOpportunity> GetHighImpactOptimizations(size_t max_optimizations = 5) const;
    
    // Automated optimization analysis
    void AnalyzePerformanceData();
    void GenerateOptimizationSuggestions();
    void PrioritizeOptimizations();
    
    // Reporting and analytics
    struct OptimizationReport {
        size_t total_optimizations;
        size_t implemented_optimizations;
        size_t pending_optimizations;
        double total_improvement_percentage;
        std::map<OptimizationType, size_t> optimizations_by_type;
        std::map<OptimizationPriority, size_t> optimizations_by_priority;
        std::map<std::string, double> improvements_by_component;
        std::vector<std::string> top_performing_optimizations;
        std::vector<std::string> regression_alerts;
    };
    
    OptimizationReport GenerateReport() const;
    std::string GenerateDetailedReport() const;
    std::string GenerateExecutiveSummary() const;
    
    // Trend analysis
    struct PerformanceTrend {
        std::string metric_name;
        std::string trend_direction; // "improving", "degrading", "stable"
        double trend_slope;
        double trend_strength;
        std::chrono::system_clock::time_point analysis_period_start;
        std::chrono::system_clock::time_point analysis_period_end;
        std::vector<double> data_points;
    };
    
    PerformanceTrend AnalyzeTrend(const std::string& metric_name, 
                                std::chrono::hours analysis_period = std::chrono::hours(24)) const;
    
    std::vector<PerformanceTrend> AnalyzeAllTrends(
        std::chrono::hours analysis_period = std::chrono::hours(24)) const;
    
    // Configuration and settings
    void SetOptimizationThreshold(OptimizationType type, double threshold);
    double GetOptimizationThreshold(OptimizationType type) const;
    void SetRegressionThreshold(const std::string& metric_name, double threshold);
    double GetRegressionThreshold(const std::string& metric_name) const;
    
    void EnableAutoAnalysis(bool enable);
    void EnableAutoRecommendations(bool enable);
    void SetAnalysisInterval(std::chrono::minutes interval);
    
    // Import/Export functionality
    bool ExportOptimizationData(const std::string& file_path) const;
    bool ImportOptimizationData(const std::string& file_path);
    bool ExportReport(const std::string& file_path, const std::string& format = "json") const;
    
    // Real-time monitoring hooks
    using PerformanceCallback = std::function<void(const std::string&, double)>;
    using RegressionCallback = std::function<void(const RegressionAlert&)>;
    using OptimizationCallback = std::function<void(const OptimizationOpportunity&)>;
    
    void SetPerformanceCallback(PerformanceCallback callback);
    void SetRegressionCallback(RegressionCallback callback);
    void SetOptimizationCallback(OptimizationCallback callback);
    
    // Batch operations
    std::vector<ValidationResult> ValidateOptimizations(const std::vector<std::string>& optimization_ids) const;
    void BatchUpdateOptimizations(const std::vector<std::string>& optimization_ids, OptimizationStatus status);
    
    // Statistics and metrics
    struct TrackerStatistics {
        size_t total_measurements_recorded;
        size_t total_optimizations_identified;
        size_t total_optimizations_implemented;
        size_t total_regressions_detected;
        size_t total_regression_alerts;
        double average_improvement_percentage;
        double total_performance_gain;
        std::chrono::system_clock::time_point tracker_started_at;
        std::chrono::system_clock::time_point last_analysis_at;
    };
    
    TrackerStatistics GetStatistics() const;
    void ResetStatistics();
    
    // Health check
    bool IsHealthy() const;
    std::vector<std::string> GetHealthIssues() const;
    void PerformHealthCheck();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Utility functions
std::string OptimizationTypeToString(OptimizationType type);
std::string OptimizationPriorityToString(OptimizationPriority priority);
std::string OptimizationStatusToString(OptimizationStatus status);

OptimizationType StringToOptimizationType(const std::string& type_str);
OptimizationPriority StringToOptimizationPriority(const std::string& priority_str);
OptimizationStatus StringToOptimizationStatus(const std::string& status_str);

double CalculateImprovementPercentage(double before, double after);
double CalculateRegressionPercentage(double baseline, double current);
bool IsSignificantImprovement(double improvement_percentage, double threshold = 5.0);
bool IsSignificantRegression(double regression_percentage, double threshold = 5.0);

std::vector<std::string> GetOptimizationRecommendations(OptimizationType type);
std::vector<std::string> GetCommonPerformanceIssues(OptimizationType type);
std::vector<std::string> GetOptimizationBestPractices(OptimizationType type);

} // namespace Production
} // namespace NeonGlyph