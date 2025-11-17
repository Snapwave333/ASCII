#pragma once

#include "NeonGlyph.h"

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>

namespace NeonGlyph {

class AIConductor {
public:
    AIConductor();
    ~AIConductor();

    Result Initialize(const Config& config);
    void Shutdown();
    
    // Real-time analysis
    Result AnalyzeAudioFrame(const AudioFrame& frame);
    Result AnalyzeSpectrum(const Spectrum& spectrum);
    
    // Musical intelligence
    std::string GetCurrentGenre() const { return m_currentGenre; }
    std::string GetCurrentMood() const { return m_currentMood; }
    float32 GetEnergyLevel() const { return m_energyLevel; }
    float32 GetDanceability() const { return m_danceability; }
    
    // Scene generation
    std::string GenerateSceneDescription();
    std::string GenerateColorPaletteSuggestion();
    std::string GenerateCharacterSetSuggestion();
    
    // Event system
    bool ShouldTriggerEvent() const { return m_eventTriggered; }
    std::string GetEventType() const { return m_eventType; }
    void AcknowledgeEvent();
    
    // Context management
    void UpdateContext(const std::string& key, const std::string& value);
    std::string GetContext(const std::string& key) const;
    
    // Safety and filtering
    bool IsContentSafe(const std::string& content);
    std::string FilterNSFWContent(const std::string& content);
    
    // Performance monitoring
    float32 GetInferenceTimeMs() const { return m_inferenceTimeMs; }
    uint32 GetModelMemoryUsageMB() const { return m_modelMemoryUsageMB; }
    
    // Director System Integration
    void ApplyDirective(const DirectorDirective& directive);

private:
    // ONNX Runtime (optional - stubbed when not available)
    void* m_onnxEnv; // Opaque pointer for ONNX Runtime
    void* m_sessionOptions;
    void* m_genreClassifier;
    void* m_moodPredictor;
    void* m_sceneGenerator;
    
    // Model inputs/outputs
    std::vector<const char*> m_genreInputNames;
    std::vector<const char*> m_genreOutputNames;
    std::vector<const char*> m_moodInputNames;
    std::vector<const char*> m_moodOutputNames;
    std::vector<const char*> m_sceneInputNames;
    std::vector<const char*> m_sceneOutputNames;
    
    // Model memory info
    void* m_memoryInfo;
    
    // Current state
    std::string m_currentGenre;
    std::string m_currentMood;
    float32 m_energyLevel;
    float32 m_danceability;
    float32 m_valence;
    float32 m_arousal;
    
    // Event system
    std::atomic<bool> m_eventTriggered;
    std::string m_eventType;
    std::chrono::steady_clock::time_point m_lastEventTime;
    
    // Context management
    std::map<std::string, std::string> m_context;
    mutable std::mutex m_contextMutex;
    
    // Audio analysis history
    std::vector<float32> m_energyHistory;
    std::vector<float32> m_spectralCentroidHistory;
    std::vector<float32> m_zeroCrossingRateHistory;
    std::vector<std::string> m_genreHistory;
    std::vector<std::string> m_moodHistory;
    
    // NSFW filtering
    std::vector<std::string> m_nsfwKeywords;
    std::vector<std::string> m_safetyWhitelist;
    
    // Performance metrics
    float32 m_inferenceTimeMs;
    uint32 m_modelMemoryUsageMB;
    
    // Threading
    std::thread m_analysisThread;
    std::atomic<bool> m_shouldStop;
    std::queue<AudioFrame> m_audioQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    
    // Configuration
    Config m_config;
    
    // Private methods
#ifdef NEONGLYPH_HAVE_ONNXRUNTIME
    Result LoadModels();
    Result InitializeGenreClassifier();
    Result InitializeMoodPredictor();
    Result InitializeSceneGenerator();
#endif
    
    // Analysis methods
    Result AnalyzeGenre(const Spectrum& spectrum);
    Result AnalyzeMood(const AudioFrame& frame, const Spectrum& spectrum);
    Result AnalyzeEnergy(const AudioFrame& frame);
    Result AnalyzeDanceability(const Spectrum& spectrum);
    
    // Scene generation
    Result GenerateSceneFromContext();
    std::string GeneratePromptFromAudioState();
    std::string GenerateColorPaletteFromMood();
    std::string GenerateCharsetFromGenre();
    
    // Event detection
    void DetectMusicalEvents();
    bool DetectBeatDrop();
    bool DetectBuildUp();
    bool DetectBreakDown();
    bool DetectGenreChange();
    bool DetectMoodShift();
    
    // Utility methods
    std::vector<float32> ExtractAudioFeatures(const AudioFrame& frame);
    std::vector<float32> ExtractSpectralFeatures(const Spectrum& spectrum);
    std::string ClassifyGenre(const std::vector<float32>& features);
    std::string PredictMood(const std::vector<float32>& features);
    
    // NSFW detection
    void LoadNSFWKeywords();
    bool ContainsNSFWContent(const std::string& text);
    std::string CleanNSFWContent(const std::string& text);
    
    // Threading
    void AnalysisThreadFunc();
    void ProcessAudioQueue();
    
    // Performance monitoring
    void UpdatePerformanceMetrics();
    void LogInferenceTime(float32 timeMs);
};

} // namespace NeonGlyph