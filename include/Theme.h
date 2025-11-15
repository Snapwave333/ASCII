#pragma once

#include "NeonGlyph.h"
#include <string>
#include <unordered_map>

namespace NeonGlyph {

struct ThemeConfig {
    std::string name;
    std::unordered_map<std::string, uint32> palette;
    float32 brightness = 0.0f;
    float32 contrast = 1.0f;
    std::string glyph_map;
};

class ASCIIConverter;

class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();

    Result Initialize(const Config& config);
    Result LoadThemeFile(const std::string& path);
    Result ApplyToASCII(ASCIIConverter* converter);
    Result AutoDetectSystemTheme();
    ThemeConfig GetTheme() const { return m_theme; }
    Result GenerateThemeFromLLM(const std::string& name, const std::string& hint);

private:
    Config m_config;
    ThemeConfig m_theme;
    Result ParseJSON(const std::string& content, ThemeConfig& out);
    uint32 HexToRGBA(const std::string& hex);
};

} // namespace NeonGlyph
