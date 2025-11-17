# NeonGlyph API Reference

**Document Type**: API Reference
**Version**: 2.1.0
**Last Updated**: 2025-11-13
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This document provides comprehensive API reference for the NeonGlyph C++ library, aligned with the current codebase. It includes class definitions, method signatures, and usage examples verified against `include/*.h`.

## Core API Classes

### AudioEngine

Manages audio capture and processing using WASAPI.

```cpp
// include/AudioEngine.h
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    Result Initialize(const Config& config);
    void Shutdown();

    // Audio capture
    Result StartCapture();
    Result StopCapture();
    bool IsCapturing() const;

    // Audio format
    AudioFormat GetFormat() const;
    uint32 GetSampleRate() const;
    uint32 GetChannels() const;

    // Audio processing
    Result ProcessAudioFrame(AudioFrame& frame);
    Result GetSpectrum(Spectrum& spectrum);
    Result GetWaveform(std::vector<float32>& waveform);

    // Beat detection and metrics
    bool IsBeatDetected() const;
    float32 GetBPM() const;
    float32 GetVolume() const;
    std::string GetMusicalKey() const;
    float32 GetSpectralCentroid() const;
    float32 GetZeroCrossingRate() const;

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
};
```

#### AudioEngine Usage Example

```cpp
#include "AudioEngine.h"

// Create and initialize audio engine
AudioEngine audio_engine;
Config cfg; // populate as needed
if (audio_engine.Initialize(cfg) != Result::Success) {
    std::cerr << "Failed to initialize audio engine" << std::endl;
    return -1;
}

// Set up audio processing callback
struct LoggerCallback : public AudioEngine::IAudioCallback {
    void OnAudioFrame(const AudioFrame& frame) override {}
    void OnBeatDetected(float32 bpm) override { std::cout << "BPM: " << bpm << "\n"; }
    void OnVolumeChanged(float32 volume) override { std::cout << "Volume: " << volume << "\n"; }
};

LoggerCallback cb;
audio_engine.RegisterCallback(&cb);

// Start audio capture
if (audio_engine.StartCapture() != Result::Success) {
    std::cerr << "Failed to start audio capture" << std::endl;
    return -1;
}

// Main application loop
while (running) {
    Spectrum spec{};
    audio_engine.GetSpectrum(spec);
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

// Cleanup
audio_engine.StopCapture();
audio_engine.Shutdown();
```

### MusicAnalyzer

Analyzes audio features for music classification and beat/section detection.

```cpp
// include/MusicAnalyzer.h
struct MusicData {
    int32 bpm;
    std::string key;
    float32 energy;
    float32 dynamics;
    std::string song_section;
    float32 beat_phase;
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
};
```

#### MusicAnalyzer Usage Example

```cpp
#include "MusicAnalyzer.h"

// Create music analyzer
MusicAnalyzer analyzer;
Config cfg;
analyzer.Initialize(cfg);

// Configure analyzer settings
analyzer.SetTempoSmoothing(0.8f);
analyzer.SetDynamicsSmoothing(0.6f);

// Analyze audio frame
AudioFrame frame{}; Spectrum spec{};
analyzer.Update(frame, spec);
MusicData md = analyzer.GetLatest();

// Use analysis results
std::cout << "BPM: " << md.bpm << ", Energy: " << md.energy << std::endl;
```

### AIDirector

AI-powered scene direction and visual effect control.

```cpp
// include/AIDirector.h
struct DirectorState { /* fields: current_scene_id, theme, palette, etc. */ };

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

    DirectorState GetState() const;
};
```

#### AIConductor

Advanced AI-driven music analysis and scene orchestration using LLM integration with psychedelic visual generation.

```cpp
// include/AIConductor.h
class AIConductor {
public:
    AIConductor();
    ~AIConductor();

    Result Initialize(const Config& config);
    void Shutdown();

    Result AnalyzeMusic(const MusicData& data, DirectorCommand& command);
    Result GenerateSceneContext(const std::string& musical_context);
    
    void SetOllamaEndpoint(const std::string& endpoint);
    void SetModel(const std::string& model);
    void SetMaxLatency(uint32_t max_latency_ms);
    void SetSafetyMode(bool enabled);
    
    // Psychedelic visual generation
    void SetPsychedelicMode(bool enabled);
    void SetPsychedelicProbability(uint32_t probability_percent); // 35-50% recommended
    bool ShouldTriggerPsychedelic() const;
    Result GeneratePsychedelicVisuals(DirectorCommand& command);
    
    bool ValidateLatency(uint32_t latency_ms) const;
    std::string GetCurrentModel() const;
    bool IsSafetyEnabled() const;
    bool IsPsychedelicModeEnabled() const;
    uint32_t GetPsychedelicProbability() const;
};
```

#### AIConductor Usage Example

```cpp
#include "AIConductor.h"

// Create and initialize AI conductor
AIConductor conductor;
Config cfg;
if (conductor.Initialize(cfg) != Result::Success) {
    std::cerr << "Failed to initialize AIConductor" << std::endl;
    return -1;
}

// Configure LLM settings
conductor.SetOllamaEndpoint("http://localhost:11434");
conductor.SetModel("llama3");
conductor.SetMaxLatency(12); // 12ms max latency
conductor.SetSafetyMode(true);

// Main processing loop
while (running) {
    // Get music data from analyzer
    MusicData md = music_analyzer.GetLatest();
    
    // Generate advanced scene command
    DirectorCommand command;
    if (conductor.AnalyzeMusic(md, command) == Result::Success) {
        ApplyVisualCommand(command);
    }
    
    // Monitor performance
    // Validate latency constraints
    if (!conductor.ValidateLatency(current_latency)) {
        // Fallback to AI Director
        DirectorCommand fallback_cmd = ai_director.Update(md);
        ApplyVisualCommand(fallback_cmd);
    }
}
```

### ASCIIConverter

Converts visual data to ASCII art representation (CPU and GPU paths).

```cpp
// include/ASCIIConverter.h
class ASCIIConverter {
public:
    ASCIIConverter();
    ~ASCIIConverter();

    Result Initialize(VulkanContext* vulkanContext, const Config& config);
    void Shutdown();

    // Settings
    void SetCharset(const std::string& charset);
    void SetDensityMapping(const std::vector<float32>& densityMap);
    void SetColorPalette(const ColorPalette& palette);

    // Conversion
    Result ConvertFrame(const uint8_t* inputTexture, uint32_t width, uint32_t height,
                        ASCIIMapping* outputBuffer, uint32_t bufferSize);
    Result ConvertFrameGPU(VkImage inputImage, uint32_t width, uint32_t height,
                           VkBuffer outputBuffer, uint32_t& outputSize);

    // Font atlas
    Result GenerateFontAtlas(uint32_t fontSize, const std::string& fontName);
    VkImageView GetFontAtlasView() const;
    VkSampler GetFontAtlasSampler() const;
};
```

#### ASCIIConverter Usage Example

```cpp
#include "ASCIIConverter.h"

ASCIIConverter converter;
converter.Initialize(vulkanContext, cfg);
converter.SetCharset(" .:-=+*#%@");
ASCIIMapping buffer[1024];
converter.ConvertFrame(rawPixels, width, height, buffer, 1024);
```

### Renderer

High-level rendering and ASCII composition control.

```cpp
// include/Renderer.h
class Renderer {
public:
    Renderer();
    ~Renderer();

    Result Initialize(VulkanContext* context, const Config& config);
    void Shutdown();

    void SetASCIIConverter(ASCIIConverter* converter);

    Result Submit(const DirectorCommand& cmd);
    Result Update(Duration dt);
    std::string GetFrameString() const;

    Result GenerateMaze(uint32 width, uint32 height, float32 complexity);
    Result SpawnCharacter(const std::string& type, const std::string& position);
    Result AnimateFluidSim(float32 scale, float32 viscosity, float32 beat_coupling);
    Result RenderText(const std::string& text, const std::string& font, const std::string& style);
};
```

#### AIDirector Usage Example

```cpp
#include "AIDirector.h"

// Create and initialize AI director
AIDirector director;
Config cfg;
if (director.Initialize(cfg) != Result::Success) {
    std::cerr << "Failed to initialize AIDirector" << std::endl;
    return -1;
}

// Configure safety settings
director.SetSafetyEnabled(true);

// Main processing loop
while (running) {
    // Get music data from analyzer
    MusicData md = music_analyzer.GetLatest();
    
    // Generate command
    DirectorCommand command = director.Update(md);
    ApplyVisualCommand(command);
    
    // Monitor performance
    // monitor app-level performance metrics
}
```

### Renderer Usage Example

```cpp
#include "Renderer.h"

Renderer renderer;
renderer.Initialize(vulkanContext, cfg);
renderer.SetASCIIConverter(&converter);

// Submit director command
DirectorCommand cmd;
cmd.type = CommandType::Text;
cmd.text = "Hello NeonGlyph!";
cmd.position = "center";
cmd.style = "neon";

renderer.Submit(cmd);
renderer.Update(std::chrono::microseconds(16000));
std::cout << renderer.GetFrameString() << std::endl;

// Generate advanced ASCII effects
renderer.GenerateMaze(80, 40, 0.7f);
renderer.AnimateFluidSim(1.0f, 0.5f, 0.8f);
renderer.RenderText("ASCII Art", "Consolas", "neon");
```

### OutputManager

Manages Spout and NDI outputs.

```cpp
// include/OutputManager.h
class OutputManager {
public:
    OutputManager();
    ~OutputManager();

    Result Initialize(const Config& config);
    void Shutdown();

    Result SendFrame(VkImage vulkanImage, uint32_t width, uint32_t height);

    Result EnableSpout(const std::string& senderName);
    Result DisableSpout();
    Result EnableNDI(const std::string& senderName);
    Result DisableNDI();

    bool IsSpoutEnabled() const;
    bool IsNDIEnabled() const;
    bool IsSpoutConnected() const;
    bool IsNDIConnected() const;

    void UpdatePerformanceMetrics();
    std::string GetStatusString() const;
};
```

### SeamlessWindow

Complete borderless window implementation with zero OS chrome.

```cpp
// include/SeamlessWindow.h
enum class DisplayMode {
    Windowed,
    Borderless,
    Fullscreen,
    Seamless  // True borderless with no OS chrome
};

enum class ScalingMode {
    Stretch,      // Fill entire screen
    AspectRatio,  // Maintain aspect ratio with black bars
    PixelPerfect, // Integer scaling only
    Adaptive      // Smart scaling based on content
};

class SeamlessWindow {
public:
    SeamlessWindow();
    ~SeamlessWindow();

    // Window creation and management
    Result Create(const std::string& title, int width, int height, bool borderless = true);
    void Destroy();
    
    // Display mode management
    Result SetDisplayMode(DisplayMode mode);
    DisplayMode GetDisplayMode() const;
    
    // Seamless scaling
    Result SetScalingMode(ScalingMode mode);
    ScalingMode GetScalingMode() const;
    
    // Perfect borderless implementation
    Result EnterSeamlessMode();
    Result EnterTrueFullscreen();
    Result EnterBorderlessWindowed();
    Result ExitSeamlessMode();
    
    // Monitor detection and adaptation
    struct MonitorInfo {
        int width, height;
        int refreshRate;
        float aspectRatio;
        bool isPrimary;
        std::string name;
    };
    
    std::vector<MonitorInfo> GetMonitors() const;
    Result SetMonitor(int monitorIndex);
    MonitorInfo GetCurrentMonitor() const;
    
    // Seamless scaling calculations
    struct ScalingInfo {
        int targetWidth, targetHeight;
        int sourceWidth, sourceHeight;
        float scaleX, scaleY;
        int offsetX, offsetY;
        bool letterbox;
    };
    
    ScalingInfo CalculateOptimalScaling(int sourceWidth, int sourceHeight) const;
    
    // Window properties
    void SetTitle(const std::string& title);
    std::string GetTitle() const;
    
    void SetPosition(int x, int y);
    void GetPosition(int& x, int& y) const;
    
    void SetSize(int width, int height);
    void GetSize(int& width, int& height) const;
    
    // Native handle access
    void* GetNativeHandle() const;
    void* GetDisplayHandle() const;
    
    // Event handling
    void PollEvents();
    bool ShouldClose() const;
    
    // Seamless input handling
    void SetKeyboardCallback(std::function<void(int key, int scancode, int action, int mods)> callback);
    void SetMouseCallback(std::function<void(double x, double y)> callback);
    void SetResizeCallback(std::function<void(int width, int height)> callback);
    
    // Visual purity - remove all OS chrome
    Result RemoveAllBorders();
    Result DisableWindowDecorations();
    Result SetTransparentBackground();
    Result EnableClickThrough(bool enable);
    
    // Performance optimization
    Result EnableVSync(bool enable);
    Result SetRefreshRate(int refreshRate);
    Result EnableTripleBuffer(bool enable);
    
    // Debug and info
    void GetDebugInfo(std::string& info) const;
    bool IsSeamless() const;
    bool IsFullscreen() const;
};
```

### CyberpunkPatternGenerator

Generates flowing @ symbol patterns with neon gradients and audio reactivity.

```cpp
// include/CyberpunkPatternGenerator.h
class CyberpunkPatternGenerator {
public:
    CyberpunkPatternGenerator();
    ~CyberpunkPatternGenerator();

    Result Initialize(const Config& config);
    void Shutdown();

    // Pattern generation
    char GeneratePatternCharacter(int x, int y, float time, float audioReactivity = 0.6f);
    uint32_t GenerateNeonColor(int x, int y, float time, float audioReactivity = 0.6f);
    float CalculatePatternIntensity(int x, int y, float time, float audioReactivity = 0.6f);

    // Wave function control
    void SetWaveFrequency(int waveIndex, float frequency);
    void SetWaveAmplitude(int waveIndex, float amplitude);
    void SetWavePhase(int waveIndex, float phase);
    
    // Audio reactivity
    void SetAudioReactivity(float reactivity);
    void SetAudioSensitivity(float sensitivity);
    float GetCurrentAudioReactivity() const;

    // Color palette
    void SetColorPalette(const std::string& paletteName); // "cyberpunk", "neon", "matrix"
    void SetCustomColors(uint32_t primary, uint32_t secondary, uint32_t accent);
    
    // Mathematical parameters
    void SetCharacterSet(const std::string& charset); // "@#S%?*+;:,. "
    void SetMinimumDensity(int minX, int minY); // Default: 100x30
    void SetScalingMode(const std::string& mode); // "adaptive", "stretch", "aspect", "pixelperfect"

    // Performance
    void SetTargetFPS(float fps); // Default: 60.0f
    void EnableFrameTiming(bool enable);
    float GetCurrentFPS() const;
    float GetFrameTime() const;

private:
    // Internal wave functions
    float WaveFunction1(float x, float y, float time);
    float WaveFunction2(float x, float y, float time);
    float WaveFunction3(float x, float y, float time);
    float WaveFunction4(float x, float y, float time);
    
    // Color generation
    uint32_t HSVToRGB(float h, float s, float v);
    uint32_t InterpolateColor(uint32_t color1, uint32_t color2, float factor);
};
```

#### OutputManager Usage Example

```cpp
#include "OutputManager.h"

OutputManager outputManager;
outputManager.Initialize(cfg);
outputManager.EnableSpout("NeonGlyph");
outputManager.SendFrame(vkImage, width, height);
outputManager.Shutdown();
```

## Core Types

### AudioFormat, AudioFrame, Spectrum

```cpp
// include/NeonGlyph.h
struct AudioFormat { 
    uint32 sampleRate; 
    uint32 channels; 
    uint32 bitsPerSample; 
    uint32 frameSize; 
    
    bool IsValid() const;
};

struct AudioFrame { 
    float32* data; 
    uint32 frameCount; 
    uint32 channelCount; 
    uint64 timestamp; 
    
    AudioFrame();
    AudioFrame(float32* d, uint32 fc, uint32 cc, uint64 ts);
};

struct Spectrum { 
    float32* magnitudes; 
    float32* phases; 
    uint32 binCount; 
    float32 frequencyResolution; 
    
    Spectrum();
};
```

### DirectorCommand

```cpp
// include/DirectorCommand.h (see repository)
// Contains composition, motion, transition, safety flags and TTL/priority fields.
// Enhanced with AI Conductor context and LLM-generated parameters
```

### ASCIIMapping and ColorPalette

```cpp
// include/NeonGlyph.h
struct ASCIIMapping { 
    char character; 
    float32 density; 
    uint32 colorIndex; 
    
    ASCIIMapping();
    ASCIIMapping(char c, float32 d, uint32 ci);
};

struct ColorPalette { 
    std::string name; 
    std::vector<uint32> colors; // RGBA format
    
    ColorPalette();
    ColorPalette(const std::string& n, const std::vector<uint32>& c);
    uint32 GetColor(uint32 index) const;
};
```

### FrameData

```cpp
struct FrameData {
    uint32_t width;           // Frame width in pixels
    uint32_t height;          // Frame height in pixels
    std::vector<uint8_t> pixel_data; // RGBA pixel data
    float timestamp;          // Frame timestamp in seconds
    uint32_t frame_number;    // Sequential frame number
    
    // Utility methods
    Color GetPixel(uint32_t x, uint32_t y) const;
    void SetPixel(uint32_t x, uint32_t y, const Color& color);
};
```

### PerformanceMetrics

```cpp
// include/NeonGlyph.h
struct PerformanceMetrics {
    float32 frameTimeMs;
    float32 cpuUsage;
    uint32 drawCalls;
    uint32 memoryUsageMB;
    uint32 audioLatencyMs;
    float32 gpuUtilization;
    
    PerformanceMetrics();
};
```

## Configuration Structures

```cpp
// include/NeonGlyph.h
struct Config {
    struct {
        uint32 width = 1920;
        uint32 height = 1080;
        bool fullscreen = false;
        bool vsync = false;
        uint32 targetFPS = TARGET_FPS; // 144 FPS
    } window;
    
    struct {
        uint32 sampleRate = 48000;
        uint32 channels = 2;
        uint32 bufferSize = 1024;
        std::string deviceId = "default";
    } audio;
    
    struct {
        std::string charset = "@%#*+=-:. ";
        uint32 fontSize = 12;
        float32 brightness = 1.0f;
        float32 contrast = 1.0f;
        bool invert = false;
    } ascii;
    
    struct {
        bool enabled = true;
        float32 sensitivity = 0.5f;
        std::string modelPath = "models/genre_classifier.onnx";
    } ai;
    
    struct {
        bool photosensitiveMode = true;
        float32 luminanceThreshold = 0.3f;
        bool nsfwFilter = true;
        bool crashRecovery = true;
    } safety;
    
    struct {
        bool spoutEnabled = true;
        bool ndiEnabled = false;
        std::string spoutName = "NeonGlyph";
        std::string ndiName = "NeonGlyph Output";
    } output;
    
    struct {
        bool enabled = false;
        std::string endpoint = "http://host.docker.internal:11434";
        std::string model = "llama3";
        uint32 maxLatencyMs = 12;
        uint32 liveIntervalMs = 200;
        bool safeEpilepsy = true;
        struct {
            bool enabled = true;
            uint32 triggerProbability = 42; // 35-50% recommended
            bool fractalPatterns = true;
            bool sacredGeometries = true;
            bool kaleidoscopicColors = true;
            bool realityMelting = true;
            bool infiniteRecursion = true;
            bool entityContactVisions = true;
        } psychedelicMode;
    } llm;
};
```

## Utilities

See `include/NeonGlyph.h` for `ConfigManager` helpers and performance metrics.

## Error Handling

Use `Result` return codes defined in `include/NeonGlyph.h` for API methods.

## Thread Safety

- `AudioEngine`: callback registration guarded by mutexes
- `OutputManager`: internal state guarded, send operations serialized
- `Renderer`, `ASCIIConverter`, `AIDirector`: use from the main thread unless externally synchronized

## Performance Considerations

- Avoid allocations in real-time paths
- Prefer GPU compute for conversion when available
- Monitor frame time and draw call targets

## References

- [NeonGlyph Architecture Overview](architecture-overview.md)
- [NeonGlyph Build Guide](build-guide.md)
- [NeonGlyph Configuration Guide](configuration-guide.md)

## Change History

### Version 2.2.0 (2025-11-13)
- **MAJOR**: Added psychedelic visual generation API methods to AIConductor
- Enhanced AIConductor with SetPsychedelicMode(), SetPsychedelicProbability(), ShouldTriggerPsychedelic()
- Added GeneratePsychedelicVisuals() method for consciousness-expanding visual directives
- Updated configuration structures with comprehensive psychedelic mode settings
- Added fractal patterns, sacred geometries, kaleidoscopic colors, reality melting, infinite recursion, entity contact visions
- Maintained epilepsy safety with 35-50% probability trigger recommendations

### Version 2.1.0 (2025-11-13)
- Added AIConductor API documentation with LLM integration
- Updated core types with constructor details and validation methods
- Enhanced configuration structures with complete Config struct
- Added PerformanceMetrics structure documentation
- Updated usage examples with advanced ASCII generation
- Added AI Conductor usage examples with latency validation

### Version 2.0.1 (2025-11-13)
- Aligned all class signatures with current headers
- Replaced VulkanRenderer section with Renderer
- Updated usage examples to use `Result` and `Config`
- Consolidated core types and configuration

### Version 2.0.0 (2024-11-13)
- Complete API documentation rewrite
- Added comprehensive usage examples
- Enhanced error handling documentation
- Added thread safety information

### Version 1.0.0 (2024-01-01)
- Initial API reference creation
- Basic class and method documentation
*** End of File
