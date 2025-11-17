#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>

namespace NeonGlyph {
namespace Production {

struct VisualConsistencyMetrics {
    double color_consistency_score;      // 0.0-1.0
    double composition_stability_score;  // 0.0-1.0
    double character_density_variance;   // lower is better
    double brightness_consistency_score; // 0.0-1.0
    double contrast_consistency_score;   // 0.0-1.0
    double temporal_smoothness_score;    // 0.0-1.0
    double overall_consistency_score;    // 0.0-1.0
    std::vector<std::string> issues_found;
    std::vector<std::string> recommendations;
};

struct FrameAnalysisResult {
    std::vector<std::vector<char>> ascii_matrix;
    double character_density;
    double average_brightness;
    double contrast_ratio;
    std::map<char, int> character_frequency;
    std::vector<std::pair<int, int>> significant_changes;
    int width;
    int height;
};

class VisualConsistencyChecker {
private:
    double color_consistency_threshold_;
    double composition_stability_threshold_;
    double brightness_variance_threshold_;
    double contrast_variance_threshold_;
    double temporal_smoothness_threshold_;
    
public:
    VisualConsistencyChecker();
    
    // Set consistency thresholds
    void SetColorConsistencyThreshold(double threshold) { color_consistency_threshold_ = threshold; }
    void SetCompositionStabilityThreshold(double threshold) { composition_stability_threshold_ = threshold; }
    void SetBrightnessVarianceThreshold(double threshold) { brightness_variance_threshold_ = threshold; }
    void SetContrastVarianceThreshold(double threshold) { contrast_variance_threshold_ = threshold; }
    void SetTemporalSmoothnessThreshold(double threshold) { temporal_smoothness_threshold_ = threshold; }
    
    // Analyze single frame
    FrameAnalysisResult AnalyzeFrame(const std::string& ascii_frame);
    
    // Analyze frame sequence for consistency
    VisualConsistencyMetrics AnalyzeFrameSequence(const std::vector<std::string>& frames);
    
    // Compare two frames for consistency
    double CalculateFrameSimilarity(const std::string& frame1, const std::string& frame2);
    
    // Detect visual anomalies
    std::vector<std::pair<int, std::string>> DetectVisualAnomalies(const std::vector<std::string>& frames);
    
    // Generate consistency report
    std::string GenerateConsistencyReport(const VisualConsistencyMetrics& metrics);
    
    // Validate against standards
    bool IsVisuallyConsistent(const VisualConsistencyMetrics& metrics);
    
private:
    // Helper functions for frame analysis
    std::vector<std::vector<char>> ParseASCIIFrame(const std::string& frame);
    double CalculateCharacterDensity(const std::vector<std::vector<char>>& matrix);
    double CalculateAverageBrightness(const std::vector<std::vector<char>>& matrix);
    double CalculateContrastRatio(const std::vector<std::vector<char>>& matrix);
    std::map<char, int> CalculateCharacterFrequency(const std::vector<std::vector<char>>& matrix);
    
    // Helper functions for consistency analysis
    double CalculateColorConsistency(const std::vector<std::string>& frames);
    double CalculateCompositionStability(const std::vector<FrameAnalysisResult>& analyses);
    double CalculateTemporalSmoothness(const std::vector<FrameAnalysisResult>& analyses);
    double CalculateBrightnessConsistency(const std::vector<FrameAnalysisResult>& analyses);
    double CalculateContrastConsistency(const std::vector<FrameAnalysisResult>& analyses);
};

class ASCIIArtValidator {
public:
    // Validate ASCII character usage
    static bool ValidateCharacterSet(const std::string& ascii_art, const std::string& allowed_chars);
    static std::map<char, int> AnalyzeCharacterDistribution(const std::string& ascii_art);
    static bool HasBalancedCharacterUsage(const std::map<char, int>& distribution);
    
    // Validate ASCII composition
    static bool ValidateCompositionBalance(const std::string& ascii_art);
    static double CalculateCompositionBalanceScore(const std::string& ascii_art);
    static std::vector<std::pair<int, int>> FindCompositionImbalances(const std::string& ascii_art);
    
    // Validate ASCII density
    static double CalculateCharacterDensity(const std::string& ascii_art);
    static bool IsDensityAppropriate(double density, const std::string& intended_style);
    static std::string ClassifyDensityLevel(double density);
    
    // Validate ASCII readability
    static double CalculateReadabilityScore(const std::string& ascii_art);
    static bool HasAdequateSpacing(const std::string& ascii_art);
    static bool HasConsistentCharacterHeight(const std::string& ascii_art);
    
    // Validate ASCII artistic quality
    static double CalculateArtisticMeritScore(const std::string& ascii_art);
    static bool ShowsCreativeUseOfCharacters(const std::string& ascii_art);
    static bool DemonstratesVisualHierarchy(const std::string& ascii_art);
};

class ColorPaletteValidator {
public:
    // Simple color validation without external dependencies
    static bool ValidateColorFormat(const std::string& color);
    static std::vector<int> ParseHexColor(const std::string& hex_color);
    static double CalculateColorDistance(const std::vector<int>& color1, const std::vector<int>& color2);
    
    // Validate color palette consistency
    static bool ValidateColorHarmony(const std::vector<std::string>& colors);
    static double CalculateColorHarmonyScore(const std::vector<std::string>& colors);
    
    // Validate color accessibility (simplified)
    static bool ValidateBasicColorAccessibility(const std::vector<std::string>& colors);
    static double CalculateMinimumColorDistance(const std::vector<std::string>& colors);
    
    // Validate color consistency across frames
    static bool ValidateColorConsistency(const std::vector<std::vector<std::string>>& frame_colors);
    static double CalculateColorConsistencyScore(const std::vector<std::vector<std::string>>& frame_colors);
};

class AnimationValidator {
public:
    // Validate animation smoothness
    static bool ValidateAnimationSmoothness(const std::vector<std::string>& frames);
    static double CalculateAnimationSmoothnessScore(const std::vector<std::string>& frames);
    
    // Validate animation timing
    static bool ValidateAnimationTiming(const std::vector<std::string>& frames, 
                                      const std::vector<double>& frame_times);
    static double CalculateTimingAccuracyScore(const std::vector<double>& frame_times, 
                                               double target_fps);
    
    // Validate transition quality
    static bool ValidateTransitionQuality(const std::vector<std::string>& frames);
    static double CalculateTransitionQualityScore(const std::vector<std::string>& frames);
    
    // Validate beat synchronization (simplified)
    static bool ValidateBeatSynchronization(const std::vector<std::string>& frames,
                                          const std::vector<double>& beat_times);
    static double CalculateBeatSyncAccuracy(const std::vector<std::string>& frames,
                                          const std::vector<double>& beat_times);
    
    // Validate animation safety (epilepsy protection)
    static bool ValidateAnimationSafety(const std::vector<std::string>& frames);
    static double CalculateFlashFrequency(const std::vector<std::string>& frames);
    static bool IsAnimationEpilepsySafe(const std::vector<std::string>& frames);
};

class AutomatedValidationSystem {
private:
    std::unique_ptr<VisualConsistencyChecker> consistency_checker_;
    std::map<std::string, std::function<bool(const std::string&)>> single_frame_validators_;
    std::map<std::string, std::function<bool(const std::vector<std::string>&)>> sequence_validators_;
    
public:
    AutomatedValidationSystem();
    
    // Register custom validators
    void RegisterSingleFrameValidator(const std::string& name,
                                    const std::function<bool(const std::string&)>& validator);
    void RegisterSequenceValidator(const std::string& name,
                                 const std::function<bool(const std::vector<std::string>&)>& validator);
    
    // Run all validations on single frame
    std::map<std::string, bool> ValidateSingleFrame(const std::string& frame);
    
    // Run all validations on frame sequence
    std::map<std::string, bool> ValidateFrameSequence(const std::vector<std::string>& frames);
    
    // Run specific validation
    bool RunSpecificValidation(const std::string& validator_name, const std::string& frame);
    bool RunSpecificSequenceValidation(const std::string& validator_name, 
                                     const std::vector<std::string>& frames);
    
    // Generate comprehensive validation report
    std::string GenerateValidationReport(const std::string& frame);
    std::string GenerateSequenceValidationReport(const std::vector<std::string>& frames);
    
    // Batch validation for multiple themes
    std::map<std::string, std::map<std::string, bool>> 
        ValidateMultipleThemes(const std::map<std::string, std::vector<std::string>>& themes);
    
    // Get validation statistics
    std::map<std::string, double> GetValidationStatistics() const;
};

// Predefined validation suites
namespace ValidationSuites {
    // Basic visual validation suite
    std::vector<std::string> GetBasicVisualValidationSuite();
    
    // Comprehensive validation suite
    std::vector<std::string> GetComprehensiveValidationSuite();
    
    // Performance-focused validation suite
    std::vector<std::string> GetPerformanceValidationSuite();
    
    // Safety-focused validation suite
    std::vector<std::string> GetSafetyValidationSuite();
    
    // Artistic quality validation suite
    std::vector<std::string> GetArtisticQualityValidationSuite();
}

} // namespace Production
} // namespace NeonGlyph