#include "production/VisualConsistencyChecker2.h"
#include <sstream>
#include <iomanip>
#include <numeric>

namespace NeonGlyph {
namespace Production {

VisualConsistencyChecker::VisualConsistencyChecker() 
    : color_consistency_threshold_(0.8),
      composition_stability_threshold_(0.7),
      brightness_variance_threshold_(0.2),
      contrast_variance_threshold_(0.3),
      temporal_smoothness_threshold_(0.6) {
}

FrameAnalysisResult VisualConsistencyChecker::AnalyzeFrame(const std::string& ascii_frame) {
    FrameAnalysisResult result;
    
    // Parse ASCII frame into matrix
    result.ascii_matrix = ParseASCIIFrame(ascii_frame);
    result.width = result.ascii_matrix.empty() ? 0 : result.ascii_matrix[0].size();
    result.height = result.ascii_matrix.size();
    
    if (result.width > 0 && result.height > 0) {
        // Calculate frame metrics
        result.character_density = CalculateCharacterDensity(result.ascii_matrix);
        result.average_brightness = CalculateAverageBrightness(result.ascii_matrix);
        result.contrast_ratio = CalculateContrastRatio(result.ascii_matrix);
        result.character_frequency = CalculateCharacterFrequency(result.ascii_matrix);
    }
    
    return result;
}

VisualConsistencyMetrics VisualConsistencyChecker::AnalyzeFrameSequence(const std::vector<std::string>& frames) {
    VisualConsistencyMetrics metrics;
    
    if (frames.empty()) {
        metrics.overall_consistency_score = 0.0;
        metrics.issues_found.push_back("No frames provided for analysis");
        return metrics;
    }
    
    // Analyze each frame
    std::vector<FrameAnalysisResult> frame_analyses;
    for (const auto& frame : frames) {
        frame_analyses.push_back(AnalyzeFrame(frame));
    }
    
    // Calculate consistency metrics
    metrics.color_consistency_score = CalculateColorConsistency(frames);
    metrics.composition_stability_score = CalculateCompositionStability(frame_analyses);
    metrics.brightness_consistency_score = CalculateBrightnessConsistency(frame_analyses);
    metrics.contrast_consistency_score = CalculateContrastConsistency(frame_analyses);
    metrics.temporal_smoothness_score = CalculateTemporalSmoothness(frame_analyses);
    
    // Calculate overall consistency score (weighted average)
    metrics.overall_consistency_score = 
        (metrics.color_consistency_score * 0.2 +
         metrics.composition_stability_score * 0.25 +
         metrics.brightness_consistency_score * 0.2 +
         metrics.contrast_consistency_score * 0.15 +
         metrics.temporal_smoothness_score * 0.2);
    
    // Generate issues and recommendations
    if (metrics.color_consistency_score < color_consistency_threshold_) {
        metrics.issues_found.push_back("Low color consistency across frames");
        metrics.recommendations.push_back("Maintain consistent color palette");
    }
    
    if (metrics.composition_stability_score < composition_stability_threshold_) {
        metrics.issues_found.push_back("Composition instability detected");
        metrics.recommendations.push_back("Stabilize composition elements");
    }
    
    if (metrics.temporal_smoothness_score < temporal_smoothness_threshold_) {
        metrics.issues_found.push_back("Poor temporal smoothness");
        metrics.recommendations.push_back("Improve frame-to-frame transitions");
    }
    
    return metrics;
}

double VisualConsistencyChecker::CalculateFrameSimilarity(const std::string& frame1, const std::string& frame2) {
    if (frame1.length() != frame2.length()) {
        return 0.0; // Different sizes, not similar
    }
    
    int matching_chars = 0;
    int total_chars = frame1.length();
    
    for (size_t i = 0; i < frame1.length(); ++i) {
        if (frame1[i] == frame2[i]) {
            matching_chars++;
        }
    }
    
    return static_cast<double>(matching_chars) / static_cast<double>(total_chars);
}

std::vector<std::pair<int, std::string>> VisualConsistencyChecker::DetectVisualAnomalies(const std::vector<std::string>& frames) {
    std::vector<std::pair<int, std::string>> anomalies;
    
    if (frames.size() < 2) {
        return anomalies;
    }
    
    // Check for sudden changes between consecutive frames
    for (size_t i = 1; i < frames.size(); ++i) {
        double similarity = CalculateFrameSimilarity(frames[i-1], frames[i]);
        
        if (similarity < 0.3) { // Less than 30% similarity
            anomalies.push_back({static_cast<int>(i), "Sudden visual change detected"});
        }
        
        // Check for extreme character count changes
        int prev_count = std::count_if(frames[i-1].begin(), frames[i-1].end(), 
                                      [](char c) { return c != ' ' && c != '\n'; });
        int curr_count = std::count_if(frames[i].begin(), frames[i].end(), 
                                      [](char c) { return c != ' ' && c != '\n'; });
        
        if (prev_count > 0) {
            double change_ratio = std::abs(curr_count - prev_count) / static_cast<double>(prev_count);
            if (change_ratio > 0.8) { // More than 80% change
                anomalies.push_back({static_cast<int>(i), "Extreme character density change"});
            }
        }
    }
    
    return anomalies;
}

std::string VisualConsistencyChecker::GenerateConsistencyReport(const VisualConsistencyMetrics& metrics) {
    std::ostringstream report;
    
    report << "Visual Consistency Report\n";
    report << "========================\n";
    report << "Overall Consistency Score: " << std::fixed << std::setprecision(2) 
           << metrics.overall_consistency_score * 100.0 << "%\n\n";
    
    report << "Detailed Metrics:\n";
    report << "  Color Consistency: " << std::fixed << std::setprecision(2) 
           << metrics.color_consistency_score * 100.0 << "%\n";
    report << "  Composition Stability: " << std::fixed << std::setprecision(2) 
           << metrics.composition_stability_score * 100.0 << "%\n";
    report << "  Brightness Consistency: " << std::fixed << std::setprecision(2) 
           << metrics.brightness_consistency_score * 100.0 << "%\n";
    report << "  Contrast Consistency: " << std::fixed << std::setprecision(2) 
           << metrics.contrast_consistency_score * 100.0 << "%\n";
    report << "  Temporal Smoothness: " << std::fixed << std::setprecision(2) 
           << metrics.temporal_smoothness_score * 100.0 << "%\n\n";
    
    if (!metrics.issues_found.empty()) {
        report << "Issues Found:\n";
        for (const auto& issue : metrics.issues_found) {
            report << "  - " << issue << "\n";
        }
        report << "\n";
    }
    
    if (!metrics.recommendations.empty()) {
        report << "Recommendations:\n";
        for (const auto& rec : metrics.recommendations) {
            report << "  - " << rec << "\n";
        }
    }
    
    return report.str();
}

bool VisualConsistencyChecker::IsVisuallyConsistent(const VisualConsistencyMetrics& metrics) {
    return metrics.overall_consistency_score >= 0.7 && // 70% overall threshold
           metrics.color_consistency_score >= color_consistency_threshold_ &&
           metrics.composition_stability_score >= composition_stability_threshold_ &&
           metrics.temporal_smoothness_score >= temporal_smoothness_threshold_;
}

// Private helper functions
std::vector<std::vector<char>> VisualConsistencyChecker::ParseASCIIFrame(const std::string& frame) {
    std::vector<std::vector<char>> matrix;
    std::vector<char> current_row;
    
    for (char c : frame) {
        if (c == '\n') {
            if (!current_row.empty()) {
                matrix.push_back(current_row);
                current_row.clear();
            }
        } else {
            current_row.push_back(c);
        }
    }
    
    if (!current_row.empty()) {
        matrix.push_back(current_row);
    }
    
    return matrix;
}

double VisualConsistencyChecker::CalculateCharacterDensity(const std::vector<std::vector<char>>& matrix) {
    if (matrix.empty() || matrix[0].empty()) {
        return 0.0;
    }
    
    int total_chars = 0;
    int non_space_chars = 0;
    
    for (const auto& row : matrix) {
        for (char c : row) {
            total_chars++;
            if (c != ' ') {
                non_space_chars++;
            }
        }
    }
    
    return static_cast<double>(non_space_chars) / static_cast<double>(total_chars);
}

double VisualConsistencyChecker::CalculateAverageBrightness(const std::vector<std::vector<char>>& matrix) {
    if (matrix.empty() || matrix[0].empty()) {
        return 0.0;
    }
    
    // Simple brightness calculation based on character density
    const std::string brightness_chars = " .:-=+*#%@";
    double total_brightness = 0.0;
    int char_count = 0;
    
    for (const auto& row : matrix) {
        for (char c : row) {
            size_t pos = brightness_chars.find(c);
            if (pos != std::string::npos) {
                // Higher position = higher brightness
                total_brightness += static_cast<double>(pos) / static_cast<double>(brightness_chars.length() - 1);
            } else {
                // Unknown character, assume medium brightness
                total_brightness += 0.5;
            }
            char_count++;
        }
    }
    
    return char_count > 0 ? total_brightness / static_cast<double>(char_count) : 0.0;
}

double VisualConsistencyChecker::CalculateContrastRatio(const std::vector<std::vector<char>>& matrix) {
    if (matrix.empty() || matrix[0].empty()) {
        return 0.0;
    }
    
    // Simple contrast calculation
    const std::string brightness_chars = " .:-=+*#%@";
    double max_brightness = 0.0;
    double min_brightness = 1.0;
    
    for (const auto& row : matrix) {
        for (char c : row) {
            size_t pos = brightness_chars.find(c);
            if (pos != std::string::npos) {
                double brightness = static_cast<double>(pos) / static_cast<double>(brightness_chars.length() - 1);
                max_brightness = std::max(max_brightness, brightness);
                min_brightness = std::min(min_brightness, brightness);
            }
        }
    }
    
    return max_brightness - min_brightness;
}

std::map<char, int> VisualConsistencyChecker::CalculateCharacterFrequency(const std::vector<std::vector<char>>& matrix) {
    std::map<char, int> frequency;
    
    for (const auto& row : matrix) {
        for (char c : row) {
            frequency[c]++;
        }
    }
    
    return frequency;
}

double VisualConsistencyChecker::CalculateColorConsistency(const std::vector<std::string>& frames) {
    // Simplified color consistency based on character usage patterns
    if (frames.size() < 2) {
        return 1.0; // Single frame is perfectly consistent
    }
    
    std::vector<std::map<char, int>> frame_distributions;
    for (const auto& frame : frames) {
        FrameAnalysisResult analysis = AnalyzeFrame(frame);
        frame_distributions.push_back(analysis.character_frequency);
    }
    
    double total_consistency = 0.0;
    int comparisons = 0;
    
    // Compare consecutive frames
    for (size_t i = 1; i < frame_distributions.size(); ++i) {
        const auto& prev_dist = frame_distributions[i-1];
        const auto& curr_dist = frame_distributions[i];
        
        // Calculate distribution similarity
        double similarity = 0.0;
        int total_chars = 0;
        
        for (const auto& [char_type, count] : prev_dist) {
            int prev_count = count;
            int curr_count = curr_dist.count(char_type) ? curr_dist.at(char_type) : 0;
            
            int max_count = std::max(prev_count, curr_count);
            if (max_count > 0) {
                similarity += 1.0 - (std::abs(prev_count - curr_count) / static_cast<double>(max_count));
            }
            total_chars += max_count;
        }
        
        if (total_chars > 0) {
            total_consistency += similarity / static_cast<double>(prev_dist.size());
            comparisons++;
        }
    }
    
    return comparisons > 0 ? total_consistency / static_cast<double>(comparisons) : 1.0;
}

double VisualConsistencyChecker::CalculateCompositionStability(const std::vector<FrameAnalysisResult>& analyses) {
    if (analyses.size() < 2) {
        return 1.0; // Single frame is perfectly stable
    }
    
    double total_stability = 0.0;
    int comparisons = 0;
    
    // Compare consecutive frames
    for (size_t i = 1; i < analyses.size(); ++i) {
        const auto& prev = analyses[i-1];
        const auto& curr = analyses[i];
        
        // Compare density stability
        double density_diff = std::abs(prev.character_density - curr.character_density);
        double density_stability = 1.0 - std::min(density_diff, 1.0);
        
        // Compare brightness stability
        double brightness_diff = std::abs(prev.average_brightness - curr.average_brightness);
        double brightness_stability = 1.0 - std::min(brightness_diff, 1.0);
        
        // Compare contrast stability
        double contrast_diff = std::abs(prev.contrast_ratio - curr.contrast_ratio);
        double contrast_stability = 1.0 - std::min(contrast_diff, 1.0);
        
        // Average stability metrics
        double frame_stability = (density_stability + brightness_stability + contrast_stability) / 3.0;
        total_stability += frame_stability;
        comparisons++;
    }
    
    return comparisons > 0 ? total_stability / static_cast<double>(comparisons) : 1.0;
}

double VisualConsistencyChecker::CalculateTemporalSmoothness(const std::vector<FrameAnalysisResult>& analyses) {
    if (analyses.size() < 3) {
        return 1.0; // Need at least 3 frames for smoothness analysis
    }
    
    double total_smoothness = 0.0;
    int measurements = 0;
    
    // Analyze smoothness of changes over time
    for (size_t i = 2; i < analyses.size(); ++i) {
        const auto& frame1 = analyses[i-2];
        const auto& frame2 = analyses[i-1];
        const auto& frame3 = analyses[i];
        
        // Calculate first derivative (rate of change)
        double density_deriv1 = frame2.character_density - frame1.character_density;
        double density_deriv2 = frame3.character_density - frame2.character_density;
        
        // Calculate second derivative (acceleration)
        double density_acceleration = std::abs(density_deriv2 - density_deriv1);
        
        // Lower acceleration = smoother transition
        double smoothness = 1.0 - std::min(density_acceleration, 1.0);
        total_smoothness += smoothness;
        measurements++;
    }
    
    return measurements > 0 ? total_smoothness / static_cast<double>(measurements) : 1.0;
}

double VisualConsistencyChecker::CalculateBrightnessConsistency(const std::vector<FrameAnalysisResult>& analyses) {
    if (analyses.empty()) {
        return 1.0;
    }
    
    // Calculate variance in brightness
    double total_brightness = 0.0;
    for (const auto& analysis : analyses) {
        total_brightness += analysis.average_brightness;
    }
    
    double mean_brightness = total_brightness / static_cast<double>(analyses.size());
    
    double variance = 0.0;
    for (const auto& analysis : analyses) {
        double diff = analysis.average_brightness - mean_brightness;
        variance += diff * diff;
    }
    
    variance /= static_cast<double>(analyses.size());
    double std_dev = std::sqrt(variance);
    
    // Convert standard deviation to consistency score (lower std_dev = higher consistency)
    return 1.0 - std::min(std_dev, 1.0);
}

double VisualConsistencyChecker::CalculateContrastConsistency(const std::vector<FrameAnalysisResult>& analyses) {
    if (analyses.empty()) {
        return 1.0;
    }
    
    // Calculate variance in contrast
    double total_contrast = 0.0;
    for (const auto& analysis : analyses) {
        total_contrast += analysis.contrast_ratio;
    }
    
    double mean_contrast = total_contrast / static_cast<double>(analyses.size());
    
    double variance = 0.0;
    for (const auto& analysis : analyses) {
        double diff = analysis.contrast_ratio - mean_contrast;
        variance += diff * diff;
    }
    
    variance /= static_cast<double>(analyses.size());
    double std_dev = std::sqrt(variance);
    
    // Convert standard deviation to consistency score
    return 1.0 - std::min(std_dev, 1.0);
}

} // namespace Production
} // namespace NeonGlyph