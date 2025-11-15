#pragma once

#include "NeonGlyph.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace NeonGlyph {

struct Scene {
    std::string id;
    std::string title;
    std::pair<float, float> relative_range; // [0.0, 1.0] within act
    std::vector<std::string> allowed_motifs;
    std::vector<std::string> banned_motifs;
    std::string intensity_curve; // e.g. "0.2->0.5"
    std::vector<std::string> dominant_palettes;
    float transition_speed = 1.0f; // multiplier for transition duration
};

struct Act {
    std::string id;
    std::string title;
    std::pair<int64, int64> time_range_sec; // absolute time range
    std::vector<std::string> theme_family;
    std::string arc_shape; // "slow_rise", "sharp_peak", "plateau", etc.
    std::vector<Scene> scenes;
    float base_transition_duration_ms = 2000.0f;
};

struct StorySet {
    std::string set_id;
    int64 total_duration_sec;
    std::vector<Act> acts;
    std::string default_palette;
    std::string default_motif;
};

struct StoryContext {
    const Act* act = nullptr;
    const Scene* scene = nullptr;
    float scene_phase = 0.0f; // [0,1] progress within current scene
    float act_phase = 0.0f;   // [0,1] progress within current act
    int64 set_time_ms = 0;    // absolute time since set start
    bool is_silent = false;   // from AudioSilenceDetector
    bool is_blackout = false; // currently in blackout state
};

class StoryClock {
public:
    StoryClock();
    ~StoryClock();

    Result Initialize(const StorySet& story_set);
    void Shutdown();

    // Update story context based on current time
    StoryContext Update(int64 set_time_ms, bool is_silent = false);
    
    // Get current story context without updating
    const StoryContext& GetContext() const { return m_context; }
    
    // Load story set from JSON
    Result LoadStorySet(const std::string& json_content);
    
    // Force jump to specific act/scene (for testing/debugging)
    void JumpTo(const std::string& act_id, const std::string& scene_id = "");
    
    // Check if we're in a valid story state
    bool IsValid() const { return m_initialized && m_story_set != nullptr; }

private:
    std::unique_ptr<StorySet> m_story_set;
    StoryContext m_context;
    bool m_initialized = false;
    
    const Act* FindActForTime(int64 time_ms) const;
    const Scene* FindSceneForPhase(const Act& act, float phase) const;
    float CalculateScenePhase(const Scene& scene, float act_phase) const;
    float CalculateActPhase(const Act& act, int64 time_ms) const;
};

// Visual fingerprint for novelty detection
struct VisualFingerprint {
    uint64 motif_hash = 0;      // hash of current motifs
    uint64 palette_hash = 0;    // hash of color palette
    uint64 composition_hash = 0;  // hash of composition parameters
    float intensity = 0.0f;     // overall visual intensity
    float complexity = 0.0f;    // visual complexity score
    
    // Calculate similarity between two fingerprints (0.0 = identical, 1.0 = completely different)
    float SimilarityTo(const VisualFingerprint& other) const;
};

// Novelty detection system
class NoveltyDetector {
public:
    NoveltyDetector();
    ~NoveltyDetector();
    
    void Initialize(float local_threshold = 0.3f, float act_threshold = 0.5f, size_t history_size = 50);
    
    // Check if visual state is novel enough
    bool IsNovel(const VisualFingerprint& fingerprint);
    
    // Add accepted fingerprint to history
    void AcceptFingerprint(const VisualFingerprint& fingerprint);
    
    // Get current novelty statistics
    float GetLocalNovelty() const { return m_local_novelty; }
    float GetActNovelty() const { return m_act_novelty; }
    
    // Clear history (useful when changing acts)
    void ClearLocalHistory();
    void ClearActHistory();

private:
    std::vector<VisualFingerprint> m_local_history;
    std::vector<VisualFingerprint> m_act_history;
    float m_local_threshold;
    float m_act_threshold;
    size_t m_max_history_size;
    float m_local_novelty = 1.0f;
    float m_act_novelty = 1.0f;
    
    float CalculateMinimumSimilarity(const std::vector<VisualFingerprint>& history, const VisualFingerprint& fingerprint) const;
};

// Audio silence detector for graceful fadeouts
class AudioSilenceDetector {
public:
    AudioSilenceDetector();
    ~AudioSilenceDetector();
    
    void Initialize(float threshold_db = -40.0f, int64 grace_period_ms = 3000);
    
    // Process audio frame and update silence state
    bool ProcessAudioFrame(float rms_level_db, int64 timestamp_ms);
    
    // Get current silence state
    bool IsSilent() const { return m_is_silent; }
    
    // Get time since silence started (-1 if not silent)
    int64 GetSilenceDurationMs() const;

private:
    float m_threshold_db;
    int64 m_grace_period_ms;
    bool m_is_silent = false;
    int64 m_silence_start_ms = -1;
    int64 m_last_timestamp_ms = 0;
};

} // namespace NeonGlyph