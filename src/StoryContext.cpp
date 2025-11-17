#include "StoryContext.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace NeonGlyph {

// StoryClock Implementation
StoryClock::StoryClock() = default;
StoryClock::~StoryClock() = default;

Result StoryClock::Initialize(const StorySet& story_set) {
    m_story_set = std::make_unique<StorySet>(story_set);
    m_context = StoryContext{};
    m_initialized = true;
    return Result::Success;
}

void StoryClock::Shutdown() {
    m_initialized = false;
    m_story_set.reset();
    m_context = StoryContext{};
}

StoryContext StoryClock::Update(int64 set_time_ms, bool is_silent) {
    if (!m_initialized || !m_story_set) {
        return m_context;
    }
    
    m_context.set_time_ms = set_time_ms;
    m_context.is_silent = is_silent;
    
    // Find current act
    const Act* act = FindActForTime(set_time_ms / 1000);
    if (!act && !m_story_set->acts.empty()) {
        act = &m_story_set->acts[0];
    }
    
    m_context.act = act;
    
    if (act) {
        m_context.act_phase = CalculateActPhase(*act, set_time_ms / 1000);
        const Scene* scene = FindSceneForPhase(*act, m_context.act_phase);
        m_context.scene = scene;
        
        if (scene) {
            m_context.scene_phase = CalculateScenePhase(*scene, m_context.act_phase);
        } else {
            m_context.scene_phase = m_context.act_phase;
        }
    } else {
        m_context.act_phase = 0.0f;
        m_context.scene_phase = 0.0f;
    }
    
    return m_context;
}

Result StoryClock::LoadStorySet(const std::string& json_content) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_content);
        
        auto story_set = std::make_unique<StorySet>();
        story_set->set_id = j.value("set_id", "default");
        story_set->total_duration_sec = j.value("total_duration_sec", 3600);
        story_set->default_palette = j.value("default_palette", "default");
        story_set->default_motif = j.value("default_motif", "default");
        
        if (j.contains("acts") && j["acts"].is_array()) {
            for (const auto& act_json : j["acts"]) {
                Act act;
                act.id = act_json.value("id", "");
                act.title = act_json.value("title", "");
                
                if (act_json.contains("time_range_sec") && act_json["time_range_sec"].is_array() && act_json["time_range_sec"].size() == 2) {
                    act.time_range_sec.first = act_json["time_range_sec"][0];
                    act.time_range_sec.second = act_json["time_range_sec"][1];
                }
                
                if (act_json.contains("theme_family") && act_json["theme_family"].is_array()) {
                    for (const auto& theme : act_json["theme_family"]) {
                        act.theme_family.push_back(theme);
                    }
                }
                
                act.arc_shape = act_json.value("arc_shape", "linear");
                act.base_transition_duration_ms = act_json.value("base_transition_duration_ms", 2000.0f);
                
                if (act_json.contains("scenes") && act_json["scenes"].is_array()) {
                    for (const auto& scene_json : act_json["scenes"]) {
                        Scene scene;
                        scene.id = scene_json.value("id", "");
                        scene.title = scene_json.value("title", "");
                        
                        if (scene_json.contains("relative_range") && scene_json["relative_range"].is_array() && scene_json["relative_range"].size() == 2) {
                            scene.relative_range.first = scene_json["relative_range"][0];
                            scene.relative_range.second = scene_json["relative_range"][1];
                        }
                        
                        scene.intensity_curve = scene_json.value("intensity_curve", "0.5->0.5");
                        
                        if (scene_json.contains("dominant_palettes") && scene_json["dominant_palettes"].is_array()) {
                            for (const auto& palette : scene_json["dominant_palettes"]) {
                                scene.dominant_palettes.push_back(palette);
                            }
                        }
                        
                        if (scene_json.contains("allowed_motifs") && scene_json["allowed_motifs"].is_array()) {
                            for (const auto& motif : scene_json["allowed_motifs"]) {
                                scene.allowed_motifs.push_back(motif);
                            }
                        }
                        
                        if (scene_json.contains("banned_motifs") && scene_json["banned_motifs"].is_array()) {
                            for (const auto& motif : scene_json["banned_motifs"]) {
                                scene.banned_motifs.push_back(motif);
                            }
                        }
                        
                        scene.transition_speed = scene_json.value("transition_speed", 1.0f);
                        act.scenes.push_back(scene);
                    }
                }
                
                story_set->acts.push_back(act);
            }
        }
        
        // Replace current story set
        m_story_set = std::move(story_set);
        m_initialized = true;
        
        return Result::Success;
        
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Failed to parse story JSON: " << e.what() << std::endl;
        return Result::InvalidArgument;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load story set: " << e.what() << std::endl;
        return Result::InvalidArgument;
    }
}

const Act* StoryClock::FindActForTime(int64 time_sec) const {
    if (!m_story_set) return nullptr;
    
    for (const auto& act : m_story_set->acts) {
        if (time_sec >= act.time_range_sec.first && time_sec < act.time_range_sec.second) {
            return &act;
        }
    }
    
    return nullptr;
}

const Scene* StoryClock::FindSceneForPhase(const Act& act, float phase) const {
    for (const auto& scene : act.scenes) {
        if (phase >= scene.relative_range.first && phase < scene.relative_range.second) {
            return &scene;
        }
    }
    
    return nullptr;
}

float StoryClock::CalculateScenePhase(const Scene& scene, float act_phase) const {
    float scene_duration = scene.relative_range.second - scene.relative_range.first;
    if (scene_duration <= 0.0f) return 0.0f;
    
    float normalized_phase = (act_phase - scene.relative_range.first) / scene_duration;
    return std::clamp(normalized_phase, 0.0f, 1.0f);
}

float StoryClock::CalculateActPhase(const Act& act, int64 time_sec) const {
    float act_duration = static_cast<float>(act.time_range_sec.second - act.time_range_sec.first);
    if (act_duration <= 0.0f) return 0.0f;
    
    float relative_time = static_cast<float>(time_sec - act.time_range_sec.first);
    return std::clamp(relative_time / act_duration, 0.0f, 1.0f);
}

// VisualFingerprint Implementation
float VisualFingerprint::SimilarityTo(const VisualFingerprint& other) const {
    float motif_diff = static_cast<float>(motif_hash != other.motif_hash);
    float palette_diff = static_cast<float>(palette_hash != other.palette_hash);
    float composition_diff = static_cast<float>(composition_hash != other.composition_hash);
    
    float intensity_diff = std::abs(intensity - other.intensity);
    float complexity_diff = std::abs(complexity - other.complexity);
    
    float total_diff = (motif_diff * 0.3f + palette_diff * 0.3f + composition_diff * 0.2f + 
                       intensity_diff * 0.1f + complexity_diff * 0.1f);
    
    return std::clamp(total_diff, 0.0f, 1.0f);
}

// NoveltyDetector Implementation
NoveltyDetector::NoveltyDetector() = default;
NoveltyDetector::~NoveltyDetector() = default;

void NoveltyDetector::Initialize(float local_threshold, float act_threshold, size_t history_size) {
    m_local_threshold = local_threshold;
    m_act_threshold = act_threshold;
    m_max_history_size = history_size;
    m_local_history.clear();
    m_act_history.clear();
    m_local_novelty = 1.0f;
    m_act_novelty = 1.0f;
}

bool NoveltyDetector::IsNovel(const VisualFingerprint& fingerprint) {
    float local_similarity = CalculateMinimumSimilarity(m_local_history, fingerprint);
    float act_similarity = CalculateMinimumSimilarity(m_act_history, fingerprint);
    
    m_local_novelty = 1.0f - local_similarity;
    m_act_novelty = 1.0f - act_similarity;
    
    return (m_local_novelty >= m_local_threshold) && (m_act_novelty >= m_act_threshold);
}

void NoveltyDetector::AcceptFingerprint(const VisualFingerprint& fingerprint) {
    m_local_history.push_back(fingerprint);
    m_act_history.push_back(fingerprint);
    
    if (m_local_history.size() > m_max_history_size) {
        m_local_history.erase(m_local_history.begin());
    }
    
    if (m_act_history.size() > m_max_history_size * 2) {
        m_act_history.erase(m_act_history.begin());
    }
}

void NoveltyDetector::ClearLocalHistory() {
    m_local_history.clear();
    m_local_novelty = 1.0f;
}

void NoveltyDetector::ClearActHistory() {
    m_act_history.clear();
    m_act_novelty = 1.0f;
}

float NoveltyDetector::CalculateMinimumSimilarity(const std::vector<VisualFingerprint>& history, const VisualFingerprint& fingerprint) const {
    if (history.empty()) return 1.0f;
    
    float max_similarity = 0.0f;
    for (const auto& historical : history) {
        float similarity = 1.0f - fingerprint.SimilarityTo(historical);
        max_similarity = std::max(max_similarity, similarity);
    }
    
    return max_similarity;
}

// AudioSilenceDetector Implementation
AudioSilenceDetector::AudioSilenceDetector() = default;
AudioSilenceDetector::~AudioSilenceDetector() = default;

void AudioSilenceDetector::Initialize(float threshold_db, int64 grace_period_ms) {
    m_threshold_db = threshold_db;
    m_grace_period_ms = grace_period_ms;
    m_is_silent = false;
    m_silence_start_ms = -1;
    m_last_timestamp_ms = 0;
}

bool AudioSilenceDetector::ProcessAudioFrame(float rms_level_db, int64 timestamp_ms) {
    m_last_timestamp_ms = timestamp_ms;
    
    bool was_silent = m_is_silent;
    bool currently_silent = rms_level_db < m_threshold_db;
    
    if (currently_silent && !was_silent) {
        if (m_silence_start_ms < 0) {
            m_silence_start_ms = timestamp_ms;
        }
        
        int64 silence_duration = timestamp_ms - m_silence_start_ms;
        if (silence_duration >= m_grace_period_ms) {
            m_is_silent = true;
        }
    } else if (!currently_silent) {
        m_is_silent = false;
        m_silence_start_ms = -1;
    }
    
    return m_is_silent;
}

int64 AudioSilenceDetector::GetSilenceDurationMs() const {
    if (!m_is_silent || m_silence_start_ms < 0) return -1;
    return m_last_timestamp_ms - m_silence_start_ms;
}

} // namespace NeonGlyph