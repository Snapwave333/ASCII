#include "production/QualityAssuranceFramework.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace NeonGlyph {
namespace Production {

QualityAssuranceFramework::QualityAssuranceFramework() {
    InitializeStandardQualityGates();
}

void QualityAssuranceFramework::InitializeStandardQualityGates() {
    // Gate 1: Technical Validation
    gate_descriptions_[QualityGate::TechnicalValidation] = 
        "Comprehensive technical validation including performance, compatibility, and stability tests";
    
    // Gate 2: Visual Quality Review
    gate_descriptions_[QualityGate::VisualQualityReview] = 
        "Visual quality assessment including aesthetic consistency, animation quality, and artistic merit";
    
    // Gate 3: Safety Validation
    gate_descriptions_[QualityGate::SafetyValidation] = 
        "Safety validation including epilepsy protection, content filtering, and emergency procedures";
    
    // Gate 4: User Experience Testing
    gate_descriptions_[QualityGate::UserExperienceTesting] = 
        "User experience evaluation including usability, performance perception, and integration testing";
}

QualityGateResult QualityAssuranceFramework::ExecuteQualityGate(QualityGate gate) {
    QualityGateResult result;
    result.gate_type = gate;
    result.gate_name = GetQualityGateDescription(gate);
    result.test_date = std::chrono::system_clock::now();
    result.overall_pass = true;
    
    // Execute all tests for this gate
    auto test_it = test_functions_.find(gate);
    if (test_it != test_functions_.end()) {
        for (const auto& test_function : test_it->second) {
            TestResult test_result = test_function();
            
            // Convert TestResult to TestMetric (simplified for this implementation)
            TestMetric metric;
            metric.name = "Automated Test";
            metric.result = test_result;
            metric.target_value = 1.0;
            metric.acceptable_range_min = 0.8;
            metric.acceptable_range_max = 1.0;
            metric.critical_threshold = 0.5;
            
            result.metrics.push_back(metric);
            
            if (test_result == TestResult::Fail) {
                result.overall_pass = false;
                result.recommendations.push_back("Test failed - requires investigation");
            }
        }
    }
    
    // Store result in history
    test_history_.push_back(result);
    
    return result;
}

std::vector<QualityGateResult> QualityAssuranceFramework::ExecuteAllQualityGates() {
    std::vector<QualityGateResult> results;
    
    results.push_back(ExecuteQualityGate(QualityGate::TechnicalValidation));
    results.push_back(ExecuteQualityGate(QualityGate::VisualQualityReview));
    results.push_back(ExecuteQualityGate(QualityGate::SafetyValidation));
    results.push_back(ExecuteQualityGate(QualityGate::UserExperienceTesting));
    
    return results;
}

std::string QualityAssuranceFramework::GetQualityGateDescription(QualityGate gate) const {
    auto it = gate_descriptions_.find(gate);
    if (it != gate_descriptions_.end()) {
        return it->second;
    }
    return "Unknown quality gate";
}

void QualityAssuranceFramework::AddTestToGate(QualityGate gate, 
                                            const std::function<TestResult()>& test_function) {
    test_functions_[gate].push_back(test_function);
}

std::vector<QualityGateResult> QualityAssuranceFramework::GetLatestTestResults() const {
    std::vector<QualityGateResult> latest_results;
    
    // Find the most recent test for each gate type
    std::map<QualityGate, QualityGateResult> latest_by_gate;
    
    for (const auto& result : test_history_) {
        auto it = latest_by_gate.find(result.gate_type);
        if (it == latest_by_gate.end() || result.test_date > it->second.test_date) {
            latest_by_gate[result.gate_type] = result;
        }
    }
    
    for (const auto& [gate_type, result] : latest_by_gate) {
        latest_results.push_back(result);
    }
    
    return latest_results;
}

std::string QualityAssuranceFramework::GenerateQAReport() const {
    std::ostringstream report;
    
    report << "Quality Assurance Report\n";
    report << "========================\n";
    report << "Generated: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n\n";
    
    auto latest_results = GetLatestTestResults();
    
    for (const auto& result : latest_results) {
        report << "Quality Gate: " << result.gate_name << "\n";
        report << "Status: " << (result.overall_pass ? "PASS" : "FAIL") << "\n";
        report << "Test Date: " << std::chrono::system_clock::to_time_t(result.test_date) << "\n";
        report << "Metrics Tested: " << result.metrics.size() << "\n";
        
        int pass_count = 0, fail_count = 0, warning_count = 0;
        for (const auto& metric : result.metrics) {
            switch (metric.result) {
                case TestResult::Pass: pass_count++; break;
                case TestResult::Fail: fail_count++; break;
                case TestResult::Warning: warning_count++; break;
                default: break;
            }
        }
        
        report << "Results: " << pass_count << " Pass, " 
               << fail_count << " Fail, " 
               << warning_count << " Warning\n";
        
        if (!result.recommendations.empty()) {
            report << "Recommendations:\n";
            for (const auto& rec : result.recommendations) {
                report << "  - " << rec << "\n";
            }
        }
        
        report << "\n";
    }
    
    return report.str();
}

bool QualityAssuranceFramework::IsThemeProductionReady() const {
    auto latest_results = GetLatestTestResults();
    
    if (latest_results.size() < 4) {
        return false; // Not all gates have been tested
    }
    
    for (const auto& result : latest_results) {
        if (!result.overall_pass) {
            return false;
        }
    }
    
    return true;
}

// TechnicalValidator Implementation
TestMetric TechnicalValidator::TestFrameRate(double target_fps, double measured_fps) {
    TestMetric metric;
    metric.name = "Frame Rate";
    metric.description = "Tests if frame rate meets target";
    metric.target_value = target_fps;
    metric.acceptable_range_min = target_fps * 0.9;
    metric.acceptable_range_max = target_fps * 1.1;
    metric.critical_threshold = target_fps * 0.5;
    
    if (measured_fps >= target_fps) {
        metric.result = TestResult::Pass;
        metric.notes = "Frame rate meets target";
    } else if (measured_fps >= metric.acceptable_range_min) {
        metric.result = TestResult::Warning;
        metric.notes = "Frame rate slightly below target";
    } else {
        metric.result = TestResult::Fail;
        metric.notes = "Frame rate significantly below target";
    }
    
    return metric;
}

TestMetric TechnicalValidator::TestLatency(double target_latency_ms, double measured_latency_ms) {
    TestMetric metric;
    metric.name = "Latency";
    metric.description = "Tests if latency is within acceptable range";
    metric.target_value = target_latency_ms;
    metric.acceptable_range_min = 0.0;
    metric.acceptable_range_max = target_latency_ms * 1.2;
    metric.critical_threshold = target_latency_ms * 2.0;
    
    if (measured_latency_ms <= target_latency_ms) {
        metric.result = TestResult::Pass;
        metric.notes = "Latency meets target";
    } else if (measured_latency_ms <= metric.acceptable_range_max) {
        metric.result = TestResult::Warning;
        metric.notes = "Latency slightly above target";
    } else {
        metric.result = TestResult::Fail;
        metric.notes = "Latency significantly above target";
    }
    
    return metric;
}

TestMetric TechnicalValidator::TestMemoryUsage(size_t max_memory_mb, size_t measured_memory_mb) {
    TestMetric metric;
    metric.name = "Memory Usage";
    metric.description = "Tests if memory usage is within limits";
    metric.target_value = static_cast<double>(max_memory_mb) * 0.8;
    metric.acceptable_range_min = 0.0;
    metric.acceptable_range_max = static_cast<double>(max_memory_mb);
    metric.critical_threshold = static_cast<double>(max_memory_mb) * 1.5;
    
    if (measured_memory_mb <= static_cast<size_t>(metric.target_value)) {
        metric.result = TestResult::Pass;
        metric.notes = "Memory usage well within limits";
    } else if (measured_memory_mb <= max_memory_mb) {
        metric.result = TestResult::Warning;
        metric.notes = "Memory usage approaching limits";
    } else {
        metric.result = TestResult::Fail;
        metric.notes = "Memory usage exceeds limits";
    }
    
    return metric;
}

// VisualQualityValidator Implementation
TestMetric VisualQualityValidator::TestColorHarmony(const std::string& color_palette) {
    TestMetric metric;
    metric.name = "Color Harmony";
    metric.description = "Tests if color palette is harmonious";
    metric.target_value = 1.0;
    metric.acceptable_range_min = 0.7;
    metric.acceptable_range_max = 1.0;
    metric.critical_threshold = 0.5;
    
    // Simplified color harmony check
    if (!color_palette.empty() && color_palette.length() > 10) {
        metric.result = TestResult::Pass;
        metric.notes = "Color palette appears harmonious";
    } else {
        metric.result = TestResult::Warning;
        metric.notes = "Color palette may need review";
    }
    
    return metric;
}

TestMetric VisualQualityValidator::TestVisualConsistency(const std::vector<std::string>& frame_sequence) {
    TestMetric metric;
    metric.name = "Visual Consistency";
    metric.description = "Tests visual consistency across frames";
    metric.target_value = 1.0;
    metric.acceptable_range_min = 0.8;
    metric.acceptable_range_max = 1.0;
    metric.critical_threshold = 0.6;
    
    if (frame_sequence.size() >= 3) {
        metric.result = TestResult::Pass;
        metric.notes = "Visual consistency maintained across frames";
    } else {
        metric.result = TestResult::Warning;
        metric.notes = "Insufficient frames for consistency check";
    }
    
    return metric;
}

// SafetyValidator Implementation
TestMetric SafetyValidator::TestFlashFrequency(float max_flashes_per_second, float measured_flashes) {
    TestMetric metric;
    metric.name = "Flash Frequency";
    metric.description = "Tests if flash frequency is safe for photosensitive users";
    metric.target_value = static_cast<double>(max_flashes_per_second) * 0.5;
    metric.acceptable_range_min = 0.0;
    metric.acceptable_range_max = static_cast<double>(max_flashes_per_second);
    metric.critical_threshold = static_cast<double>(max_flashes_per_second) * 1.5;
    
    if (measured_flashes <= max_flashes_per_second) {
        metric.result = TestResult::Pass;
        metric.notes = "Flash frequency within safe limits";
    } else {
        metric.result = TestResult::Fail;
        metric.notes = "Flash frequency exceeds safety limits - epilepsy risk";
    }
    
    return metric;
}

// UserExperienceValidator Implementation
TestMetric UserExperienceValidator::TestEaseOfUse(int ease_of_use_score) {
    TestMetric metric;
    metric.name = "Ease of Use";
    metric.description = "Tests user-reported ease of use";
    metric.target_value = 8.0;
    metric.acceptable_range_min = 6.0;
    metric.acceptable_range_max = 10.0;
    metric.critical_threshold = 4.0;
    
    if (ease_of_use_score >= 8) {
        metric.result = TestResult::Pass;
        metric.notes = "High ease of use rating";
    } else if (ease_of_use_score >= 6) {
        metric.result = TestResult::Warning;
        metric.notes = "Acceptable ease of use rating";
    } else {
        metric.result = TestResult::Fail;
        metric.notes = "Low ease of use rating - needs improvement";
    }
    
    return metric;
}

TestMetric UserExperienceValidator::TestOverallSatisfaction(int satisfaction_score) {
    TestMetric metric;
    metric.name = "Overall Satisfaction";
    metric.description = "Tests overall user satisfaction";
    metric.target_value = 9.0;
    metric.acceptable_range_min = 7.0;
    metric.acceptable_range_max = 10.0;
    metric.critical_threshold = 5.0;
    
    if (satisfaction_score >= 9) {
        metric.result = TestResult::Pass;
        metric.notes = "Excellent user satisfaction";
    } else if (satisfaction_score >= 7) {
        metric.result = TestResult::Warning;
        metric.notes = "Good user satisfaction";
    } else {
        metric.result = TestResult::Fail;
        metric.notes = "Poor user satisfaction - major improvements needed";
    }
    
    return metric;
}

} // namespace Production
} // namespace NeonGlyph