#pragma once

#include "NeonGlyph.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace NeonGlyph {

enum class NarrativeBeat : uint32 {
    Setup = 0,
    RisingAction = 1,
    Climax = 2,
    FallingAction = 3,
    Finale = 4,
    Bridge = 5,
    Chorus = 6,
    Verse = 7,
    Breakdown = 8
};

enum class Pacing : uint32 {
    LongTake = 0,
    Medium = 1,
    FastCuts = 2
};

enum class ShotType : uint32 {
    WideShot = 0,
    CloseUp = 1,
    TrackingShot = 2,
    Pan = 3,
    DollyZoom = 4
};

struct Motion {
    std::string type;
    std::string axis;
    float32 speed = 0.0f;
    float32 amplitude = 0.0f;
    int32 duration_ms = 0;
    std::string easing;
};

struct Composition {
    float32 focus_density = 0.0f;
    bool rule_of_thirds = true;
    float32 center_bias = 0.0f;
    std::string symmetry;
};

struct MiseEnScene {
    std::string palette;
    float32 brightness = 1.0f;
    float32 contrast = 1.0f;
    std::string set_dressing;
};

struct Transition {
    std::string type;
    int32 duration_ms = 0;
};

struct AudioSync {
    bool beat_sync = true;
    int32 bpm = 0;
    float32 phase = 0.0f;
    std::string key;
    float32 energy = 0.0f;
    std::string song_section;
};

struct Intent {
    std::string mode;
    std::string description;
};

struct Directive {
    std::string name;
    std::unordered_map<std::string, std::string> params;
};

struct DirectorCommand {
    int64 timestamp_ms = 0;
    std::string scene_id;
    NarrativeBeat narrative_beat = NarrativeBeat::Setup;
    Pacing pacing = Pacing::Medium;
    ShotType shot_type = ShotType::WideShot;
    Motion motion;
    Composition composition;
    MiseEnScene mise_en_scene;
    Transition transition;
    std::vector<std::string> effects;
    std::string glyph_map;
    AudioSync audio_sync;
    Intent intent;
    std::vector<Directive> directives;
    int32 priority = 0;
    int32 ttl_ms = 0;
    bool repeat = false;
};

} // namespace NeonGlyph

