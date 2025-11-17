#pragma once

#include "NeonGlyph.h"
#include "DirectorClient.h"
#include "DirectorTypes.h"
#if NEONGLYPH_HAVE_GLFW
#include "VulkanOptimizedContext.h"
#endif
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>

namespace NeonGlyph {

class Application {
public:
    Application();
    ~Application();

    Result Run();
    void RequestExit();
    Result ParseCommandLineArgs(int argc, char* argv[]);

private:
    std::unique_ptr<class Window> m_window;
    std::unique_ptr<class VulkanContext> m_vulkanContext;
    std::unique_ptr<class AudioEngine> m_audioEngine;
    std::unique_ptr<class ASCIIConverter> m_asciiConverter;
    std::unique_ptr<class AIConductor> m_aiConductor;
    std::unique_ptr<class MusicAnalyzer> m_musicAnalyzer;
    std::unique_ptr<class AIDirector> m_aiDirector;
    std::unique_ptr<class Renderer> m_renderer;
    std::unique_ptr<class ConfigManager> m_configManager;
    std::unique_ptr<class SafetyManager> m_safetyManager;
    std::unique_ptr<class OutputManager> m_outputManager;
    std::unique_ptr<DirectorClient> m_directorClient;
    NeonGlyph::DirectorDirective m_lastDirective;
    std::thread m_directorWorkerThread;
    std::queue<DirectorStateSnapshot> m_stateSnapshotQueue;
    std::mutex m_snapshotQueueMutex;
    std::atomic<bool> m_directorEnabled{false};

    std::atomic<bool> m_shouldExit;
    std::atomic<bool> m_isRunning;
    std::chrono::steady_clock::time_point m_startTime;

    // Headless mode configuration
    bool m_headlessMode = false;
    bool m_forceHeadless = false;
    bool m_fallbackEnabled = true;
    std::string m_headlessReason;
    std::chrono::steady_clock::time_point m_headlessActivationTime;

    PerformanceMetrics m_performanceMetrics;
    std::vector<float32> m_frameTimeHistory;
    uint32_t m_frameCount;

    Config m_config;
    std::vector<ColorPalette> m_palettes;
    std::string m_currentCharset;

    void SwitchPalette(const std::string& paletteName);
    void SwitchCharset(const std::string& charsetName);
    ColorPalette* GetPalette(const std::string& name);
    std::string GetCharset(const std::string& name);

    Result InitializeSystems();
    Result InitializeWindow();
    Result InitializeVulkan();
    Result InitializeAudio();
    Result InitializeASCII();
    Result InitializeAI();
    Result InitializeConfig();
    Result InitializeSafety();

    // Enhanced initialization methods
    Result InitializeWindowWithFallback();
    Result InitializeHeadlessMode();
    void LogHeadlessModeActivation(const std::string& reason);
    bool ShouldUseHeadlessMode() const;
    bool IsHeadlessRequested() const { return m_forceHeadless || m_config.headless.enabled; }
    bool IsHeadlessActive() const { return m_headlessMode; }

    void MainLoop();
    void ProcessInput();
    void Update(float32 deltaTime);
    void Render();

    Result BeginFrame();
    Result EndFrame();
    void CalculateDeltaTime();
    void UpdatePerformanceMetrics();

    void OnWindowResize(uint32_t width, uint32_t height);
    void OnKeyPress(int key);
    void OnAudioFrame(const AudioFrame& frame);
    void OnBeatDetected(float32 bpm);

    void ShutdownSystems();
    void CleanupResources();

    void LogSystemInfo();
    void LogPerformanceMetrics();
    void LogSafetyViolation(const std::string& violation);

    void HandleError(Result result, const std::string& message);
    bool ShouldContinueAfterError(Result result);

    // Director integration
    void InitializeDirector();
    void ShutdownDirector();
    void DirectorWorkerLoop();
    void SendStateToDirector();
    void ProcessDirectorDirectives();

    bool m_morphSequenceActive = false;
    size_t m_morphIndex = 0;
    std::chrono::steady_clock::time_point m_lastMorphTick;
    std::vector<std::string> m_morphTechniques;
    bool m_colorTestActive = false;
    size_t m_colorIndex = 0;
    std::chrono::steady_clock::time_point m_lastColorTick;
    std::vector<std::string> m_colorModes;
    std::chrono::steady_clock::time_point m_lastPerfLog;
    float32 m_prevFrameTimeMs = 0.0f;
    uint64 m_droppedFrames = 0;
    bool m_metricsEnabled = false;
    std::string m_metricsPath;
    std::ofstream m_metrics;
    bool m_prevF1Pressed = false;
    bool m_prevF11Pressed = false;
    float32 m_audioRms = 0.0f;
    float32 m_audioPeak = 0.0f;
    float32 m_audioBass = 0.0f;
    float32 m_audioMids = 0.0f;
    float32 m_audioHighs = 0.0f;
};

} // namespace NeonGlyph
