#include "PaletteManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>

namespace NeonGlyph {

static std::string Lower(const std::string& s){ std::string r=s; for(char& c:r) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return r; }

uint32 PaletteManager::ParseHex(const std::string& s){ std::string t=s; if(!t.empty() && t[0]=='#') t=t.substr(1); if(t.size()==6) t+="FF"; try{ return static_cast<uint32>(std::stoul(t, nullptr, 16)); } catch(...) { return 0xFFFFFFFFu; } }

Result PaletteManager::LoadFromJson(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return Result::FileNotFound;
    nlohmann::json j;
    try { f >> j; } catch (...) { return Result::InvalidArgument; }
    m_palettes.clear();
    if (j.contains("palettes") && j["palettes"].is_array()) {
        for (auto& pj : j["palettes"]) {
            RichPalette rp;
            rp.name = pj.value("name", "");
            for (auto& x : pj.value("role_tags", std::vector<std::string>{})) rp.role_tags.push_back(x);
            for (auto& x : pj.value("mood_tags", std::vector<std::string>{})) rp.mood_tags.push_back(x);
            for (auto& x : pj.value("scene_tags", std::vector<std::string>{})) rp.scene_tags.push_back(x);
            rp.temperature = pj.value("temperature", "neutral");
            rp.energy = pj.value("energy", "medium");
            rp.brightness = pj.value("brightness", "medium");
            rp.primary = ParseHex(pj.value("primary", "#FFFFFFFF"));
            rp.secondary = ParseHex(pj.value("secondary", "#FFFFFFFF"));
            rp.secondary2 = ParseHex(pj.value("secondary2", "#FFFFFFFF"));
            rp.accent = ParseHex(pj.value("accent", "#FFFFFFFF"));
            rp.shadow = ParseHex(pj.value("shadow", "#202020FF"));
            rp.highlight = ParseHex(pj.value("highlight", "#FFFFFFFF"));
            if (!rp.name.empty()) m_palettes.push_back(rp);
        }
    }
    m_rules.phase_rules.clear();
    if (j.contains("randomization_rules") && j["randomization_rules"].contains("phase_rules")) {
        auto prj = j["randomization_rules"]["phase_rules"];
        for (auto it = prj.begin(); it != prj.end(); ++it) {
            PhaseRule pr;
            auto v = it.value();
            if (v.contains("preferred_role_tags")) for (auto& x : v["preferred_role_tags"]) pr.preferred_role_tags.push_back(x.get<std::string>());
            if (v.contains("avoid_role_tags")) for (auto& x : v["avoid_role_tags"]) pr.avoid_role_tags.push_back(x.get<std::string>());
            if (v.contains("avoid_moods")) for (auto& x : v["avoid_moods"]) pr.avoid_moods.push_back(x.get<std::string>());
            if (v.contains("fallback_role_tags")) for (auto& x : v["fallback_role_tags"]) pr.fallback_role_tags.push_back(x.get<std::string>());
            if (v.contains("bias_moods")) for (auto& x : v["bias_moods"]) pr.bias_moods.push_back(x.get<std::string>());
            if (v.contains("bias_energy")) for (auto& x : v["bias_energy"]) pr.bias_energy.push_back(x.get<std::string>());
            m_rules.phase_rules[it.key()] = pr;
        }
        if (j["randomization_rules"].contains("non_repetition") && j["randomization_rules"]["non_repetition"].contains("max_recent_history")) {
            m_rules.max_recent_history = j["randomization_rules"]["non_repetition"]["max_recent_history"].get<uint32>();
        }
    }
    return Result::Success;
}

RichPalette PaletteManager::GetByName(const std::string& name) const {
    for (auto& p : m_palettes) if (p.name == name) return p;
    return RichPalette{};
}

int PaletteManager::Score(const RichPalette& p, const SelectionInput& in, const PhaseRule* pr) const {
    int s = 0;
    if (pr) {
        for (auto& a : pr->avoid_role_tags) for (auto& r : p.role_tags) if (Lower(a)==Lower(r)) return -1000;
        for (auto& a : pr->avoid_moods) for (auto& r : p.mood_tags) if (Lower(a)==Lower(r)) return -1000;
    }
    for (auto& m : in.mood) for (auto& r : p.mood_tags) if (Lower(m)==Lower(r)) s += 2;
    if (pr) for (auto& br : pr->preferred_role_tags) for (auto& r : p.role_tags) if (Lower(br)==Lower(r)) s += 1;
    auto mapEnergy = [&](float e){ if (e < 0.25f) return std::string("low"); if (e < 0.5f) return std::string("medium"); if (e < 0.8f) return std::string("high"); return std::string("extreme"); };
    std::string ebin = mapEnergy(in.energy_level_0_1);
    if (Lower(ebin)==Lower(p.energy)) s += 2;
    bool preferBright = in.is_dark_venue && in.color_mode == "truecolor";
    if (preferBright && Lower(p.brightness)=="high") s += 1;
    if (!preferBright && (Lower(p.brightness)=="medium" || Lower(p.brightness)=="low")) s += 1;
    int bpm = in.tempo_bpm;
    if (bpm > 150 && (Lower(p.energy)=="high" || Lower(p.energy)=="extreme")) s += 2;
    if (bpm < 90 && (Lower(p.energy)=="low" || Lower(p.energy)=="medium")) s += 1;
    return s;
}

RichPalette PaletteManager::SelectPalette(const SelectionInput& in) {
    const PhaseRule* pr = nullptr;
    auto it = m_rules.phase_rules.find(in.phase);
    if (it != m_rules.phase_rules.end()) pr = &it->second;
    std::vector<const RichPalette*> candidates;
    for (auto& p : m_palettes) {
        bool roleOk = true;
        if (pr && !pr->preferred_role_tags.empty()) {
            roleOk = false;
            for (auto& rr : pr->preferred_role_tags) {
                for (auto& r : p.role_tags) if (Lower(rr)==Lower(r)) { roleOk = true; break; }
                if (roleOk) break;
            }
        }
        if (!roleOk && pr && !pr->fallback_role_tags.empty()) {
            for (auto& rr : pr->fallback_role_tags) {
                for (auto& r : p.role_tags) if (Lower(rr)==Lower(r)) { roleOk = true; break; }
                if (roleOk) break;
            }
        }
        if (!roleOk) continue;
        bool avoid = false;
        if (pr) {
            for (auto& a : pr->avoid_role_tags) for (auto& r : p.role_tags) if (Lower(a)==Lower(r)) { avoid = true; break; }
            for (auto& a : pr->avoid_moods) for (auto& r : p.mood_tags) if (Lower(a)==Lower(r)) { avoid = true; break; }
        }
        if (avoid) continue;
        candidates.push_back(&p);
    }
    auto histIt = m_recent.find(in.phase);
    std::vector<const RichPalette*> filtered;
    if (histIt != m_recent.end()) {
        for (auto* p : candidates) {
            bool recent = false;
            for (auto& n : histIt->second) if (n == p->name) { recent = true; break; }
            if (!recent) filtered.push_back(p);
        }
        if (filtered.empty()) filtered = candidates;
    } else {
        filtered = candidates;
    }
    int bestScore = -100000;
    const RichPalette* best = filtered.empty() ? nullptr : filtered[0];
    for (auto* p : filtered) {
        int sc = Score(*p, in, pr);
        if (sc > bestScore) { bestScore = sc; best = p; }
    }
    if (!best && !m_palettes.empty()) best = &m_palettes[0];
    RichPalette out = best ? *best : RichPalette{};
    PushHistory(in.phase, out.name);
    return out;
}

void PaletteManager::PushHistory(const std::string& phase, const std::string& name) {
    auto& dq = m_recent[phase];
    dq.push_back(name);
    while (dq.size() > m_rules.max_recent_history) dq.pop_front();
}

}