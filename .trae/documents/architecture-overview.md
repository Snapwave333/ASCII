# NeonGlyph Architecture Overview

**Document Type**: Architecture Document
**Version**: 2.1.0
**Last Updated**: 2025-11-13
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This document provides a comprehensive overview of the NeonGlyph system architecture, including component design, data flow, and technical implementation details.

## System Architecture

### High-Level Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Audio Input   │───▶│  AI Director    │───▶│  ASCII Output   │
│  (WASAPI)       │    │  (ONNX/DirectML)│    │  (Vulkan)       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Music Analyzer │    │ Director Command│    │  Spout/NDI      │
│  (FFT/Beat)     │    │  Processing     │    │  Broadcasting   │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Core Components

#### 1. Audio Engine (WASAPI)
- **Purpose**: Low-latency audio capture and processing
- **Technology**: Windows Audio Session API (WASAPI)
- **Features**: Loopback recording, real-time analysis
- **Latency**: <16ms audio-to-visual pipeline
- **Thread Priority**: Real-time audio thread with AVRT support

#### 2. Music Analyzer
- **Purpose**: Extract musical features from audio stream
- **Techniques**: 
  - Fast Fourier Transform (FFT) for frequency analysis
  - Beat detection and tempo estimation
  - Spectral analysis for genre classification
  - Onset detection for visual synchronization
- **Integration**: Direct interface with AI Conductor for real-time analysis

#### 3. AI Director
- **Purpose**: Intelligent scene composition and visual direction
- **Technology**: ONNX Runtime with DirectML acceleration (optional)
- **Capabilities**:
  - Real-time music genre classification
  - Scene transition logic
  - Visual effect parameter generation
  - Safety filtering (epilepsy protection)
- **Fallback**: CPU-based inference when GPU unavailable

#### 4. AI Conductor (New Component)
- **Purpose**: Advanced AI-driven music analysis and scene orchestration with psychedelic visual generation
- **Technology**: LLM integration via Ollama API with random psychedelic triggers
- **Capabilities**:
  - Advanced musical analysis and interpretation
  - Context-aware scene generation
  - Real-time adaptation to musical changes
  - Enhanced safety protocols with epilepsy protection
  - **NEW**: Psychedelic DMT/acid trip visual generation (35-50% probability)
  - **NEW**: Fractal patterns and sacred geometry synthesis
  - **NEW**: Consciousness-expanding visual directives
- **Configuration**: Configurable latency limits and safety modes, psychedelic trigger probability

#### 5. ASCII Converter
- **Purpose**: Convert visual data to ASCII art representation
- **Technology**: Vulkan compute shaders with CPU fallback
- **Features**:
  - Real-time pixel-to-character mapping
  - Configurable character sets
  - Brightness and contrast adjustment
  - Color palette application
- **Font Support**: Optional FreeType integration for custom fonts

#### 6. Renderer
- **Purpose**: High-level frame composition and ASCII rendering control
- **Technology**: Vulkan API via `VulkanContext`, compute shaders for conversion
- **Capabilities**:
  - 8K resolution support (7680×4320)
  - 60+ FPS rendering performance
  - Instanced/compute-driven ASCII operations
  - Text rendering, maze generation, fluid sim hooks
- **Canvas System**: ASCII canvas with dynamic resizing

#### 7. Output Manager
- **Purpose**: Multi-format output and broadcasting
- **Technologies**: Spout 2, NDI 5 (optional integration)
- **Features**:
  - Zero-latency texture sharing
  - OBS Studio integration
  - Resolume compatibility
  - Network streaming capabilities
- **Status**: Framework ready, integration pending

#### 8. SeamlessWindow Engine (New Component)
- **Purpose**: Complete borderless implementation with zero OS chrome
- **Technology**: Windows API with WS_POPUP style implementation
- **Features**:
  - Universal monitor support with automatic detection
  - Perfect pixel coverage with exact screen dimension positioning
  - Mathematical character density calculation for optimal ASCII rendering
  - 60 FPS performance with <800 draw calls
  - Zero OS chrome using WS_POPUP | WS_VISIBLE minimal style
  - Multi-display adaptation from 1024x768 to 8K resolutions
- **Implementation**: Native Windows GDI32/User32 API integration
- **Performance**: <2ms window creation, <1ms mode switching

#### 9. Cyberpunk Visual System (New Component)
- **Purpose**: Generate flowing @ symbol patterns with neon gradients and audio reactivity
- **Technology**: Mathematical wave function generation with real-time audio analysis
- **Capabilities**:
  - Four-wave pattern synthesis (wave1-4 combination algorithms)
  - Hue-based neon gradient system (Cyan→Purple→Green→Cyan transitions)
  - Audio reactivity with 0.4-0.7 sensitivity range
  - Mathematical character density mapping to ASCII set "@#S%?*+;:,. "
  - Brightness variation for depth perception (0.6-1.0 range)
- **Frame Rate**: 60 FPS with 0.016ms timing precision
- **Character Placement**: Exact pixel positioning with center anchor alignment

#### 10. Complete Control Integration (New Component)
- **Purpose**: Seamless keyboard control with invisible UI
- **Technology**: Global keyboard hook with modifier key detection
- **Control Scheme**:
  - **ESC**: Immediate exit with clean shutdown sequence
  - **Double-click**: Mode confirmation and user feedback
  - **Alt+B**: AI Break functionality
  - **Alt+S**: Stop AI Show functionality
  - **Alt+P**: Start AI Show functionality
- **Implementation**: GLFW keyboard callbacks with Windows message processing
- **Response Time**: <50ms control response with thread-safe execution

## Data Flow Architecture

### Real-Time Processing Pipeline

```
Audio Buffer (48kHz) → FFT Analysis (1024 samples) → Feature Extraction → AI Conductor Analysis → Director Command → ASCII Conversion → Vulkan Rendering → Output Broadcast
     ↓                        ↓                      ↓              ↓                    ↓              ↓                    ↓
WASAPI Loopback → Music Analyzer → ONNX Director → LLM Context → ASCII Canvas → Compute Shaders → Spout/NDI
```

### Processing Stages

1. **Audio Capture** (1ms)
   - WASAPI loopback recording with AVRT priority
   - 48kHz, 16-bit stereo (configurable)
   - Circular buffer management with thread safety
   - Real-time thread priority for minimal latency

2. **Frequency Analysis** (2ms)
   - 1024-point FFT with configurable window functions
   - Hanning window application
   - Magnitude spectrum calculation
   - Real-time spectrum updates at 60+ FPS

3. **Feature Extraction** (1ms)
   - Spectral centroid for brightness analysis
   - Spectral rolloff for frequency distribution
   - Zero crossing rate for noisiness detection
   - RMS energy for volume tracking
   - Beat detection and tempo estimation

4. **AI Processing** (5ms)
   - **AI Conductor**: Advanced LLM-based musical analysis
   - **AI Director**: ONNX model inference for genre classification
   - Scene parameter generation with context awareness
   - Safety filtering with epilepsy protection
   - Configurable latency limits (12ms default)

5. **Visual Generation** (6ms)
   - ASCII density mapping with multiple character sets
   - Dynamic canvas resizing based on resolution
   - Color palette application from theme system
   - Effect processing with beat synchronization
   - Real-time character placement optimization

6. **Rendering** (1ms)
   - Vulkan command recording with compute shaders
   - GPU-accelerated ASCII conversion
   - Frame composition with multi-layer support
   - Presentation submission with VSync options

**Total Pipeline Latency**: 16ms target (12ms with AI Conductor)

## Component Details

### Audio Engine Architecture

```cpp
class AudioEngine {
private:
    WASAPIRenderer* audio_renderer;
    CircularBuffer<float> audio_buffer;
    std::thread capture_thread;
    IAudioCallback* audio_callback;
    
public:
    Result Initialize(const Config& config);
    void Shutdown();
    Result StartCapture();
    Result StopCapture();
    Result ProcessAudioFrame(AudioFrame& frame);
    Result GetSpectrum(Spectrum& spectrum);
    AudioFeatures ExtractFeatures();
    
    // Callback interface for real-time audio events
    void RegisterCallback(IAudioCallback* callback);
    void UnregisterCallback(IAudioCallback* callback);
};
```

### AI Conductor Architecture (New)

```cpp
class AIConductor {
private:
    std::string ollama_endpoint;
    std::string model_name;
    uint32_t max_latency_ms;
    bool safe_epilepsy_mode;
    SafetyManager* safety_manager;
    
public:
    Result Initialize(const Config& config);
    void Shutdown();
    Result AnalyzeMusic(const MusicData& data, DirectorCommand& command);
    Result GenerateSceneContext(const std::string& musical_context);
    bool ValidateLatency(uint32_t latency_ms);
    void SetSafetyMode(bool enabled);
};
```

### AI Director Architecture

```cpp
class AIDirector {
private:
    ONNXSession* model_session;
    DirectorCommand current_command;
    SafetyManager safety_manager;
    
public:
    Result Initialize(const Config& config);
    void Shutdown();
    Result LoadModel(const std::string& model_path);
    DirectorCommand ProcessAudioFeatures(const AudioFeatures& features);
    bool ValidateSafety(const DirectorCommand& command);
    void UpdateScene(const DirectorCommand& command);
    DirectorState GetState() const;
};
```

### Renderer Architecture

```cpp
class Renderer {
private:
    VulkanContext* m_vulkanContext;
    ASCIIConverter* m_asciiConverter;
    Config m_config;
    std::queue<DirectorCommand> m_queue;
    
    // ASCII canvas system
    uint32 m_canvasWidth;
    uint32 m_canvasHeight;
    std::vector<char> m_canvas;
    std::string m_lastFrame;
    
public:
    Result Initialize(VulkanContext* context, const Config& config);
    void Shutdown();
    Result Submit(const DirectorCommand& cmd);
    Result Update(Duration dt);
    std::string GetFrameString() const;
    
    // Advanced ASCII generation
    Result GenerateMaze(uint32 width, uint32 height, float32 complexity);
    Result SpawnCharacter(const std::string& type, const std::string& position);
    Result AnimateFluidSim(float32 scale, float32 viscosity, float32 beat_coupling);
    Result RenderText(const std::string& text, const std::string& font, const std::string& style);
};
```

## Configuration System

### Core Configuration Structure

```cpp
struct Config {
    struct {
        uint32 width = 1920;
        uint32 height = 1080;
        bool fullscreen = false;
        bool vsync = false;
        uint32 targetFPS = TARGET_FPS; // 144 FPS default
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
};
```

### Theme Configuration

```json
{
  "theme": {
    "name": "neon_cyberpunk",
    "palette": {
      "primary": "#0FF0FC",
      "secondary": "#FF4D67",
      "background": "#0D0D0D",
      "accent": "#FFE066"
    },
    "ascii": {
      "characters": "@%#*+=-:. ",
      "brightness": 0.8,
      "contrast": 1.2,
      "fontSize": 12
    }
  }
}
```

### AI Conductor Configuration (New)

```json
{
  "llm": {
    "enabled": true,
    "endpoint": "http://host.docker.internal:11434",
    "model": "llama3",
    "max_latency_ms": 12,
    "live_interval_ms": 200,
    "safe_epilepsy": true,
    "psychedelic_mode": {
      "enabled": true,
      "trigger_probability": 42,
      "visual_elements": [
        "fractal_patterns",
        "sacred_geometries",
        "kaleidoscopic_colors",
        "reality_melting",
        "infinite_recursion",
        "entity_contact_visions"
      ]
    },
    "musical_context": {
      "genre_detection": true,
      "tempo_analysis": true,
      "mood_classification": true
    }
  }
}
```

## Safety Systems

### Epilepsy Protection

```cpp
class SafetyManager {
private:
    float luminance_threshold;
    float flash_frequency_limit;
    RollingAverage luminance_history;
    
public:
    bool ValidateFrame(const FrameData& frame);
    bool CheckFlashFrequency(float current_luminance);
    void ApplyProtection(FrameData& frame);
};
```

### Content Filtering

- **NSFW Detection**: AI-powered content analysis
- **Brightness Limiting**: Rolling average luminance clamping
- **Flash Prevention**: Temporal frequency analysis
- **Color Safety**: Photosensitive color filtering
- **Psychedelic Safety**: Consciousness-expanding visual limits with epilepsy protection
- **Reality Distortion**: Controlled reality-bending effect intensity

## Performance Optimization

### Memory Management

```cpp
class MemoryPool {
private:
    std::vector<MemoryChunk> chunks;
    std::queue<MemoryBlock*> free_blocks;
    
public:
    void* Allocate(size_t size);
    void Deallocate(void* ptr);
    void Defragment();
};
```

### Multi-threading Architecture

- **Audio Thread**: Real-time audio capture and processing
- **AI Thread**: ONNX model inference
- **Render Thread**: Vulkan command recording and submission
- **Main Thread**: UI and system coordination

### GPU Optimization

- **Compute Shaders**: Parallel ASCII conversion
- **Instanced Rendering**: Efficient character drawing
- **Texture Atlases**: Reduced draw calls
- **Command Buffers**: Batch rendering operations

## Integration Points

### Spout Integration

```cpp
class SpoutOutput {
private:
    spoutSender* sender;
    GLuint shared_texture;
    
public:
    bool Initialize(const std::string& name);
    void SendFrame(const FrameData& frame);
    void Cleanup();
};
```

### NDI Integration

```cpp
class NDIOutput {
private:
    NDIlib_send_instance_t instance;
    NDIlib_video_frame_v2_t frame;
    
public:
    bool Initialize(const std::string& name);
    void SendFrame(const FrameData& frame);
    void Cleanup();
};
```

## Security Architecture

### Input Validation

- **Audio Buffer Bounds**: Prevent overflow attacks
- **Configuration Validation**: Schema-based validation
- **Model Integrity**: ONNX model verification
- **Resource Limits**: Memory and CPU constraints

### Sandboxing

- **Process Isolation**: Separate renderer process
- **File System Access**: Restricted permissions
- **Network Access**: Limited to necessary services
- **GPU Access**: Controlled resource allocation

## Monitoring and Observability

### Metrics Collection

```cpp
class MetricsCollector {
private:
    prometheus::Registry* registry;
    prometheus::Counter* frame_counter;
    prometheus::Histogram* latency_histogram;
    
public:
    void RecordFrame();
    void RecordLatency(float milliseconds);
    void RecordError(const std::string& error_type);
};
```

### Health Checks

- **Service Health**: Component status monitoring
- **Performance Metrics**: FPS, latency, resource usage
- **Error Tracking**: Exception and crash reporting
- **Resource Monitoring**: Memory, CPU, GPU utilization

## Scalability Considerations

### Horizontal Scaling

- **Multi-GPU Support**: Vulkan multi-device rendering
- **Distributed Processing**: Network-based audio distribution
- **Load Balancing**: Multiple renderer instances
- **Service Discovery**: Dynamic component registration

### Vertical Scaling

- **CPU Optimization**: SIMD instructions, parallel processing
- **GPU Optimization**: Compute shader utilization
- **Memory Optimization**: Pool allocation, cache-friendly data structures
- **I/O Optimization**: Asynchronous operations, buffering

## Future Architecture Enhancements

### Planned Features

1. **VR Support**: 3D ASCII environments with psychedelic depth perception
2. **Multi-user Collaboration**: Shared VJ sessions with consciousness synchronization
3. **Cloud Integration**: Distributed processing for enhanced psychedelic rendering
4. **AI Model Updates**: Dynamic model loading for expanded consciousness states
5. **Plugin Architecture**: Extensible component system for third-party psychedelic effects
6. **Advanced Psychedelic Synthesis**: Machine learning-powered consciousness mapping
7. **Real-time Consciousness State Mapping**: Biofeedback integration for personalized trips

### Technical Roadmap

- **Phase 1**: Core architecture stabilization
- **Phase 2**: Performance optimization
- **Phase 3**: Advanced AI integration
- **Phase 4**: Multi-platform support
- **Phase 5**: Cloud-native architecture

## References

- [Vulkan Specification](https://www.khronos.org/registry/vulkan/specs/)
- [ONNX Runtime Documentation](https://onnxruntime.ai/docs/)
- [WASAPI Documentation](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi)
- [Spout Documentation](https://spout.zeal.co/)
- [NDI SDK Documentation](https://www.ndi.tv/sdk/)

## Change History

### Version 2.2.0 (2025-11-13)
- **MAJOR**: Added psychedelic visual generation architecture
- Enhanced AI Conductor with consciousness-expanding visual directives
- Updated configuration system with psychedelic mode settings (42% trigger probability)
- Added fractal patterns, sacred geometries, and reality-bending transformations
- Enhanced safety systems with psychedelic visual limits and epilepsy protection
- Updated future roadmap with advanced psychedelic synthesis and consciousness mapping

### Version 2.1.0 (2025-11-13)
- Added AI Conductor component with LLM integration
- Updated configuration system with LLM settings
- Enhanced audio pipeline with AVRT support
- Added ASCII canvas system details
- Updated performance targets for 144 FPS
- Added FreeType font support information

### Version 2.0.1 (2025-11-13)
- Updated Renderer section to match current class name and responsibilities
- Added alignment with `VulkanContext` and compute pipeline usage

### Version 2.0.0 (2024-11-13)
- Complete architecture documentation rewrite
- Added detailed component specifications
- Enhanced security and safety documentation
- Added scalability considerations

### Version 1.0.0 (2024-01-01)
- Initial architecture overview
- Basic component descriptions
- Data flow diagrams
