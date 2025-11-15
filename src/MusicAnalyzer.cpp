#include "MusicAnalyzer.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace NeonGlyph {

static float32 Clamp01(float32 v) { return std::max(0.0f, std::min(1.0f, v)); }

MusicAnalyzer::MusicAnalyzer()
    : m_initialized(false)
    , m_tempoSmoothing(0.85f)
    , m_dynamicsSmoothing(0.85f)
    , m_energyEnvelope(0.0f)
    , m_avgEnergy(0.0f)
    , m_lastInstantEnergy(0.0f)
    , m_lastBeatTimestamp(0)
    , m_fluxPrev(0.0f) {
}

MusicAnalyzer::~MusicAnalyzer() {
    Shutdown();
}

Result MusicAnalyzer::Initialize(const Config& config) {
    m_config = config;
    m_initialized.store(true);
    Reset();
    return Result::Success;
}

void MusicAnalyzer::Shutdown() {
    m_initialized.store(false);
    m_prevMagnitudes.clear();
    m_beatIntervalsMs.clear();
}

void MusicAnalyzer::Reset() {
    m_latest = {};
    m_latest.key = "Unknown";
    m_latest.song_section = "intro";
    m_latest.beat_phase = 0.0f;
    m_latest.bpm = 0;
    m_energyEnvelope = 0.0f;
    m_avgEnergy = 0.0f;
    m_lastInstantEnergy = 0.0f;
    m_lastBeatTimestamp = 0;
    m_fluxPrev = 0.0f;
}

static float32 ComputeRMS(const AudioFrame& frame) {
    if (!frame.data || frame.frameCount == 0 || frame.channelCount == 0) return 0.0f;
    const uint64 samples = static_cast<uint64>(frame.frameCount) * frame.channelCount;
    float64 sumSq = 0.0;
    for (uint64 i = 0; i < samples; ++i) {
        float64 s = frame.data[i];
        sumSq += s * s;
    }
    return static_cast<float32>(std::sqrt(sumSq / std::max<uint64>(1, samples)));
}

static float32 SpectralFlux(const Spectrum& spectrum, std::vector<float32>& prev, float32& prevFlux) {
    if (spectrum.binCount == 0 || spectrum.magnitudes == nullptr) return 0.0f;
    if (prev.size() != spectrum.binCount) prev.assign(spectrum.binCount, 0.0f);
    float32 flux = 0.0f;
    for (uint32 i = 0; i < spectrum.binCount; ++i) {
        float32 diff = spectrum.magnitudes[i] - prev[i];
        if (diff > 0.0f) flux += diff;
    }
    prevFlux = flux;
    // update prev
    for (uint32 i = 0; i < spectrum.binCount; ++i) prev[i] = spectrum.magnitudes[i];
    return flux;
}

static float32 DominantFrequency(const Spectrum& spectrum) {
    if (spectrum.binCount == 0 || spectrum.magnitudes == nullptr) return 0.0f;
    uint32 idx = 0;
    float32 maxMag = -1.0f;
    for (uint32 i = 0; i < spectrum.binCount; ++i) {
        float32 m = spectrum.magnitudes[i];
        if (m > maxMag) { maxMag = m; idx = i; }
    }
    return spectrum.frequencyResolution * idx;
}

static std::string EstimateKeyFromFrequency(float32 freq) {
    if (freq <= 0.0f) return "Unknown";
    // Map frequency to nearest MIDI note
    // MIDI 69 = A4 = 440 Hz
    float64 midi = 69.0 + 12.0 * std::log2(freq / 440.0);
    int note = static_cast<int>(std::round(midi)) % 12;
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    std::string pc = names[(note + 12) % 12];
    return pc + "m"; // crude minor bias
}

static float32 Median(std::deque<float32> d) {
    if (d.empty()) return 0.0f;
    std::vector<float32> v(d.begin(), d.end());
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n % 2) return v[n/2];
    return (v[n/2 - 1] + v[n/2]) * 0.5f;
}

Result MusicAnalyzer::Update(const AudioFrame& frame, const Spectrum& spectrum) {
    if (!m_initialized.load()) return Result::InitializationFailed;

    float32 instantEnergy = ComputeRMS(frame);
    m_energyEnvelope = m_dynamicsSmoothing * m_energyEnvelope + (1.0f - m_dynamicsSmoothing) * instantEnergy;
    m_avgEnergy = 0.98f * m_avgEnergy + 0.02f * instantEnergy;

    float32 flux = SpectralFlux(spectrum, m_prevMagnitudes, m_fluxPrev);
    float32 dynamics = std::abs(instantEnergy - m_lastInstantEnergy) * 0.5f + flux * 0.05f;
    m_lastInstantEnergy = instantEnergy;

    const float32 threshold = m_avgEnergy * 1.15f; // adaptive threshold
    if (m_energyEnvelope > threshold && instantEnergy > m_avgEnergy) {
        if (m_lastBeatTimestamp == 0 || (frame.timestamp - m_lastBeatTimestamp) > 120) { // debounce
            if (m_lastBeatTimestamp != 0) {
                float32 intervalMs = static_cast<float32>(frame.timestamp - m_lastBeatTimestamp) / 1000.0f;
                if (intervalMs > 200.0f && intervalMs < 2000.0f) {
                    m_beatIntervalsMs.push_back(intervalMs);
                    if (m_beatIntervalsMs.size() > 32) m_beatIntervalsMs.pop_front();
                }
            }
            m_lastBeatTimestamp = frame.timestamp;
        }
    }

    float32 bpm = m_latest.bpm;
    float32 medianInterval = Median(m_beatIntervalsMs);
    if (medianInterval > 0.0f) {
        float32 est = 60000.0f / medianInterval;
        bpm = m_tempoSmoothing * bpm + (1.0f - m_tempoSmoothing) * est;
    }

    float32 domFreq = DominantFrequency(spectrum);
    std::string key = EstimateKeyFromFrequency(domFreq);

    float32 beatLenMs = (bpm > 0.0f) ? (60000.0f / bpm) : 0.0f;
    float32 phase = 0.0f;
    if (beatLenMs > 0.0f && m_lastBeatTimestamp > 0) {
        float32 tSince = static_cast<float32>(frame.timestamp - m_lastBeatTimestamp) / 1000.0f;
        phase = std::fmod(tSince, beatLenMs) / beatLenMs;
    }

    std::string section = m_latest.song_section;
    if (bpm > 0.0f) {
        if (flux > 0.5f && dynamics > 0.2f) section = "bridge";
        else if (instantEnergy > m_avgEnergy * 1.3f && dynamics > 0.25f) section = "chorus";
        else if (instantEnergy < m_avgEnergy * 0.9f) section = "verse";
        else section = "build";
    }

    m_latest.bpm = static_cast<int32>(std::round(bpm));
    m_latest.key = key;
    m_latest.energy = Clamp01(instantEnergy);
    m_latest.dynamics = Clamp01(dynamics);
    m_latest.song_section = section;
    m_latest.beat_phase = Clamp01(phase);

    return Result::Success;
}

MusicData MusicAnalyzer::GetLatest() const {
    return m_latest;
}

void MusicAnalyzer::SetTempoSmoothing(float32 alpha) {
    m_tempoSmoothing = std::clamp(alpha, 0.0f, 0.999f);
}

void MusicAnalyzer::SetDynamicsSmoothing(float32 alpha) {
    m_dynamicsSmoothing = std::clamp(alpha, 0.0f, 0.999f);
}

} // namespace NeonGlyph
