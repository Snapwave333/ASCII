#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <regex>
#include <functional>
#include <chrono>

namespace NeonGlyph {
namespace Production {

enum class AssetType {
    Theme,
    CharacterSet,
    ColorPalette,
    Animation,
    Audio,
    Configuration,
    Documentation,
    Script,
    Shader,
    Texture,
    Model,
    Prefab,
    Scene,
    Unknown
};

enum class NamingConvention {
    CamelCase,
    PascalCase,
    SnakeCase,
    KebabCase,
    UpperCase,
    LowerCase,
    MixedCase
};

struct NamingRule {
    std::string name;
    std::string pattern;
    std::string description;
    bool is_required;
    bool is_case_sensitive;
    std::vector<std::string> allowed_values;
    std::vector<std::string> forbidden_values;
    std::function<bool(const std::string&)> custom_validator;
};

struct TaxonomyCategory {
    std::string name;
    std::string description;
    std::vector<std::string> subcategories;
    std::vector<std::string> tags;
    std::map<std::string, std::string> metadata;
    bool is_active;
};

struct AssetClassification {
    AssetType type;
    std::string category;
    std::string subcategory;
    std::vector<std::string> tags;
    std::map<std::string, std::string> metadata;
    std::string naming_convention;
    std::string version;
    std::string author;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point modified_at;
};

struct ValidationResult {
    bool is_valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> suggestions;
    std::map<std::string, std::string> metadata;
};

class NamingConventions {
public:
    NamingConventions();
    ~NamingConventions();

    // Naming convention management
    void AddNamingRule(const NamingRule& rule);
    void RemoveNamingRule(const std::string& rule_name);
    void UpdateNamingRule(const std::string& rule_name, const NamingRule& rule);
    std::vector<NamingRule> GetNamingRules() const;
    std::vector<NamingRule> GetNamingRulesForType(AssetType type) const;
    
    // Taxonomy management
    void AddTaxonomyCategory(const TaxonomyCategory& category);
    void RemoveTaxonomyCategory(const std::string& category_name);
    void UpdateTaxonomyCategory(const std::string& category_name, const TaxonomyCategory& category);
    std::vector<TaxonomyCategory> GetTaxonomyCategories() const;
    std::vector<TaxonomyCategory> GetActiveTaxonomyCategories() const;
    
    // Asset classification
    AssetClassification ClassifyAsset(const std::string& asset_path) const;
    AssetClassification ClassifyAsset(const std::string& asset_path, AssetType type) const;
    std::string GenerateAssetName(const AssetClassification& classification) const;
    std::string GenerateAssetName(AssetType type, const std::string& category, 
                                 const std::string& subcategory = "", 
                                 const std::string& base_name = "") const;
    
    // Validation
    ValidationResult ValidateName(const std::string& name) const;
    ValidationResult ValidateName(const std::string& name, AssetType type) const;
    ValidationResult ValidateName(const std::string& name, const std::vector<NamingRule>& rules) const;
    
    // Name transformation
    std::string ConvertToConvention(const std::string& name, NamingConvention convention) const;
    std::string ConvertToConvention(const std::string& name, const std::string& convention_name) const;
    std::string SanitizeName(const std::string& name) const;
    std::string GenerateUniqueName(const std::string& base_name, 
                                  const std::vector<std::string>& existing_names) const;
    
    // Category and tag management
    void AddCategory(const std::string& category_name, const std::string& description = "");
    void RemoveCategory(const std::string& category_name);
    void AddSubcategory(const std::string& category_name, const std::string& subcategory_name);
    void RemoveSubcategory(const std::string& category_name, const std::string& subcategory_name);
    void AddTag(const std::string& tag_name, const std::string& category = "");
    void RemoveTag(const std::string& tag_name, const std::string& category = "");
    
    std::vector<std::string> GetCategories() const;
    std::vector<std::string> GetSubcategories(const std::string& category) const;
    std::vector<std::string> GetTags(const std::string& category = "") const;
    
    // Metadata management
    void SetMetadata(const std::string& key, const std::string& value, const std::string& category = "");
    std::string GetMetadata(const std::string& key, const std::string& category = "") const;
    std::map<std::string, std::string> GetAllMetadata(const std::string& category = "") const;
    
    // Preset management
    void LoadPreset(const std::string& preset_name);
    void SavePreset(const std::string& preset_name) const;
    std::vector<std::string> GetAvailablePresets() const;
    
    // Import/Export
    bool ImportConfiguration(const std::string& file_path);
    bool ExportConfiguration(const std::string& file_path) const;
    bool ImportTaxonomy(const std::string& file_path);
    bool ExportTaxonomy(const std::string& file_path) const;
    
    // Search and query
    std::vector<std::string> SearchAssets(const std::string& query, AssetType type = AssetType::Unknown) const;
    std::vector<std::string> SearchByCategory(const std::string& category) const;
    std::vector<std::string> SearchByTag(const std::string& tag) const;
    std::vector<std::string> SearchByMetadata(const std::string& key, const std::string& value) const;
    
    // Statistics and reporting
    struct NamingStatistics {
        size_t total_rules;
        size_t active_rules;
        size_t total_categories;
        size_t active_categories;
        size_t total_tags;
        size_t validated_names;
        size_t invalid_names;
        std::map<AssetType, size_t> asset_type_counts;
        std::map<std::string, size_t> category_counts;
        std::map<std::string, size_t> tag_counts;
    };
    
    NamingStatistics GetStatistics() const;
    std::string GenerateReport() const;
    std::vector<std::string> GetValidationHistory() const;
    
    // Configuration management
    void SetConfiguration(const std::string& key, const std::string& value);
    std::string GetConfiguration(const std::string& key) const;
    std::map<std::string, std::string> GetAllConfigurations() const;
    
    // Batch operations
    std::vector<ValidationResult> ValidateBatch(const std::vector<std::string>& names) const;
    std::vector<std::string> ConvertBatch(const std::vector<std::string>& names, NamingConvention convention) const;
    std::vector<AssetClassification> ClassifyBatch(const std::vector<std::string>& asset_paths) const;
    
    // Custom validation
    void AddCustomValidator(const std::string& name, std::function<bool(const std::string&)> validator);
    void RemoveCustomValidator(const std::string& name);
    std::vector<std::string> GetCustomValidators() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Utility functions
std::string ConvertCamelCaseToSnakeCase(const std::string& camel_case);
std::string ConvertSnakeCaseToCamelCase(const std::string& snake_case);
std::string ConvertCamelCaseToKebabCase(const std::string& camel_case);
std::string ConvertKebabCaseToCamelCase(const std::string& kebab_case);
std::string ConvertToPascalCase(const std::string& input);
std::string ConvertToUpperCase(const std::string& input);
std::string ConvertToLowerCase(const std::string& input);

bool IsValidCamelCase(const std::string& input);
bool IsValidSnakeCase(const std::string& input);
bool IsValidKebabCase(const std::string& input);
bool IsValidPascalCase(const std::string& input);

std::string ExtractBaseName(const std::string& asset_name);
std::string ExtractVersion(const std::string& asset_name);
std::string ExtractCategory(const std::string& asset_name);
std::string ExtractExtension(const std::string& asset_name);

std::vector<std::string> GenerateNameSuggestions(const std::string& base_name, AssetType type);
std::vector<std::string> GenerateCategorySuggestions(const std::string& asset_name);
std::vector<std::string> GenerateTagSuggestions(const std::string& asset_name);

} // namespace Production
} // namespace NeonGlyph