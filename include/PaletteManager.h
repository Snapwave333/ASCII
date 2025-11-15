#pragma once

#include "NeonGlyph.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>

namespace NeonGlyph {

struct PhaseRule {
    std::vector<std::string> preferred_role_tags;
    std::vector<std::string> avoid_role_tags;
    std::vector<std::string> avoid_moods;
    std::vector<std::string> fallback_role_tags;
    std::vector<std::string> bias_moods;
    std::vector<std::string> bias_energy;
};

struct RandomizationRules {
    std::unordered_map<std::string, PhaseRule> phase_rules;
    uint32 max_recent_history = 3;
};

struct SelectionInput {
    std::string phase;
    std::vector<std::string> mood;
    int32 tempo_bpm = 120;
    float32 energy_level_0_1 = 0.5f;
    std::string color_mode;
    bool is_dark_venue = false;
    bool is_test_sequence = false;
};

class PaletteManager {
public:
    Result LoadFromJson(const std::string& path);
    const std::vector<RichPalette>& GetPalettes() const { return m_palettes; }
    RichPalette SelectPalette(const SelectionInput& in);
    RichPalette GetByName(const std::string& name) const;
    void PushHistory(const std::string& phase, const std::string& name);

private:
    std::vector<RichPalette> m_palettes;
    RandomizationRules m_rules;
    std::unordered_map<std::string, std::deque<std::string>> m_recent;
    int Score(const RichPalette& p, const SelectionInput& in, const PhaseRule* pr) const;
    static uint32 ParseHex(const std::string& s);
};

}
