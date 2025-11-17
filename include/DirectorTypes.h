#pragma once

#include <string>
#include <vector>

namespace NeonGlyph {

struct DirectorDirective {
    // scene identity 
    std::string sceneId;          // e.g. "SCN-DEMO-RADIAL" 
    std::string motionPreset;     // e.g. "swirl_orbit" 
    std::string paletteShift;     // e.g. "warm_burst" 
    float intensity = 1.0f;       // 0.0–2.0 mapped from audio energy 
    std::string mood;             // e.g. "calm", "build", "peak" 

    // optional micro-events (can be empty) 
    std::vector<std::string> microEvents;
};

// State snapshot sent to Director Daemon
struct DirectorStateSnapshot {
    double time = 0.0;            // Current time in seconds
    
    struct AudioData {
        float rms = 0.0f;         // Root mean square energy
        float bass = 0.0f;        // Low frequency energy
        float mids = 0.0f;        // Mid frequency energy  
        float highs = 0.0f;       // High frequency energy
    } audio;
    
    struct SceneData {
        std::string id;           // Current scene identifier
        float intensity = 0.0f;   // Current scene intensity
        std::string palette;      // Current palette name
    } scene;
    
    // Recent history for context
    std::vector<std::string> recentScenes;
    std::vector<std::string> recentMoods;
};

} // namespace NeonGlyph