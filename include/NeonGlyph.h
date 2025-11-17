#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Platform detection
#ifdef _WIN32
    #define NEONGLYPH_PLATFORM_WINDOWS
    #define NEONGLYPH_EXPORT __declspec(dllexport)
    #define NEONGLYPH_IMPORT __declspec(dllimport)
#else
    #define NEONGLYPH_EXPORT __attribute__((visibility("default")))
    #define NEONGLYPH_IMPORT
#endif

// Build configuration
#ifdef NEONGLYPH_BUILD_DLL
    #define NEONGLYPH_API NEONGLYPH_EXPORT
#else
    #define NEONGLYPH_API NEONGLYPH_IMPORT
#endif

// Debug utilities
#ifdef _DEBUG
    #define NEONGLYPH_DEBUG 1
    #define NEONGLYPH_ASSERT(x) if(!(x)) { __debugbreak(); }
#else
    #define NEONGLYPH_DEBUG 0
    #define NEONGLYPH_ASSERT(x)
#endif

#ifndef NEONGLYPH_HAVE_GLFW
#define NEONGLYPH_HAVE_GLFW 1
#endif

// Performance profiling
#define NEONGLYPH_PROFILE 1

namespace NeonGlyph {

// Forward declarations
class Application;
class Window;
class VulkanContext;
class AudioEngine;
class ASCIIConverter;
class AIConductor;
class SafetyManager;


// Core types
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;

// Time types
using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::microseconds;

// Constants
constexpr uint32 MAX_ASCII_CHARS = 256;
constexpr uint32 MAX_PALETTE_COLORS = 16;
constexpr uint32 MAX_AUDIO_CHANNELS = 2;
constexpr uint32 MAX_AUDIO_SAMPLE_RATE = 48000;
constexpr uint32 MAX_FFT_BINS = 2048;
constexpr uint32 MAX_TEXTURE_SIZE = 8192;
constexpr uint32 TARGET_FPS = 144;
constexpr uint32 TARGET_FRAME_TIME_MS = 1000 / TARGET_FPS;

// Error codes
enum class Result : uint32 {
    Success = 0,
    Error = 1,
    InvalidArgument = 2,
    OutOfMemory = 3,
    NotImplemented = 4,
    Timeout = 5,
    DeviceLost = 6,
    InitializationFailed = 7,
    AlreadyInitialized = 8,
    FileNotFound = 8,
    PermissionDenied = 9,
    NetworkError = 10,
    ValidationFailed = 11,
    UnsupportedOperation = 12,
    NotInitialized = 13
};

// Log levels
enum class LogLevel : uint32 {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

// Audio format
struct AudioFormat {
    uint32 sampleRate;
    uint32 channels;
    uint32 bitsPerSample;
    uint32 frameSize;
    
    bool IsValid() const {
        return sampleRate > 0 && channels > 0 && 
               bitsPerSample > 0 && (bitsPerSample % 8) == 0;
    }
};

// Audio frame
struct AudioFrame {
    float32* data;
    uint32 frameCount;
    uint32 channelCount;
    uint64 timestamp;
    
    AudioFrame() : data(nullptr), frameCount(0), channelCount(0), timestamp(0) {}
    AudioFrame(float32* d, uint32 fc, uint32 cc, uint64 ts) 
        : data(d), frameCount(fc), channelCount(cc), timestamp(ts) {}
};

// FFT spectrum
struct Spectrum {
    float32* magnitudes;
    float32* phases;
    uint32 binCount;
    float32 frequencyResolution;
    
    Spectrum() : magnitudes(nullptr), phases(nullptr), binCount(0), frequencyResolution(0.0f) {}
};

// ASCII character mapping
struct ASCIIMapping {
    char character;
    float32 density;
    uint32 colorIndex;
    
    ASCIIMapping() : character(' '), density(0.0f), colorIndex(0) {}
    ASCIIMapping(char c, float32 d, uint32 ci) 
        : character(c), density(d), colorIndex(ci) {}
};

// Color palette
struct ColorPalette {
    std::string name;
    std::vector<uint32> colors; // RGBA format
    
    ColorPalette() = default;
    ColorPalette(const std::string& n, const std::vector<uint32>& c) 
        : name(n), colors(c) {}
    
    uint32 GetColor(uint32 index) const {
        if (index < colors.size()) return colors[index];
        return 0xFFFFFFFF; // White fallback
    }
};

struct RichPalette {
    std::string name;
    std::vector<std::string> role_tags;
    std::vector<std::string> mood_tags;
    std::vector<std::string> scene_tags;
    std::string temperature;
    std::string energy;
    std::string brightness;
    uint32 primary = 0xFFFFFFFFu;
    uint32 secondary = 0xFFFFFFFFu;
    uint32 secondary2 = 0xFFFFFFFFu;
    uint32 accent = 0xFFFFFFFFu;
    uint32 shadow = 0xFF202020u;
    uint32 highlight = 0xFFFFFFFFu;
};

// Configuration
struct Config {
    struct {
        uint32 width = 1920;
        uint32 height = 1080;
        bool fullscreen = false;
        bool borderless = false;
        bool vsync = false;
        uint32 targetFPS = TARGET_FPS;
        bool disableTray = false;
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
    } llm;
    
    struct {
        bool autoStart = false;
        bool minimizeToTray = true;
        bool startMinimized = false;
    } startup;
    struct {
        bool enabled = false;           // Force headless mode
        bool fallback = true;           // Enable automatic fallback to headless
        bool logActivation = true;    // Log when headless mode is activated
        std::string activationReason;   // Reason for headless activation
    } headless;
    struct {
        uint32 startupBgColor = 0x00000000u;
        bool blackStartup = true;
        bool overlayEnabled = true;
        std::string overlayPosition = "top_left";
    } render;
};

struct OverlayData {
    bool enabled = true;
    std::string position = "top_left";
    float32 frameMs = 0.0f;
    float32 renderMs = 0.0f;
    float32 presentMs = 0.0f;
    float32 jitter = 0.0f;
    uint32 droppedFrames = 0;
    uint32 fps = TARGET_FPS;
    float32 audioRms = 0.0f;
    float32 audioPeak = 0.0f;
    float32 audioBass = 0.0f;
    float32 audioMids = 0.0f;
    float32 audioHighs = 0.0f;
};

class ConfigManager {
public:
    Result LoadConfig(const std::string& filename, Config& config);
    Result SaveConfig(const std::string& filename, const Config& config);
    Result LoadDefaultConfig(Config& config);
    Result LoadPaletteConfig(const std::string& filename, std::vector<ColorPalette>& palettes);
    Result SavePaletteConfig(const std::string& filename, const std::vector<ColorPalette>& palettes);
    std::vector<ColorPalette> GetDefaultPalettes();
    Result ValidateConfig(const Config& config, std::string& message);
private:
    Result ParseConfigFile(const std::string& content, Config& config);
    std::string SerializeConfig(const Config& config);
    Result ParsePaletteConfig(const std::string& content, std::vector<ColorPalette>& palettes);
    std::string SerializePaletteConfig(const std::vector<ColorPalette>& palettes);
    std::string FindConfigFile(const std::string& filename);
};

// Performance metrics
struct PerformanceMetrics {
    float32 frameTimeMs;
    float32 cpuUsage;
    uint32 drawCalls;
    uint32 memoryUsageMB;
    uint32 audioLatencyMs;
    float32 gpuUtilization;
    
    PerformanceMetrics() 
        : frameTimeMs(0.0f), cpuUsage(0.0f), drawCalls(0), 
          memoryUsageMB(0), audioLatencyMs(0), gpuUtilization(0.0f) {}
};

} // namespace NeonGlyph
