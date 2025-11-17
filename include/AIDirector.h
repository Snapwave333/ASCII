#pragma once

#include "NeonGlyph.h"
#include "DirectorCommand.h"
#include "MusicAnalyzer.h"
#include "StoryContext.h"
#include "PaletteManager.h"
#include <string>
#include <deque>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>
#include <memory>
#include <unordered_map>

namespace NeonGlyph {

struct DirectorState {
    std::string current_scene_id;
    NarrativeBeat narrative_beat = NarrativeBeat::Setup;
    std::string theme;
    std::string palette;
    std::string glyph_map;
    std::string intent_mode;
    std::string emotional_tone;
    std::string section;
    float32 section_progress = 0.0f;
    float32 energy_smoothed = 0.0f;
    Pacing pacing = Pacing::Medium;
    ShotType shot_type = ShotType::WideShot;
    std::deque<ShotType> shot_history;
    uint32 transition_cooldown_ms = 0;
    uint64 random_seed = 0;
    bool nsfw_blocked = true;
};

class AIDirector {
public:
    AIDirector();
    ~AIDirector();

    Result Initialize(const Config& config);
    void Shutdown();

    DirectorCommand Update(const MusicData& data);

    void SetRandomSeed(uint64 seed);
    void SetSafetyEnabled(bool enabled);
    void SetIntentMode(const std::string& mode);
    
    // Story integration methods
    Result LoadStorySet(const std::string& json_content);
    void SetStoryEnabled(bool enabled) { m_story_enabled = enabled; }
    const StoryContext& GetStoryContext() const { return m_story_context; }

    DirectorState GetState() const;

private:
    Config m_config;
    std::atomic<bool> m_initialized;
    DirectorState m_state;
    std::chrono::steady_clock::time_point m_lastTransition;
    std::string m_lastSection;
    uint32 m_shotCooldownMs = 500;
    uint32 m_transitionMinIntervalMs = 400;
    float32 m_energyThresholdHigh = 0.7f;
    float32 m_energyThresholdLow = 0.3f;
    
    // Story integration
    std::unique_ptr<StoryClock> m_story_clock;
    std::unique_ptr<NoveltyDetector> m_novelty_detector;
    std::unique_ptr<AudioSilenceDetector> m_silence_detector;
    std::unique_ptr<PaletteManager> m_palette_manager;
    StoryContext m_story_context;
    bool m_story_enabled = true;
    int64 m_set_start_time_ms = 0;

    NarrativeBeat MapSectionToBeat(const std::string& section) const;
    Motion MakeMotion(const MusicData& m) const;
    Composition MakeComposition() const;
    MiseEnScene MakeMiseEnScene() const;
    Transition NextTransition();
    std::vector<std::string> EffectsFor() const;
    std::vector<Directive> GeneratorDirectives(const MusicData& m) const;
    int32 PriorityFor() const;
    int32 TTLFor() const;
    bool Validate(const DirectorCommand& cmd) const;
    DirectorCommand Fallback(const MusicData& m) const;
    void UpdateStateFromMusic(const MusicData& m);
    bool SectionChanged(const std::string& s) const;
    void PushShot(ShotType s);
    std::string BuildPrompt(const MusicData& m) const;
    std::string BuildLLMRequest(const MusicData& m) const;
    bool QueryLLM(const std::string& body, std::string& response) const;
    void ApplyLLMResponse(const std::string& resp, DirectorCommand& cmd);
    void AIWorkerLoop();
    void EnqueueMusic(const MusicData& m);
    std::atomic<bool> m_shouldStop{false};
    std::thread m_aiThread;
    std::queue<MusicData> m_musicQueue;
    std::mutex m_queueMutex;
    std::atomic<bool> m_hasLLMCmd{false};
    DirectorCommand m_latestLLMCmd;
    
    // Story-aware methods
    VisualFingerprint CalculateVisualFingerprint(const DirectorCommand& cmd) const;
    DirectorCommand ApplyStoryConstraints(const DirectorCommand& cmd, const StoryContext& story);
    DirectorCommand CreateBlackoutCommand() const;
    Result LoadSceneWeights(const std::string& path);
    float GetSceneWeight(const std::string& scene_id) const;
    bool ShouldInjectEasterEgg() const;
    std::unordered_map<std::string, float> m_sceneWeights;
    int64 m_lastEasterEggMs = 0;
};

} // namespace NeonGlyph
