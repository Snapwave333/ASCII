#include "production/NamingConventions.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <set>
#include <random>

namespace NeonGlyph {
namespace Production {

// Utility functions
std::string ConvertCamelCaseToSnakeCase(const std::string& camel_case) {
    std::string result;
    for (size_t i = 0; i < camel_case.length(); ++i) {
        char c = camel_case[i];
        if (std::isupper(c)) {
            if (i > 0) {
                result += '_';
            }
            result += std::tolower(c);
        } else {
            result += c;
        }
    }
    return result;
}

std::string ConvertSnakeCaseToCamelCase(const std::string& snake_case) {
    std::string result;
    bool capitalize_next = false;
    for (char c : snake_case) {
        if (c == '_') {
            capitalize_next = true;
        } else if (capitalize_next) {
            result += std::toupper(c);
            capitalize_next = false;
        } else {
            result += std::tolower(c);
        }
    }
    return result;
}

std::string ConvertCamelCaseToKebabCase(const std::string& camel_case) {
    std::string result;
    for (size_t i = 0; i < camel_case.length(); ++i) {
        char c = camel_case[i];
        if (std::isupper(c)) {
            if (i > 0) {
                result += '-';
            }
            result += std::tolower(c);
        } else {
            result += c;
        }
    }
    return result;
}

std::string ConvertKebabCaseToCamelCase(const std::string& kebab_case) {
    std::string result;
    bool capitalize_next = false;
    for (char c : kebab_case) {
        if (c == '-') {
            capitalize_next = true;
        } else if (capitalize_next) {
            result += std::toupper(c);
            capitalize_next = false;
        } else {
            result += std::tolower(c);
        }
    }
    return result;
}

std::string ConvertToPascalCase(const std::string& input) {
    std::string result = ConvertKebabCaseToCamelCase(input);
    if (!result.empty()) {
        result[0] = std::toupper(result[0]);
    }
    return result;
}

std::string ConvertToUpperCase(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string ConvertToLowerCase(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool IsValidCamelCase(const std::string& input) {
    if (input.empty()) return false;
    return std::regex_match(input, std::regex("^[a-z][a-zA-Z0-9]*$"));
}

bool IsValidSnakeCase(const std::string& input) {
    if (input.empty()) return false;
    return std::regex_match(input, std::regex("^[a-z][a-z0-9_]*$"));
}

bool IsValidKebabCase(const std::string& input) {
    if (input.empty()) return false;
    return std::regex_match(input, std::regex("^[a-z][a-z0-9\-]*$"));
}

bool IsValidPascalCase(const std::string& input) {
    if (input.empty()) return false;
    return std::regex_match(input, std::regex("^[A-Z][a-zA-Z0-9]*$"));
}

std::string ExtractBaseName(const std::string& asset_name) {
    std::filesystem::path path(asset_name);
    std::string filename = path.stem().string();
    
    // Remove version numbers and other suffixes
    std::regex version_regex(R"(_v\d+(\.\d+)*$)");
    filename = std::regex_replace(filename, version_regex, "");
    
    return filename;
}

std::string ExtractVersion(const std::string& asset_name) {
    std::regex version_regex(R"(_v(\d+(\.\d+)*))");
    std::smatch match;
    if (std::regex_search(asset_name, match, version_regex)) {
        return match[1].str();
    }
    return "";
}

std::string ExtractCategory(const std::string& asset_name) {
    std::filesystem::path path(asset_name);
    std::vector<std::string> parts;
    
    for (const auto& part : path) {
        parts.push_back(part.string());
    }
    
    if (parts.size() >= 2) {
        return parts[parts.size() - 2];
    }
    
    return "";
}

std::string ExtractExtension(const std::string& asset_name) {
    std::filesystem::path path(asset_name);
    return path.extension().string();
}

std::vector<std::string> GenerateNameSuggestions(const std::string& base_name, AssetType type) {
    std::vector<std::string> suggestions;
    
    suggestions.push_back(ConvertToPascalCase(base_name));
    suggestions.push_back(ConvertCamelCaseToSnakeCase(base_name));
    suggestions.push_back(ConvertCamelCaseToKebabCase(base_name));
    suggestions.push_back(ConvertToUpperCase(base_name));
    suggestions.push_back(ConvertToLowerCase(base_name));
    
    // Add type-specific suffixes
    switch (type) {
        case AssetType::Theme:
            suggestions.push_back(base_name + "Theme");
            suggestions.push_back(base_name + "_theme");
            break;
        case AssetType::CharacterSet:
            suggestions.push_back(base_name + "Chars");
            suggestions.push_back(base_name + "_chars");
            break;
        case AssetType::ColorPalette:
            suggestions.push_back(base_name + "Palette");
            suggestions.push_back(base_name + "_palette");
            break;
        case AssetType::Animation:
            suggestions.push_back(base_name + "Anim");
            suggestions.push_back(base_name + "_anim");
            break;
        case AssetType::Audio:
            suggestions.push_back(base_name + "Audio");
            suggestions.push_back(base_name + "_audio");
            break;
        default:
            suggestions.push_back(base_name + "Asset");
            suggestions.push_back(base_name + "_asset");
            break;
    }
    
    return suggestions;
}

std::vector<std::string> GenerateCategorySuggestions(const std::string& asset_name) {
    std::vector<std::string> suggestions;
    
    // Extract keywords from asset name
    std::string base_name = ExtractBaseName(asset_name);
    std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::tolower);
    
    // Common VJ/ASCII art categories
    std::vector<std::string> common_categories = {
        "themes", "characters", "colors", "animations", "audio", 
        "effects", "transitions", "generators", "processors", "utilities"
    };
    
    for (const auto& category : common_categories) {
        if (base_name.find(category) != std::string::npos) {
            suggestions.push_back(category);
        }
    }
    
    // Style-based categories
    if (base_name.find("cyber") != std::string::npos || 
        base_name.find("neon") != std::string::npos ||
        base_name.find("tech") != std::string::npos) {
        suggestions.push_back("cyberpunk");
    }
    
    if (base_name.find("retro") != std::string::npos || 
        base_name.find("vintage") != std::string::npos ||
        base_name.find("8bit") != std::string::npos) {
        suggestions.push_back("retro");
    }
    
    if (base_name.find("minimal") != std::string::npos || 
        base_name.find("clean") != std::string::npos ||
        base_name.find("simple") != std::string::npos) {
        suggestions.push_back("minimalist");
    }
    
    if (suggestions.empty()) {
        suggestions.push_back("general");
    }
    
    return suggestions;
}

std::vector<std::string> GenerateTagSuggestions(const std::string& asset_name) {
    std::vector<std::string> suggestions;
    
    std::string base_name = ExtractBaseName(asset_name);
    std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::tolower);
    
    // Common VJ/ASCII art tags
    std::vector<std::pair<std::string, std::vector<std::string>>> tag_mappings = {
        {"fast", {"high-energy", "rapid", "quick"}},
        {"slow", {"calm", "smooth", "gradual"}},
        {"bright", {"vivid", "intense", "luminous"}},
        {"dark", {"dim", "shadow", "noir"}},
        {"color", {"chromatic", "hue", "spectrum"}},
        {"mono", {"monochrome", "grayscale", "single-color"}},
        {"complex", {"intricate", "detailed", "elaborate"}},
        {"simple", {"basic", "minimal", "clean"}},
        {"abstract", {"non-representational", "geometric", "pattern"}},
        {"text", {"typography", "font", "character-based"}}
    };
    
    for (const auto& mapping : tag_mappings) {
        if (base_name.find(mapping.first) != std::string::npos) {
            suggestions.insert(suggestions.end(), mapping.second.begin(), mapping.second.end());
        }
    }
    
    // Add generic tags based on length and complexity
    if (base_name.length() < 8) {
        suggestions.push_back("short-name");
    } else if (base_name.length() > 20) {
        suggestions.push_back("long-name");
    }
    
    if (suggestions.empty()) {
        suggestions.push_back("general");
    }
    
    return suggestions;
}

// Implementation class
class NamingConventions::Impl {
public:
    Impl() {
        InitializeDefaultRules();
        InitializeDefaultTaxonomy();
    }

    void AddNamingRule(const NamingRule& rule) {
        naming_rules_[rule.name] = rule;
    }

    void RemoveNamingRule(const std::string& rule_name) {
        naming_rules_.erase(rule_name);
    }

    void UpdateNamingRule(const std::string& rule_name, const NamingRule& rule) {
        naming_rules_[rule_name] = rule;
    }

    std::vector<NamingRule> GetNamingRules() const {
        std::vector<NamingRule> rules;
        for (const auto& pair : naming_rules_) {
            rules.push_back(pair.second);
        }
        return rules;
    }

    std::vector<NamingRule> GetNamingRulesForType(AssetType type) const {
        std::vector<NamingRule> type_rules;
        std::string type_str = AssetTypeToString(type);
        
        for (const auto& pair : naming_rules_) {
            if (pair.second.name.find(type_str) != std::string::npos) {
                type_rules.push_back(pair.second);
            }
        }
        
        return type_rules;
    }

    void AddTaxonomyCategory(const TaxonomyCategory& category) {
        taxonomy_categories_[category.name] = category;
    }

    void RemoveTaxonomyCategory(const std::string& category_name) {
        taxonomy_categories_.erase(category_name);
    }

    void UpdateTaxonomyCategory(const std::string& category_name, const TaxonomyCategory& category) {
        taxonomy_categories_[category_name] = category;
    }

    std::vector<TaxonomyCategory> GetTaxonomyCategories() const {
        std::vector<TaxonomyCategory> categories;
        for (const auto& pair : taxonomy_categories_) {
            categories.push_back(pair.second);
        }
        return categories;
    }

    std::vector<TaxonomyCategory> GetActiveTaxonomyCategories() const {
        std::vector<TaxonomyCategory> active_categories;
        for (const auto& pair : taxonomy_categories_) {
            if (pair.second.is_active) {
                active_categories.push_back(pair.second);
            }
        }
        return active_categories;
    }

    AssetClassification ClassifyAsset(const std::string& asset_path) const {
        AssetClassification classification;
        classification.created_at = std::chrono::system_clock::now();
        classification.modified_at = std::chrono::system_clock::now();
        
        // Determine asset type from extension and path
        classification.type = DetermineAssetType(asset_path);
        
        // Extract category from path
        classification.category = ExtractCategory(asset_path);
        
        // Generate tags based on name and path
        std::vector<std::string> suggestions = GenerateTagSuggestions(asset_path);
        classification.tags = suggestions;
        
        // Determine naming convention
        classification.naming_convention = DetermineNamingConvention(asset_path);
        
        return classification;
    }

    AssetClassification ClassifyAsset(const std::string& asset_path, AssetType type) const {
        AssetClassification classification = ClassifyAsset(asset_path);
        classification.type = type;
        return classification;
    }

    std::string GenerateAssetName(const AssetClassification& classification) const {
        return GenerateAssetName(classification.type, classification.category, 
                               classification.subcategory, "");
    }

    std::string GenerateAssetName(AssetType type, const std::string& category, 
                                 const std::string& subcategory, const std::string& base_name) const {
        std::stringstream name_stream;
        
        // Add category
        if (!category.empty()) {
            name_stream << ConvertToPascalCase(category);
        }
        
        // Add subcategory
        if (!subcategory.empty()) {
            if (!category.empty()) {
                name_stream << "_";
            }
            name_stream << ConvertToPascalCase(subcategory);
        }
        
        // Add base name or type-specific suffix
        if (!base_name.empty()) {
            if (!category.empty() || !subcategory.empty()) {
                name_stream << "_";
            }
            name_stream << ConvertToPascalCase(base_name);
        } else {
            if (!category.empty() || !subcategory.empty()) {
                name_stream << "_";
            }
            name_stream << AssetTypeToString(type);
        }
        
        return name_stream.str();
    }

    ValidationResult ValidateName(const std::string& name) const {
        ValidationResult result;
        result.is_valid = true;
        
        // Check against all naming rules
        for (const auto& pair : naming_rules_) {
            const NamingRule& rule = pair.second;
            if (!ValidateAgainstRule(name, rule)) {
                result.is_valid = false;
                result.errors.push_back("Failed rule: " + rule.name + " - " + rule.description);
            }
        }
        
        return result;
    }

    ValidationResult ValidateName(const std::string& name, AssetType type) const {
        ValidationResult result;
        result.is_valid = true;
        
        // Get rules specific to this asset type
        std::vector<NamingRule> type_rules = GetNamingRulesForType(type);
        
        for (const NamingRule& rule : type_rules) {
            if (!ValidateAgainstRule(name, rule)) {
                result.is_valid = false;
                result.errors.push_back("Failed rule: " + rule.name + " - " + rule.description);
            }
        }
        
        return result;
    }

    ValidationResult ValidateName(const std::string& name, const std::vector<NamingRule>& rules) const {
        ValidationResult result;
        result.is_valid = true;
        
        for (const NamingRule& rule : rules) {
            if (!ValidateAgainstRule(name, rule)) {
                result.is_valid = false;
                result.errors.push_back("Failed rule: " + rule.name + " - " + rule.description);
            }
        }
        
        return result;
    }

    std::string ConvertToConvention(const std::string& name, NamingConvention convention) const {
        switch (convention) {
            case NamingConvention::CamelCase:
                return ConvertSnakeCaseToCamelCase(name);
            case NamingConvention::PascalCase:
                return ConvertToPascalCase(name);
            case NamingConvention::SnakeCase:
                return ConvertCamelCaseToSnakeCase(name);
            case NamingConvention::KebabCase:
                return ConvertCamelCaseToKebabCase(name);
            case NamingConvention::UpperCase:
                return ConvertToUpperCase(name);
            case NamingConvention::LowerCase:
                return ConvertToLowerCase(name);
            default:
                return name;
        }
    }

    std::string ConvertToConvention(const std::string& name, const std::string& convention_name) const {
        if (convention_name == "camelCase") {
            return ConvertToConvention(name, NamingConvention::CamelCase);
        } else if (convention_name == "PascalCase") {
            return ConvertToConvention(name, NamingConvention::PascalCase);
        } else if (convention_name == "snake_case") {
            return ConvertToConvention(name, NamingConvention::SnakeCase);
        } else if (convention_name == "kebab-case") {
            return ConvertToConvention(name, NamingConvention::KebabCase);
        } else if (convention_name == "UPPER_CASE") {
            return ConvertToConvention(name, NamingConvention::UpperCase);
        } else if (convention_name == "lower_case") {
            return ConvertToConvention(name, NamingConvention::LowerCase);
        }
        return name;
    }

    std::string SanitizeName(const std::string& name) const {
        std::string sanitized = name;
        
        // Remove invalid characters
        sanitized = std::regex_replace(sanitized, std::regex(R"([^a-zA-Z0-9_\-\s])"), "");
        
        // Replace spaces with underscores
        std::replace(sanitized.begin(), sanitized.end(), ' ', '_');
        
        // Remove multiple consecutive underscores
        sanitized = std::regex_replace(sanitized, std::regex(R"(_{2,})"), "_");
        
        // Remove leading/trailing underscores
        sanitized = std::regex_replace(sanitized, std::regex(R"(^_+|_+$)"), "");
        
        return sanitized;
    }

    std::string GenerateUniqueName(const std::string& base_name, 
                                 const std::vector<std::string>& existing_names) const {
        std::string unique_name = base_name;
        int counter = 1;
        
        while (std::find(existing_names.begin(), existing_names.end(), unique_name) != existing_names.end()) {
            unique_name = base_name + "_" + std::to_string(counter);
            counter++;
        }
        
        return unique_name;
    }

    void AddCategory(const std::string& category_name, const std::string& description) {
        TaxonomyCategory category;
        category.name = category_name;
        category.description = description;
        category.is_active = true;
        
        taxonomy_categories_[category_name] = category;
    }

    void RemoveCategory(const std::string& category_name) {
        taxonomy_categories_.erase(category_name);
    }

    void AddSubcategory(const std::string& category_name, const std::string& subcategory_name) {
        auto it = taxonomy_categories_.find(category_name);
        if (it != taxonomy_categories_.end()) {
            it->second.subcategories.push_back(subcategory_name);
        }
    }

    void RemoveSubcategory(const std::string& category_name, const std::string& subcategory_name) {
        auto it = taxonomy_categories_.find(category_name);
        if (it != taxonomy_categories_.end()) {
            auto& subcategories = it->second.subcategories;
            subcategories.erase(std::remove(subcategories.begin(), subcategories.end(), subcategory_name), subcategories.end());
        }
    }

    void AddTag(const std::string& tag_name, const std::string& category) {
        if (category.empty()) {
            global_tags_.insert(tag_name);
        } else {
            auto it = taxonomy_categories_.find(category);
            if (it != taxonomy_categories_.end()) {
                it->second.tags.push_back(tag_name);
            }
        }
    }

    void RemoveTag(const std::string& tag_name, const std::string& category) {
        if (category.empty()) {
            global_tags_.erase(tag_name);
        } else {
            auto it = taxonomy_categories_.find(category);
            if (it != taxonomy_categories_.end()) {
                auto& tags = it->second.tags;
                tags.erase(std::remove(tags.begin(), tags.end(), tag_name), tags.end());
            }
        }
    }

    std::vector<std::string> GetCategories() const {
        std::vector<std::string> categories;
        for (const auto& pair : taxonomy_categories_) {
            categories.push_back(pair.first);
        }
        return categories;
    }

    std::vector<std::string> GetSubcategories(const std::string& category) const {
        auto it = taxonomy_categories_.find(category);
        if (it != taxonomy_categories_.end()) {
            return it->second.subcategories;
        }
        return std::vector<std::string>();
    }

    std::vector<std::string> GetTags(const std::string& category) const {
        if (category.empty()) {
            return std::vector<std::string>(global_tags_.begin(), global_tags_.end());
        } else {
            auto it = taxonomy_categories_.find(category);
            if (it != taxonomy_categories_.end()) {
                return it->second.tags;
            }
        }
        return std::vector<std::string>();
    }

    NamingStatistics GetStatistics() const {
        NamingStatistics stats{};
        stats.total_rules = naming_rules_.size();
        stats.active_rules = std::count_if(naming_rules_.begin(), naming_rules_.end(), 
            [](const auto& pair) { return !pair.second.forbidden_values.empty() || !pair.second.allowed_values.empty(); });
        stats.total_categories = taxonomy_categories_.size();
        stats.active_categories = std::count_if(taxonomy_categories_.begin(), taxonomy_categories_.end(),
            [](const auto& pair) { return pair.second.is_active; });
        stats.total_tags = global_tags_.size();
        
        for (const auto& pair : taxonomy_categories_) {
            stats.total_tags += pair.second.tags.size();
        }
        
        return stats;
    }

private:
    void InitializeDefaultRules() {
        // Theme naming rules
        NamingRule theme_rule;
        theme_rule.name = "ThemeNaming";
        theme_rule.pattern = R"(^[A-Z][a-zA-Z0-9]*Theme$)";
        theme_rule.description = "Theme names should start with uppercase and end with 'Theme'";
        theme_rule.is_required = true;
        theme_rule.is_case_sensitive = true;
        naming_rules_["ThemeNaming"] = theme_rule;
        
        // Character set naming rules
        NamingRule charset_rule;
        charset_rule.name = "CharacterSetNaming";
        charset_rule.pattern = R"(^[a-z][a-z0-9_]*_chars$)";
        charset_rule.description = "Character sets should use snake_case and end with '_chars'";
        charset_rule.is_required = true;
        charset_rule.is_case_sensitive = true;
        naming_rules_["CharacterSetNaming"] = charset_rule;
        
        // Color palette naming rules
        NamingRule palette_rule;
        palette_rule.name = "ColorPaletteNaming";
        palette_rule.pattern = R"(^[A-Z][a-zA-Z0-9]*Palette$)";
        palette_rule.description = "Color palettes should use PascalCase and end with 'Palette'";
        palette_rule.is_required = true;
        palette_rule.is_case_sensitive = true;
        naming_rules_["ColorPaletteNaming"] = palette_rule;
        
        // Version naming rules
        NamingRule version_rule;
        version_rule.name = "VersionNaming";
        version_rule.pattern = R"(_v\d+(\.\d+)*)";
        version_rule.description = "Versions should follow semantic versioning format";
        version_rule.is_required = false;
        version_rule.is_case_sensitive = true;
        naming_rules_["VersionNaming"] = version_rule;
    }

    void InitializeDefaultTaxonomy() {
        // VJ/ASCII Art specific categories
        std::vector<std::pair<std::string, std::vector<std::string>>> categories = {
            {"themes", {"cyberpunk", "retro", "minimal", "abstract", "nature", "urban"}},
            {"characters", {"ascii", "unicode", "blocks", "symbols", "custom"}},
            {"colors", {"palettes", "gradients", "themes", "cycles"}},
            {"animations", {"transitions", "effects", "generators", "loops"}},
            {"audio", {"reactive", "ambient", "beats", "synthesis"}},
            {"effects", {"distortion", "blur", "glow", "pixelation", "noise"}},
            {"utilities", {"tools", "converters", "validators", "generators"}}
        };
        
        for (const auto& category_pair : categories) {
            TaxonomyCategory category;
            category.name = category_pair.first;
            category.description = "Category for " + category_pair.first;
            category.subcategories = category_pair.second;
            category.is_active = true;
            
            taxonomy_categories_[category_pair.first] = category;
        }
    }

    AssetType DetermineAssetType(const std::string& asset_path) const {
        std::filesystem::path path(asset_path);
        std::string extension = path.extension().string();
        std::string filename = path.filename().string();
        
        if (filename.find("theme") != std::string::npos) {
            return AssetType::Theme;
        } else if (filename.find("char") != std::string::npos) {
            return AssetType::CharacterSet;
        } else if (filename.find("palette") != std::string::npos) {
            return AssetType::ColorPalette;
        } else if (filename.find("anim") != std::string::npos) {
            return AssetType::Animation;
        } else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
            return AssetType::Audio;
        } else if (extension == ".json" || extension == ".xml") {
            return AssetType::Configuration;
        } else if (extension == ".md" || extension == ".txt") {
            return AssetType::Documentation;
        } else if (extension == ".lua" || extension == ".py") {
            return AssetType::Script;
        } else {
            return AssetType::Unknown;
        }
    }

    std::string AssetTypeToString(AssetType type) const {
        switch (type) {
            case AssetType::Theme: return "Theme";
            case AssetType::CharacterSet: return "CharacterSet";
            case AssetType::ColorPalette: return "ColorPalette";
            case AssetType::Animation: return "Animation";
            case AssetType::Audio: return "Audio";
            case AssetType::Configuration: return "Configuration";
            case AssetType::Documentation: return "Documentation";
            case AssetType::Script: return "Script";
            case AssetType::Shader: return "Shader";
            case AssetType::Texture: return "Texture";
            case AssetType::Model: return "Model";
            case AssetType::Prefab: return "Prefab";
            case AssetType::Scene: return "Scene";
            default: return "Unknown";
        }
    }

    std::string DetermineNamingConvention(const std::string& asset_path) const {
        std::string filename = std::filesystem::path(asset_path).stem().string();
        
        if (IsValidPascalCase(filename)) {
            return "PascalCase";
        } else if (IsValidCamelCase(filename)) {
            return "camelCase";
        } else if (IsValidSnakeCase(filename)) {
            return "snake_case";
        } else if (IsValidKebabCase(filename)) {
            return "kebab-case";
        } else if (std::all_of(filename.begin(), filename.end(), ::isupper)) {
            return "UPPER_CASE";
        } else if (std::all_of(filename.begin(), filename.end(), ::islower)) {
            return "lower_case";
        } else {
            return "mixed";
        }
    }

    bool ValidateAgainstRule(const std::string& name, const NamingRule& rule) const {
        if (!rule.pattern.empty()) {
            std::regex pattern(rule.pattern);
            if (!std::regex_match(name, pattern)) {
                return false;
            }
        }
        
        if (!rule.allowed_values.empty()) {
            bool found = false;
            for (const auto& allowed : rule.allowed_values) {
                if (rule.is_case_sensitive) {
                    if (name.find(allowed) != std::string::npos) {
                        found = true;
                        break;
                    }
                } else {
                    std::string lower_name = ConvertToLowerCase(name);
                    std::string lower_allowed = ConvertToLowerCase(allowed);
                    if (lower_name.find(lower_allowed) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                return false;
            }
        }
        
        if (!rule.forbidden_values.empty()) {
            for (const auto& forbidden : rule.forbidden_values) {
                if (rule.is_case_sensitive) {
                    if (name.find(forbidden) != std::string::npos) {
                        return false;
                    }
                } else {
                    std::string lower_name = ConvertToLowerCase(name);
                    std::string lower_forbidden = ConvertToLowerCase(forbidden);
                    if (lower_name.find(lower_forbidden) != std::string::npos) {
                        return false;
                    }
                }
            }
        }
        
        if (rule.custom_validator) {
            return rule.custom_validator(name);
        }
        
        return true;
    }

    std::map<std::string, NamingRule> naming_rules_;
    std::map<std::string, TaxonomyCategory> taxonomy_categories_;
    std::set<std::string> global_tags_;
    std::map<std::string, std::string> configuration_;
    std::map<std::string, std::function<bool(const std::string&)>> custom_validators_;
    std::vector<std::string> validation_history_;
};

// NamingConventions implementation
NamingConventions::NamingConventions() : impl_(std::make_unique<Impl>()) {}
NamingConventions::~NamingConventions() = default;

void NamingConventions::AddNamingRule(const NamingRule& rule) {
    impl_->AddNamingRule(rule);
}

void NamingConventions::RemoveNamingRule(const std::string& rule_name) {
    impl_->RemoveNamingRule(rule_name);
}

void NamingConventions::UpdateNamingRule(const std::string& rule_name, const NamingRule& rule) {
    impl_->UpdateNamingRule(rule_name, rule);
}

std::vector<NamingRule> NamingConventions::GetNamingRules() const {
    return impl_->GetNamingRules();
}

std::vector<NamingRule> NamingConventions::GetNamingRulesForType(AssetType type) const {
    return impl_->GetNamingRulesForType(type);
}

void NamingConventions::AddTaxonomyCategory(const TaxonomyCategory& category) {
    impl_->AddTaxonomyCategory(category);
}

void NamingConventions::RemoveTaxonomyCategory(const std::string& category_name) {
    impl_->RemoveTaxonomyCategory(category_name);
}

void NamingConventions::UpdateTaxonomyCategory(const std::string& category_name, const TaxonomyCategory& category) {
    impl_->UpdateTaxonomyCategory(category_name, category);
}

std::vector<TaxonomyCategory> NamingConventions::GetTaxonomyCategories() const {
    return impl_->GetTaxonomyCategories();
}

std::vector<TaxonomyCategory> NamingConventions::GetActiveTaxonomyCategories() const {
    return impl_->GetActiveTaxonomyCategories();
}

AssetClassification NamingConventions::ClassifyAsset(const std::string& asset_path) const {
    return impl_->ClassifyAsset(asset_path);
}

AssetClassification NamingConventions::ClassifyAsset(const std::string& asset_path, AssetType type) const {
    return impl_->ClassifyAsset(asset_path, type);
}

std::string NamingConventions::GenerateAssetName(const AssetClassification& classification) const {
    return impl_->GenerateAssetName(classification);
}

std::string NamingConventions::GenerateAssetName(AssetType type, const std::string& category, 
                                               const std::string& subcategory, const std::string& base_name) const {
    return impl_->GenerateAssetName(type, category, subcategory, base_name);
}

ValidationResult NamingConventions::ValidateName(const std::string& name) const {
    return impl_->ValidateName(name);
}

ValidationResult NamingConventions::ValidateName(const std::string& name, AssetType type) const {
    return impl_->ValidateName(name, type);
}

ValidationResult NamingConventions::ValidateName(const std::string& name, const std::vector<NamingRule>& rules) const {
    return impl_->ValidateName(name, rules);
}

std::string NamingConventions::ConvertToConvention(const std::string& name, NamingConvention convention) const {
    return impl_->ConvertToConvention(name, convention);
}

std::string NamingConventions::ConvertToConvention(const std::string& name, const std::string& convention_name) const {
    return impl_->ConvertToConvention(name, convention_name);
}

std::string NamingConventions::SanitizeName(const std::string& name) const {
    return impl_->SanitizeName(name);
}

std::string NamingConventions::GenerateUniqueName(const std::string& base_name, 
                                                  const std::vector<std::string>& existing_names) const {
    return impl_->GenerateUniqueName(base_name, existing_names);
}

void NamingConventions::AddCategory(const std::string& category_name, const std::string& description) {
    impl_->AddCategory(category_name, description);
}

void NamingConventions::RemoveCategory(const std::string& category_name) {
    impl_->RemoveCategory(category_name);
}

void NamingConventions::AddSubcategory(const std::string& category_name, const std::string& subcategory_name) {
    impl_->AddSubcategory(category_name, subcategory_name);
}

void NamingConventions::RemoveSubcategory(const std::string& category_name, const std::string& subcategory_name) {
    impl_->RemoveSubcategory(category_name, subcategory_name);
}

void NamingConventions::AddTag(const std::string& tag_name, const std::string& category) {
    impl_->AddTag(tag_name, category);
}

void NamingConventions::RemoveTag(const std::string& tag_name, const std::string& category) {
    impl_->RemoveTag(tag_name, category);
}

std::vector<std::string> NamingConventions::GetCategories() const {
    return impl_->GetCategories();
}

std::vector<std::string> NamingConventions::GetSubcategories(const std::string& category) const {
    return impl_->GetSubcategories(category);
}

std::vector<std::string> NamingConventions::GetTags(const std::string& category) const {
    return impl_->GetTags(category);
}

NamingConventions::NamingStatistics NamingConventions::GetStatistics() const {
    return impl_->GetStatistics();
}

} // namespace Production
} // namespace NeonGlyph