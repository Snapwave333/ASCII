#pragma once

#include "NeonGlyph.h"
#include <string>
#include <atomic>
#include <vector>
#include <deque>

namespace NeonGlyph {

struct MusicData {
    int32 bpm = 0;
    std::string key;
    float32 energy = 0.0f;
    float32 dynamics = 0.0f;
    std::string song_section;
    float32 beat_phase = 0.0f;
    float32 rms_level_db = -30.0f; // RMS level in dB for silence detection
};

class MusicAnalyzer {
public:
    MusicAnalyzer();
    ~MusicAnalyzer();

    Result Initialize(const Config& config);
    void Shutdown();

    Result Update(const AudioFrame& frame, const Spectrum& spectrum);

    MusicData GetLatest() const;

    void SetTempoSmoothing(float32 alpha);
    void SetDynamicsSmoothing(float32 alpha);
    void Reset();

private:
    Config m_config;
    std::atomic<bool> m_initialized;
    float32 m_tempoSmoothing;
    float32 m_dynamicsSmoothing;
    MusicData m_latest;

    float32 m_energyEnvelope;
    float32 m_avgEnergy;
    float32 m_lastInstantEnergy;
    uint64 m_lastBeatTimestamp;
    std::deque<float32> m_beatIntervalsMs;
    float32 m_fluxPrev;
    std::vector<float32> m_prevMagnitudes;
};

} // namespace NeonGlyph

