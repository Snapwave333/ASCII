#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>

namespace NeonGlyph {
namespace Production {

enum class QualityGate {
    TechnicalValidation,
    VisualQualityReview,
    SafetyValidation,
    UserExperienceTesting
};

enum class TestResult {
    Pass,
    Fail,
    Warning,
    NotApplicable
};

struct TestMetric {
    std::string name;
    std::string description;
    double target_value;
    double acceptable_range_min;
    double acceptable_range_max;
    double critical_threshold;
    TestResult result;
    std::string notes;
};

struct QualityGateResult {
    QualityGate gate_type;
    std::string gate_name;
    std::vector<TestMetric> metrics;
    bool overall_pass;
    std::chrono::system_clock::time_point test_date;
    std::string tester_name;
    std::string approval_status;
    std::vector<std::string> recommendations;
};

class QualityAssuranceFramework {
private:
    std::map<QualityGate, std::vector<std::function<TestResult()>>> test_functions_;
    std::map<QualityGate, std::string> gate_descriptions_;
    std::vector<QualityGateResult> test_history_;
    
public:
    QualityAssuranceFramework();
    
    // Initialize standard quality gates and tests
    void InitializeStandardQualityGates();
    
    // Execute quality gate
    QualityGateResult ExecuteQualityGate(QualityGate gate);
    
    // Execute all quality gates
    std::vector<QualityGateResult> ExecuteAllQualityGates();
    
    // Get quality gate description
    std::string GetQualityGateDescription(QualityGate gate) const;
    
    // Add custom test to quality gate
    void AddTestToGate(QualityGate gate, const std::function<TestResult()>& test_function);
    
    // Get test history
    std::vector<QualityGateResult> GetTestHistory() const { return test_history_; }
    
    // Get latest test results
    std::vector<QualityGateResult> GetLatestTestResults() const;
    
    // Generate comprehensive QA report
    std::string GenerateQAReport() const;
    
    // Check if theme passes all quality gates
    bool IsThemeProductionReady() const;
};

class TechnicalValidator {
public:
    // Performance tests
    static TestMetric TestFrameRate(double target_fps, double measured_fps);
    static TestMetric TestLatency(double target_latency_ms, double measured_latency_ms);
    static TestMetric TestMemoryUsage(size_t max_memory_mb, size_t measured_memory_mb);
    static TestMetric TestCPUUsage(double max_cpu_percent, double measured_cpu_percent);
    static TestMetric TestGPUMemoryUsage(size_t max_gpu_memory_mb, size_t measured_gpu_memory_mb);
    static TestMetric TestDrawCalls(int max_draw_calls, int measured_draw_calls);
    
    // Compatibility tests
    static TestMetric TestResolutionSupport(int width, int height);
    static TestMetric TestHardwareCompatibility(const std::string& hardware_profile);
    static TestMetric TestDriverCompatibility(const std::string& driver_version);
    
    // Stability tests
    static TestMetric TestCrashRecovery(bool crash_recovery_enabled);
    static TestMetric TestMemoryLeaks(size_t memory_growth_mb);
    static TestMetric TestResourceCleanup(bool resources_cleaned);
    
    // Security tests
    static TestMetric TestInputValidation(bool input_validated);
    static TestMetric TestResourceLimits(bool limits_enforced);
    static TestMetric TestSandboxing(bool sandbox_enabled);
};

class VisualQualityValidator {
public:
    // Aesthetic quality tests
    static TestMetric TestColorHarmony(const std::string& color_palette);
    static TestMetric TestVisualConsistency(const std::vector<std::string>& frame_sequence);
    static TestMetric TestCompositionBalance(const std::string& ascii_art);
    static TestMetric TestCharacterReadability(const std::string& character_set);
    static TestMetric TestAnimationSmoothness(const std::vector<std::string>& animation_frames);
    
    // Technical visual tests
    static TestMetric TestContrastRatio(double min_contrast, double measured_contrast);
    static TestMetric TestBrightnessConsistency(float brightness_variance);
    static TestMetric TestColorBlindAccessibility(const std::string& palette);
    
    // Artistic merit tests
    static TestMetric TestCreativeExpression(int creativity_score);
    static TestMetric TestAestheticAppeal(int aesthetic_rating);
    static TestMetric TestThemeAdherence(int theme_fidelity_score);
    
    // Animation quality tests
    static TestMetric TestTimingAccuracy(const std::vector<std::chrono::milliseconds>& frame_times);
    static TestMetric TestTransitionSmoothness(const std::vector<std::string>& transitions);
    static TestMetric TestBeatSynchronization(float beat_sync_accuracy);
};

class SafetyValidator {
public:
    // Epilepsy protection tests
    static TestMetric TestFlashFrequency(float max_flashes_per_second, float measured_flashes);
    static TestMetric TestLuminanceChanges(float max_luminance_delta, float measured_delta);
    static TestMetric TestPatternStability(bool stable_patterns);
    static TestMetric TestRedFlashElimination(bool red_flash_safe);
    
    // Content filtering tests
    static TestMetric TestNSFWDetection(bool nsfw_content_detected);
    static TestMetric TestInappropriateContent(bool inappropriate_content_found);
    static TestMetric TestCopyrightCompliance(bool copyright_compliant);
    
    // Performance safety tests
    static TestMetric TestThermalThrottling(bool thermal_limits_respected);
    static TestMetric TestPowerConsumption(float max_power_watts, float measured_power);
    static TestMetric TestFanNoise(float max_noise_db, float measured_noise);
    
    // Emergency safety tests
    static TestMetric TestEmergencyStop(bool emergency_stop_functional);
    static TestMetric TestFallbackMechanisms(bool fallbacks_working);
    static TestMetric TestRecoveryProcedures(bool recovery_successful);
};

class UserExperienceValidator {
public:
    // Usability tests
    static TestMetric TestEaseOfUse(int ease_of_use_score);
    static TestMetric TestLearningCurve(int learning_difficulty_score);
    static TestMetric TestIntuitiveness(int intuitiveness_score);
    static TestMetric TestCustomizationOptions(int customization_score);
    
    // Performance perception tests
    static TestMetric TestResponsiveness(int responsiveness_score);
    static TestMetric TestSmoothness(int smoothness_score);
    static TestMetric TestReliability(int reliability_score);
    static TestMetric TestStability(int stability_score);
    
    // Integration tests
    static TestMetric TestSoftwareIntegration(bool integrates_well);
    static TestMetric TestHardwareCompatibility(bool hardware_compatible);
    static TestMetric TestWorkflowIntegration(bool workflow_compatible);
    
    // User satisfaction tests
    static TestMetric TestOverallSatisfaction(int satisfaction_score);
    static TestMetric TestVisualAppeal(int visual_appeal_score);
    static TestMetric TestFunctionalSatisfaction(int functional_score);
    static TestMetric TestLikelihoodToRecommend(int nps_score);
};

class AutomatedTestingFramework {
private:
    std::map<std::string, std::function<TestResult()>> automated_tests_;
    std::chrono::seconds test_timeout_;
    
public:
    AutomatedTestingFramework();
    
    // Register automated test
    void RegisterAutomatedTest(const std::string& test_name, 
                             const std::function<TestResult()>& test_function);
    
    // Run single automated test
    TestResult RunAutomatedTest(const std::string& test_name);
    
    // Run all automated tests
    std::map<std::string, TestResult> RunAllAutomatedTests();
    
    // Run tests with timeout
    TestResult RunTestWithTimeout(const std::string& test_name, 
                                std::chrono::seconds timeout);
    
    // Generate automated test report
    std::string GenerateAutomatedTestReport();
    
    // Set test timeout
    void SetTestTimeout(std::chrono::seconds timeout) { test_timeout_ = timeout; }
};

// Predefined test suites
namespace StandardTestSuites {
    // Comprehensive production test suite
    std::vector<std::function<TestResult()>> GetProductionTestSuite();
    
    // Quick validation test suite
    std::vector<std::function<TestResult()>> GetQuickValidationSuite();
    
    // Performance-focused test suite
    std::vector<std::function<TestResult()>> GetPerformanceTestSuite();
    
    // Safety-focused test suite
    std::vector<std::function<TestResult()>> GetSafetyTestSuite();
    
    // Visual quality test suite
    std::vector<std::function<TestResult()>> GetVisualQualityTestSuite();
}

} // namespace Production
} // namespace NeonGlyph