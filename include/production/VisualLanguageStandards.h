#pragma once

#include <string>
#include <vector>
#include <map>
#include <array>

namespace NeonGlyph {
namespace Production {

struct ColorPalette {
    std::string primary;
    std::string secondary;
    std::string accent;
    std::string background;
    std::string highlight;
    
    bool IsValid() const {
        return !primary.empty() && !secondary.empty() && !accent.empty() && 
               !background.empty() && !highlight.empty();
    }
};

struct CharacterSet {
    std::string characters;
    std::string name;
    std::string description;
    int density_level; // 1-10, higher = more dense
    
    bool IsValid() const {
        return !characters.empty() && !name.empty() && density_level >= 1 && density_level <= 10;
    }
};

struct CompositionRules {
    float rule_of_thirds_weight;     // 0.0-1.0
    float visual_hierarchy_weight;   // 0.0-1.0
    float negative_space_ratio;      // 0.0-1.0 (target 0.2-0.4)
    float reading_flow_priority;     // 0.0-1.0
    
    bool IsValid() const {
        return rule_of_thirds_weight >= 0.0f && rule_of_thirds_weight <= 1.0f &&
               visual_hierarchy_weight >= 0.0f && visual_hierarchy_weight <= 1.0f &&
               negative_space_ratio >= 0.0f && negative_space_ratio <= 1.0f &&
               reading_flow_priority >= 0.0f && reading_flow_priority <= 1.0f;
    }
};

struct AnimationTiming {
    float beat_synchronization_interval; // seconds
    float transition_duration;           // seconds (0.2-0.5)
    float hold_time_min;                 // seconds (0.5)
    float hold_time_max;                 // seconds (2.0)
    float max_flash_rate;                // flashes per second (max 3)
    
    bool IsValid() const {
        return beat_synchronization_interval > 0.0f &&
               transition_duration >= 0.1f && transition_duration <= 2.0f &&
               hold_time_min >= 0.1f && hold_time_max >= hold_time_min &&
               max_flash_rate > 0.0f && max_flash_rate <= 3.0f;
    }
};

struct TechnicalSpecifications {
    int min_resolution_width;
    int min_resolution_height;
    int target_resolution_width;
    int target_resolution_height;
    int max_resolution_width;
    int max_resolution_height;
    float min_frame_rate;
    float target_frame_rate;
    float max_latency_ms;
    float max_cpu_usage;
    size_t max_memory_mb;
    size_t max_gpu_memory_mb;
    
    bool IsValid() const {
        return min_resolution_width > 0 && min_resolution_height > 0 &&
               target_resolution_width >= min_resolution_width &&
               target_resolution_height >= min_resolution_height &&
               max_resolution_width >= target_resolution_width &&
               max_resolution_height >= target_resolution_height &&
               min_frame_rate > 0.0f && target_frame_rate >= min_frame_rate &&
               max_latency_ms > 0.0f && max_cpu_usage > 0.0f &&
               max_memory_mb > 0 && max_gpu_memory_mb > 0;
    }
};

class VisualLanguageStandards {
private:
    std::map<std::string, ColorPalette> standard_palettes_;
    std::map<std::string, CharacterSet> standard_charsets_;
    CompositionRules default_composition_rules_;
    AnimationTiming default_animation_timing_;
    TechnicalSpecifications default_technical_specs_;
    
public:
    VisualLanguageStandards();
    
    // Initialize standard palettes and character sets
    void InitializeStandards();
    
    // Color palette management
    void AddStandardPalette(const std::string& name, const ColorPalette& palette);
    ColorPalette GetStandardPalette(const std::string& name) const;
    std::vector<std::string> GetAvailablePalettes() const;
    bool ValidatePalette(const ColorPalette& palette) const;
    
    // Character set management
    void AddStandardCharset(const std::string& name, const CharacterSet& charset);
    CharacterSet GetStandardCharset(const std::string& name) const;
    std::vector<std::string> GetAvailableCharsets() const;
    bool ValidateCharset(const CharacterSet& charset) const;
    
    // Composition rules
    void SetDefaultCompositionRules(const CompositionRules& rules);
    CompositionRules GetDefaultCompositionRules() const;
    bool ValidateCompositionRules(const CompositionRules& rules) const;
    
    // Animation timing
    void SetDefaultAnimationTiming(const AnimationTiming& timing);
    AnimationTiming GetDefaultAnimationTiming() const;
    bool ValidateAnimationTiming(const AnimationTiming& timing) const;
    
    // Technical specifications
    void SetDefaultTechnicalSpecifications(const TechnicalSpecifications& specs);
    TechnicalSpecifications GetDefaultTechnicalSpecifications() const;
    bool ValidateTechnicalSpecifications(const TechnicalSpecifications& specs) const;
    
    // Comprehensive validation
    bool ValidateCompleteTheme(const std::string& palette_name,
                               const std::string& charset_name,
                               const CompositionRules& composition,
                               const AnimationTiming& animation,
                               const TechnicalSpecifications& tech) const;
};

class ASCIIArtValidator {
public:
    // Validate ASCII art composition
    static bool ValidateComposition(const std::string& ascii_art,
                                  const CompositionRules& rules);
    
    // Check character density
    static float CalculateCharacterDensity(const std::string& ascii_art);
    static bool IsDensityAppropriate(float density, int target_level);
    
    // Validate color usage
    static bool ValidateColorUsage(const std::string& ascii_art,
                                 const ColorPalette& palette);
    
    // Check for visual consistency
    static bool CheckVisualConsistency(const std::vector<std::string>& frame_sequence);
    
    // Validate animation timing
    static bool ValidateAnimationTiming(const std::vector<std::chrono::milliseconds>& frame_times,
                                    const AnimationTiming& timing);
    
    // Comprehensive ASCII art validation
    static bool ValidateCompleteASCIIArt(const std::string& ascii_art,
                                       const VisualLanguageStandards& standards,
                                       const std::string& palette_name,
                                       const std::string& charset_name);
};

// Predefined standard palettes
namespace StandardPalettes {
    const ColorPalette CYBERPUNK {"#00ff00", "#ff0080", "#00ffff", "#0a0a0a", "#ffffff"};
    const ColorPalette VAPORWAVE {"#ff71ce", "#01cdf4", "#05ffa1", "#1a1a2e", "#f9f871"};
    const ColorPalette MATRIX {"#00ff41", "#008f11", "#003008", "#000000", "#7fff00"};
    const ColorPalette NOIR {"#ffffff", "#cccccc", "#999999", "#333333", "#000000"};
    const ColorPalette RETRO {"#ff6b6b", "#4ecdc4", "#45b7d1", "#2c3e50", "#f9ca24"};
}

// Predefined standard character sets
namespace StandardCharsets {
    const CharacterSet HIGH_DENSITY {
        "@%#*+=-:. ",
        "High Density",
        "Full range for detailed scenes",
        10
    };
    
    const CharacterSet MEDIUM_DENSITY {
        "@#*=-. ",
        "Medium Density",
        "Balanced detail and performance",
        7
    };
    
    const CharacterSet LOW_DENSITY {
        "@#-. ",
        "Low Density",
        "Minimalist approach",
        4
    };
    
    const CharacterSet BLOCKS {
        "█▉▊▋▌▍▎▏ ",
        "Unicode Blocks",
        "Solid block characters",
        9
    };
    
    const CharacterSet SHADES {
        "▓▒░ ",
        "Shade Characters",
        "Shading and gradients",
        4
    };
}

} // namespace Production
} // namespace NeonGlyph