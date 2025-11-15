#include "AudioEngine.h"
#include <initguid.h>
#include <combaseapi.h>
#include <cmath>
#include <algorithm>

namespace NeonGlyph {

AudioEngine::AudioEngine()
    : m_deviceEnumerator(nullptr)
    , m_audioDevice(nullptr)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    , m_waveFormat(nullptr)
    , m_isCapturing(false)
    , m_beatDetected(false)
    , m_currentBPM(0.0f)
    , m_currentVolume(0.0f)
    , m_spectralCentroid(0.0f)
    , m_zeroCrossingRate(0.0f)
    , m_shouldStop(false)
    , m_avrtHandle(nullptr)
    , m_taskIndex(0) {}

AudioEngine::~AudioEngine() { Shutdown(); }

Result AudioEngine::Initialize(const Config& config) {
    m_config = config;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return Result::InitializationFailed;
    Result r = InitializeWASAPI();
    if (r != Result::Success) return r;
    InitializeFFT();
    return Result::Success;
}

void AudioEngine::Shutdown() {
    StopCapture();
    CleanupFFT();
    if (m_captureClient) { m_captureClient->Release(); m_captureClient = nullptr; }
    if (m_audioClient) { m_audioClient->Release(); m_audioClient = nullptr; }
    if (m_audioDevice) { m_audioDevice->Release(); m_audioDevice = nullptr; }
    if (m_deviceEnumerator) { m_deviceEnumerator->Release(); m_deviceEnumerator = nullptr; }
    if (m_waveFormat) { CoTaskMemFree(m_waveFormat); m_waveFormat = nullptr; }
    CoUninitialize();
}

Result AudioEngine::InitializeWASAPI() {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&m_deviceEnumerator));
    if (FAILED(hr)) return Result::InitializationFailed;
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_audioDevice);
    if (FAILED(hr)) return Result::InitializationFailed;
    hr = m_audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&m_audioClient));
    if (FAILED(hr)) return Result::InitializationFailed;
    hr = m_audioClient->GetMixFormat(&m_waveFormat);
    if (FAILED(hr)) return Result::InitializationFailed;
    m_format.sampleRate = m_waveFormat->nSamplesPerSec;
    m_format.channels = m_waveFormat->nChannels;
    m_format.bitsPerSample = m_waveFormat->wBitsPerSample;
    m_format.frameSize = m_waveFormat->nBlockAlign;
    REFERENCE_TIME hnsBufferDuration = 10000000;
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsBufferDuration, 0, m_waveFormat, nullptr);
    if (FAILED(hr)) return Result::InitializationFailed;
    hr = m_audioClient->GetService(IID_PPV_ARGS(&m_captureClient));
    if (FAILED(hr)) return Result::InitializationFailed;
    m_audioBuffer.resize(1024 * m_format.channels);
    m_fftInput.resize(1024);
    m_fftOutput.resize(1024);
    m_spectrumMagnitudes.resize(512);
    m_spectrumPhases.resize(512);
    return Result::Success;
}

Result AudioEngine::StartCapture() {
    if (m_isCapturing) return Result::Success;
    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr)) return Result::Error;
    m_shouldStop.store(false);
    SetThreadPriority();
    Result r = StartCaptureThread();
    if (r != Result::Success) return r;
    m_isCapturing = true;
    return Result::Success;
}

Result AudioEngine::StopCapture() {
    if (!m_isCapturing) return Result::Success;
    m_shouldStop.store(true);
    if (m_captureThread.joinable()) m_captureThread.join();
    RestoreThreadPriority();
    HRESULT hr = m_audioClient->Stop();
    m_isCapturing = false;
    return FAILED(hr) ? Result::Error : Result::Success;
}

Result AudioEngine::StartCaptureThread() {
    m_captureThread = std::thread(&AudioEngine::CaptureThreadFunc, this);
    return Result::Success;
}

void AudioEngine::CaptureThreadFunc() {
    while (!m_shouldStop.load()) {
        ProcessCapturedData();
        Sleep(5);
    }
}

Result AudioEngine::ProcessAudioFrame(AudioFrame& frame) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    frame.data = m_audioBuffer.data();
    frame.frameCount = static_cast<uint32>(m_audioBuffer.size() / std::max<uint32>(1, m_format.channels));
    frame.channelCount = m_format.channels;
    frame.timestamp = static_cast<uint64>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    return Result::Success;
}

Result AudioEngine::GetSpectrum(Spectrum& spectrum) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    spectrum.magnitudes = m_spectrumMagnitudes.data();
    spectrum.phases = m_spectrumPhases.data();
    spectrum.binCount = static_cast<uint32>(m_spectrumMagnitudes.size());
    spectrum.frequencyResolution = static_cast<float32>(m_format.sampleRate) / static_cast<float32>(m_fftInput.size());
    return Result::Success;
}

Result AudioEngine::GetWaveform(std::vector<float32>& waveform) {
    std::lock_guard<std::mutex> lock(m_audioMutex);
    waveform = m_audioBuffer;
    return Result::Success;
}

void AudioEngine::ConvertToFloat32(const uint8_t* data, uint32_t frameCount) {
    if (!data) return;
    if (m_waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        const float32* src = reinterpret_cast<const float32*>(data);
        size_t count = static_cast<size_t>(frameCount) * m_format.channels;
        m_audioBuffer.assign(src, src + count);
    } else if (m_waveFormat->wFormatTag == WAVE_FORMAT_PCM && m_waveFormat->wBitsPerSample == 16) {
        const int16_t* src = reinterpret_cast<const int16_t*>(data);
        size_t count = static_cast<size_t>(frameCount) * m_format.channels;
        m_audioBuffer.resize(count);
        for (size_t i = 0; i < count; ++i) m_audioBuffer[i] = static_cast<float32>(src[i]) / 32768.0f;
    }
}

void AudioEngine::ApplyWindowFunction() {
    size_t n = (std::min)(m_fftInput.size(), m_audioBuffer.size());
    for (size_t i = 0; i < n; ++i) {
        float32 w = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * static_cast<float32>(i) / static_cast<float32>(n - 1)));
        m_fftInput[i] = m_audioBuffer[i] * w;
    }
}

void AudioEngine::PerformFFT() {
    size_t n = m_fftInput.size();
    size_t bins = m_spectrumMagnitudes.size();
    for (size_t k = 0; k < bins; ++k) {
        float64 re = 0.0;
        float64 im = 0.0;
        for (size_t t = 0; t < n; ++t) {
            float64 ang = -2.0 * 3.14159265358979323846 * static_cast<float64>(k) * static_cast<float64>(t) / static_cast<float64>(n);
            re += m_fftInput[t] * std::cos(ang);
            im += m_fftInput[t] * std::sin(ang);
        }
        float64 mag = std::sqrt(re * re + im * im);
        m_spectrumMagnitudes[k] = static_cast<float32>(mag);
        m_spectrumPhases[k] = static_cast<float32>(std::atan2(im, re));
    }
}

void AudioEngine::CalculateSpectrum() {
    ApplyWindowFunction();
    PerformFFT();
}

float32 AudioEngine::CalculateVolume(const std::vector<float32>& audioData) {
    float64 s = 0.0;
    for (float32 v : audioData) s += static_cast<float64>(v) * static_cast<float64>(v);
    s /= std::max<size_t>(1, audioData.size());
    return static_cast<float32>(std::sqrt(s));
}

float32 AudioEngine::CalculateSpectralCentroid() {
    float64 num = 0.0;
    float64 den = 0.0;
    float64 res = static_cast<float64>(m_format.sampleRate) / static_cast<float64>(m_fftInput.size());
    for (size_t i = 0; i < m_spectrumMagnitudes.size(); ++i) {
        float64 f = res * static_cast<float64>(i);
        float64 m = static_cast<float64>(m_spectrumMagnitudes[i]);
        num += f * m;
        den += m;
    }
    return den > 0.0 ? static_cast<float32>(num / den) : 0.0f;
}

float32 AudioEngine::CalculateZeroCrossingRate() {
    size_t n = m_audioBuffer.size();
    if (n < 2) return 0.0f;
    uint32 crossings = 0;
    for (size_t i = 1; i < n; ++i) {
        crossings += (m_audioBuffer[i - 1] > 0 && m_audioBuffer[i] < 0) || (m_audioBuffer[i - 1] < 0 && m_audioBuffer[i] > 0);
    }
    return static_cast<float32>(crossings) / static_cast<float32>(n);
}

std::string AudioEngine::EstimateMusicalKey() {
    float32 c = CalculateSpectralCentroid();
    if (c <= 0.0f) return "Unknown";
    float64 midi = 69.0 + 12.0 * std::log2(static_cast<float64>(c) / 440.0);
    int note = static_cast<int>(std::round(midi)) % 12;
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    std::string pc = names[(note + 12) % 12];
    return pc + "m";
}

float32 AudioEngine::CalculateEnergy(const std::vector<float32>& spectrum) {
    float64 e = 0.0;
    for (float32 v : spectrum) e += static_cast<float64>(v);
    return static_cast<float32>(e);
}

bool AudioEngine::IsLocalMaximum(const std::vector<float32>& values, uint32_t index) {
    if (index == 0 || index + 1 >= values.size()) return false;
    return values[index] > values[index - 1] && values[index] > values[index + 1];
}

void AudioEngine::DetectBeat() {
    float32 e = CalculateEnergy(m_spectrumMagnitudes);
    static float32 avg = 0.0f;
    avg = 0.98f * avg + 0.02f * e;
    if (e > avg * 1.15f) {
        m_beatDetected.store(true);
    } else {
        m_beatDetected.store(false);
    }
}

void AudioEngine::AnalyzeMusicalContent() {
    m_currentVolume = CalculateVolume(m_audioBuffer);
    m_spectralCentroid = CalculateSpectralCentroid();
    m_zeroCrossingRate = CalculateZeroCrossingRate();
    m_musicalKey = EstimateMusicalKey();
}

void AudioEngine::InitializeFFT() {}
void AudioEngine::CleanupFFT() {}

Result AudioEngine::SetThreadPriority() {
    m_avrtHandle = AvSetMmThreadCharacteristicsA("Pro Audio", &m_taskIndex);
    return m_avrtHandle ? Result::Success : Result::Error;
}

Result AudioEngine::RestoreThreadPriority() {
    if (m_avrtHandle) {
        AvRevertMmThreadCharacteristics(m_avrtHandle);
        m_avrtHandle = nullptr;
    }
    return Result::Success;
}

Result AudioEngine::ProcessCapturedData() {
    UINT32 packetFrames = 0;
    HRESULT hr = m_captureClient->GetNextPacketSize(&packetFrames);
    if (FAILED(hr)) return Result::Error;
    
    while (packetFrames) {
        BYTE* data = nullptr;
        UINT32 frames = 0;
        DWORD flags = 0;
        hr = m_captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
        if (FAILED(hr)) return Result::Error;
        {
            std::lock_guard<std::mutex> lock(m_audioMutex);
            ConvertToFloat32(data, frames);
            CalculateSpectrum();
            DetectBeat();
            AnalyzeMusicalContent();
        }
        m_captureClient->ReleaseBuffer(frames);
        hr = m_captureClient->GetNextPacketSize(&packetFrames);
        if (FAILED(hr)) return Result::Error;
    }
    
    return Result::Success;
}

} // namespace NeonGlyph
