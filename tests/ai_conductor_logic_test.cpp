#include "NeonGlyph.h"
#include "AIConductor.h"
#include <iostream>

using namespace NeonGlyph;

static Spectrum makeSpectrum(uint32_t bins, float32 res, float32 peakFreq) {
    Spectrum s{};
    s.binCount = bins;
    s.frequencyResolution = res;
    s.magnitudes = new float32[bins];
    for (uint32_t i = 0; i < bins; ++i) {
        float32 f = i * res;
        float32 v = std::exp(-std::abs(f - peakFreq) / 500.0f);
        s.magnitudes[i] = v;
    }
    return s;
}

static AudioFrame makeFrame(uint32_t frames, uint32_t channels, float32 amp) {
    AudioFrame a{};
    a.frameCount = frames;
    a.channelCount = channels;
    a.data = new float32[static_cast<size_t>(frames) * channels];
    for (uint32_t i = 0; i < frames * channels; ++i) a.data[i] = amp;
    a.timestamp = 100000;
    return a;
}

int main() {
    AIConductor ai;
    Config cfg{};
    cfg.ai.enabled = true;
    if (ai.Initialize(cfg) != Result::Success) {
        std::cout << "SKIP: ai init" << std::endl; return 2;
    }
    Spectrum s1 = makeSpectrum(2048, 10.0f, 300.0f);
    ai.AnalyzeSpectrum(s1);
    std::string g1 = ai.GenerateCharacterSetSuggestion();
    bool ok1 = !g1.empty();
    Spectrum s2 = makeSpectrum(2048, 10.0f, 1400.0f);
    AudioFrame f2 = makeFrame(1024, 2, 0.8f);
    ai.AnalyzeMood(f2, s2);
    std::string mood = ai.GenerateColorPaletteSuggestion();
    bool ok2 = !mood.empty();
    delete[] s1.magnitudes;
    delete[] s2.magnitudes;
    delete[] f2.data;
    bool all = ok1 && ok2;
    std::cout << (all ? "PASS" : "FAIL") << ": ai conductor logic" << std::endl;
    return all ? 0 : 1;
}