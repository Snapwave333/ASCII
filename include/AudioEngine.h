#pragma once

#include "NeonGlyph.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace NeonGlyph {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    Result Initialize(const Config& config);
    void Shutdown();
    
    // Audio capture
    Result StartCapture();
    Result StopCapture();
    bool IsCapturing() const { return m_isCapturing; }
    
    // Audio format
    AudioFormat GetFormat() const { return m_format; }
    uint32 GetSampleRate() const { return m_format.sampleRate; }
    uint32 GetChannels() const { return m_format.channels; }
    
    // Audio processing
    Result ProcessAudioFrame(AudioFrame& frame);
    Result GetSpectrum(Spectrum& spectrum);
    Result GetWaveform(std::vector<float32>& waveform);
    
    // Beat detection
    bool IsBeatDetected() const { return m_beatDetected; }
    float32 GetBPM() const { return m_currentBPM; }
    float32 GetVolume() const { return m_currentVolume; }
    
    // Musical analysis
    std::string GetMusicalKey() const { return m_musicalKey; }
    float32 GetSpectralCentroid() const { return m_spectralCentroid; }
    float32 GetZeroCrossingRate() const { return m_zeroCrossingRate; }
    
    // Callback interface
    class IAudioCallback {
    public:
        virtual ~IAudioCallback() = default;
        virtual void OnAudioFrame(const AudioFrame& frame) = 0;
        virtual void OnBeatDetected(float32 bpm) = 0;
        virtual void OnVolumeChanged(float32 volume) = 0;
    };
    
    void RegisterCallback(IAudioCallback* callback);
    void UnregisterCallback(IAudioCallback* callback);

private:
    // WASAPI interfaces
    IMMDeviceEnumerator* m_deviceEnumerator;
    IMMDevice* m_audioDevice;
    IAudioClient* m_audioClient;
    IAudioCaptureClient* m_captureClient;
    
    // Audio format and state
    AudioFormat m_format;
    WAVEFORMATEX* m_waveFormat;
    bool m_isCapturing;
    
    // Audio processing
    std::vector<float32> m_audioBuffer;
    std::vector<float32> m_fftInput;
    std::vector<float32> m_fftOutput;
    std::vector<float32> m_spectrumMagnitudes;
    std::vector<float32> m_spectrumPhases;
    
    // Beat detection
    std::atomic<bool> m_beatDetected;
    float32 m_currentBPM;
    float32 m_currentVolume;
    std::string m_musicalKey;
    float32 m_spectralCentroid;
    float32 m_zeroCrossingRate;
    
    // Threading
    std::thread m_captureThread;
    std::atomic<bool> m_shouldStop;
    std::mutex m_audioMutex;
    std::condition_variable m_audioCondition;
    
    // Callbacks
    std::vector<IAudioCallback*> m_callbacks;
    std::mutex m_callbackMutex;
    
    // Performance
    HANDLE m_avrtHandle;
    DWORD m_taskIndex;
    
    // Configuration
    Config m_config;
    
    // Private methods
    Result InitializeWASAPI();
    Result StartCaptureThread();
    void CaptureThreadFunc();
    Result ProcessCapturedData();
    
    // Audio processing
    void ConvertToFloat32(const uint8_t* data, uint32_t frameCount);
    void ApplyWindowFunction();
    void PerformFFT();
    void CalculateSpectrum();
    void DetectBeat();
    void AnalyzeMusicalContent();
    float32 CalculateVolume(const std::vector<float32>& audioData);
    float32 CalculateSpectralCentroid();
    float32 CalculateZeroCrossingRate();
    std::string EstimateMusicalKey();
    
    // FFT utilities
    void InitializeFFT();
    void CleanupFFT();
    
    // Beat detection algorithms
    float32 CalculateEnergy(const std::vector<float32>& spectrum);
    bool IsLocalMaximum(const std::vector<float32>& values, uint32_t index);
    
    // Thread priority
    Result SetThreadPriority();
    Result RestoreThreadPriority();
};

} // namespace NeonGlyph