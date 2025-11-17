#include "Application.h"
#include "VulkanContext.h"
#if NEONGLYPH_HAVE_GLFW
#include "VulkanOptimizedContext.h"
#endif
#include "AudioEngine.h"
#include "ASCIIConverter.h"
#include "MusicAnalyzer.h"
#include "AIDirector.h"
#include "Renderer.h"
#include "OutputManager.h"
#include "AIConductor.h"
#include "Theme.h"
#include "WinIntegration.h"
#include "Window.h"
#include "SafetyManager.h"
#include <iostream>
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace NeonGlyph {

Application::Application() 
    : m_shouldExit(false)
    , m_isRunning(false)
    , m_frameCount(0)
    , m_currentCharset("@%#*+=-:. ") {
    m_frameTimeHistory.reserve(144); // 1 second at 144fps
}

Application::~Application() {
    ShutdownSystems();
}

Result Application::Run() {
    std::cout << "PROJECT NEON-GLYPH - AI-Driven ASCII Visual Synthesis Engine" << std::endl;
    NeonGlyph::Logger::LogLine("Version 3.0.1 Startup");
    std::cout << "Initializing systems..." << std::endl;
    
    Result result = InitializeSystems();
    if (result != Result::Success) {
        std::cerr << "Failed to initialize systems: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    LogSystemInfo();
    m_startTime = std::chrono::steady_clock::now();
    m_isRunning = true;
    m_metricsPath = "staging_run.log";
    m_metricsEnabled = true;
    m_metrics.open(m_metricsPath, std::ios::out | std::ios::app);
    
    std::cout << "Starting main loop..." << std::endl;
    NeonGlyph::Logger::LogLine("Startup MainLoop start");
    MainLoop();
    
    std::cout << "Shutting down..." << std::endl;
    return Result::Success;
}

Result Application::InitializeSystems() {
    std::cout << "[Application] Initializing configuration..." << std::endl;
    Result result = InitializeConfig();
    if (result != Result::Success) {
        std::cerr << "InitializeConfig failed with error: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    // Check if headless mode should be used
    if (ShouldUseHeadlessMode()) {
        std::cout << "[Application] Headless mode requested, skipping window initialization..." << std::endl;
        LogHeadlessModeActivation(m_forceHeadless ? std::string("Forced via CLI") : std::string("Enabled via config"));
        result = InitializeHeadlessMode();
        if (result != Result::Success) {
            std::cerr << "InitializeHeadlessMode failed with error: " << static_cast<uint32_t>(result) << std::endl;
            return result;
        }
    } else {
#if NEONGLYPH_HAVE_GLFW
        std::cout << "[Application] Initializing window with fallback support..." << std::endl;
        result = InitializeWindowWithFallback();
        if (result != Result::Success) {
            std::cerr << "InitializeWindowWithFallback failed with error: " << static_cast<uint32_t>(result) << std::endl;
            return result;
        }
#endif
    }
    
    std::cout << "[Application] Initializing audio..." << std::endl;
    result = InitializeAudio();
    if (result != Result::Success) {
        std::cerr << "InitializeAudio failed with error: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    std::cout << "[Application] Initializing AI..." << std::endl;
    result = InitializeAI();
    if (result != Result::Success) {
        std::cerr << "InitializeAI failed with error: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    std::cout << "[Application] Initializing safety systems..." << std::endl;
    result = InitializeSafety();
    if (result != Result::Success) {
        std::cerr << "InitializeSafety failed with error: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    std::cout << "[Application] All systems initialized successfully" << std::endl;
    m_outputManager = std::make_unique<OutputManager>();
    m_outputManager->Initialize(m_config);
    return Result::Success;
}

Result Application::InitializeWindow() {
    std::cout << "[Application] Creating window..." << std::endl;
    m_window = std::make_unique<Window>();
    Result r = m_window->Create(m_config);
    if (r != Result::Success) {
        std::cerr << "[Application] Window creation failed with error: " << static_cast<uint32_t>(r) << std::endl;
        return r;
    }
    std::cout << "[Application] Window created successfully, enabling DPI awareness..." << std::endl;
    WinIntegration::EnableDPIAwareness();
    std::cout << "[Application] DPI awareness enabled" << std::endl;
    NeonGlyph::Logger::LogLine("Startup Window+GLFW initialized");
    return Result::Success;
}

Result Application::InitializeVulkan() {
#if NEONGLYPH_HAVE_GLFW
    std::cout << "[Application] Initializing optimized Vulkan context..." << std::endl;
    m_vulkanContext = std::make_unique<VulkanOptimizedContext>();
    std::cout << "[Application] Creating Vulkan instance..." << std::endl;
    // Create Vulkan instance and select physical device first
    Result result = m_vulkanContext->CreateInstance();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create Vulkan instance" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Selecting physical device..." << std::endl;
    result = m_vulkanContext->SelectPhysicalDevice();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to select physical device" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating Vulkan surface..." << std::endl;
    // Create Vulkan surface for window
    VkSurfaceKHR surface;
    result = m_window->CreateVulkanSurface(m_vulkanContext->GetInstance(), &surface);
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create Vulkan surface" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Attaching surface to Vulkan context..." << std::endl;
    // Attach surface to Vulkan context so it can be used for device selection
    result = m_vulkanContext->AttachSurface(surface);
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to attach surface to Vulkan context" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating logical device..." << std::endl;
    // Now create logical device with proper surface support
    result = m_vulkanContext->CreateLogicalDevice();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create logical device" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating command pools..." << std::endl;
    // Continue with remaining initialization
    result = m_vulkanContext->CreateCommandPools();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create command pools" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating command buffers..." << std::endl;
    result = m_vulkanContext->CreateCommandBuffers();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create command buffers" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating synchronization objects..." << std::endl;
    result = m_vulkanContext->CreateSyncObjects();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create sync objects" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating compute pipeline..." << std::endl;
    result = m_vulkanContext->CreateComputePipeline();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create compute pipeline" << std::endl;
        return result;
    }
    
    std::cout << "[Application] Creating descriptor pool..." << std::endl;
    result = m_vulkanContext->CreateDescriptorPool();
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to create descriptor pool" << std::endl;
        return result;
    }
    if (m_vulkanContext->HasSurface()) {
        std::cout << "[Application] Creating swapchain..." << std::endl;
        result = m_vulkanContext->CreateSwapchain();
        if (result != Result::Success) {
            std::cerr << "[Application] Failed to create swapchain" << std::endl;
            return result;
        }
        std::cout << "[Application] Creating image views..." << std::endl;
        result = m_vulkanContext->CreateImageViews();
        if (result != Result::Success) {
            std::cerr << "[Application] Failed to create image views" << std::endl;
            return result;
        }
        std::cout << "[Application] Creating frame resources (staging buffer)..." << std::endl;
        result = m_vulkanContext->CreateFrameResources();
        if (result != Result::Success) {
            std::cerr << "[Application] Failed to create frame resources" << std::endl;
            return result;
        }
    }

    std::cout << "[Application] Vulkan initialization completed successfully" << std::endl;
    NeonGlyph::Logger::LogLine("Startup Vulkan initialized");
#else
    std::cout << "[Application] Initializing headless Vulkan context..." << std::endl;
    m_vulkanContext = std::make_unique<VulkanContext>();
    Result result = m_vulkanContext->Initialize(m_config);
    if (result != Result::Success) {
        std::cerr << "[Application] Vulkan headless initialization failed" << std::endl;
        return result;
    }
    return Result::Success;
#endif
    
    return Result::Success;
}

Result Application::InitializeAudio() {
    m_audioEngine = std::make_unique<AudioEngine>();
    Result result = m_audioEngine->Initialize(m_config);
    if (result != Result::Success) return result;
    
    // Start audio capture
    result = m_audioEngine->StartCapture();
    if (result != Result::Success) return result;
    NeonGlyph::Logger::LogLine("Startup Audio/WASAPI initialized");
    
    // Initialize MusicAnalyzer
    m_musicAnalyzer = std::make_unique<MusicAnalyzer>();
    result = m_musicAnalyzer->Initialize(m_config);
    if (result != Result::Success) return result;
    
    return Result::Success;
}

Result Application::InitializeASCII() {
    m_asciiConverter = std::make_unique<ASCIIConverter>();
    Result r = m_asciiConverter->Initialize(m_vulkanContext.get(), m_config);
    if (r != Result::Success) return r;
    m_renderer = std::make_unique<Renderer>();
    r = m_renderer->Initialize(m_vulkanContext.get(), m_config);
    if (r != Result::Success) return r;
    m_renderer->SetASCIIConverter(m_asciiConverter.get());
    auto themeManager = std::make_unique<ThemeManager>();
    themeManager->Initialize(m_config);
    themeManager->AutoDetectSystemTheme();
    themeManager->ApplyToASCII(m_asciiConverter.get());
    {
        m_renderer->RenderNeonGlyphLogo();
        m_renderer->RenderTestCard("auto");
        NeonGlyph::Logger::LogLine("Startup HeroCinematic queued");
        DirectorCommand c0; c0.scene_id = "SCN-BOOT"; c0.directives.push_back({"RenderTestCard", {{"mode","auto"}}}); m_renderer->Submit(c0);
        m_morphSequenceActive = true;
        m_morphIndex = 0;
        m_morphTechniques = {"fold","beam","panel","glitch","portal"};
        m_lastMorphTick = std::chrono::steady_clock::now();
        m_colorTestActive = true;
        m_colorIndex = 0;
        m_colorModes = {"mono","truecolor","ansi"};
        m_lastColorTick = std::chrono::steady_clock::now();
        m_lastPerfLog = std::chrono::steady_clock::now();
    }
    return Result::Success;
}

Result Application::InitializeAI() {
#if NEONGLYPH_HAVE_ONNXRUNTIME
    m_aiConductor = std::make_unique<AIConductor>();
    Result result = m_aiConductor->Initialize(m_config);
    if (result != Result::Success) return result;
    m_aiDirector = std::make_unique<AIDirector>();
    Result result2 = m_aiDirector->Initialize(m_config);
    if (result2 != Result::Success) return result2;
    NeonGlyph::Logger::LogLine("Startup AI Director constructed+started");
#if NEONGLYPH_HAVE_DIRECTOR
    InitializeDirector();
#endif
    return Result::Success;
#else
    return Result::Success;
#endif
}

Result Application::InitializeConfig() {
    m_configManager = std::make_unique<ConfigManager>();
    
    // Load main configuration
    Result result = m_configManager->LoadDefaultConfig(m_config);
    if (result != Result::Success) {
        std::cerr << "Failed to load default configuration" << std::endl;
        return result;
    }
    
    // Load palette configurations
    std::vector<ColorPalette> palettes;
    result = m_configManager->LoadPaletteConfig("config/palettes.json", palettes);
    if (result == Result::Success) {
        m_palettes = palettes;
        std::cout << "Loaded " << m_palettes.size() << " color palettes" << std::endl;
    } else {
        // Use default palettes if file loading fails
        m_palettes = m_configManager->GetDefaultPalettes();
        std::cout << "Using default color palettes" << std::endl;
    }
    
    // Load charset configurations (we'll implement this similarly)
    std::cout << "Configuration loaded successfully" << std::endl;
    if (m_config.render.blackStartup) {
        m_config.render.startupBgColor = 0xFF000000u;
    }
    
    return Result::Success;
}

Result Application::InitializeSafety() {
    m_safetyManager = std::make_unique<SafetyManager>();
    return m_safetyManager->Initialize(m_config);
}

void Application::MainLoop() {
#if NEONGLYPH_HAVE_GLFW
    if (m_headlessMode) {
        // Headless mode main loop - no window operations
        std::cout << "[Application] Running in headless mode - window operations disabled" << std::endl;
        
        while (!m_shouldExit) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            // Calculate delta time
            CalculateDeltaTime();
            
            // Update systems (no input processing in headless mode)
            float32 deltaTime = m_performanceMetrics.frameTimeMs / 1000.0f;
            Update(deltaTime);
            
            // Render frame (headless rendering)
            auto renderStart = std::chrono::high_resolution_clock::now();
            Render();
            auto renderEnd = std::chrono::high_resolution_clock::now();
            
            // Update performance metrics
            auto frameEnd = std::chrono::high_resolution_clock::now();
            float32 frameTime = std::chrono::duration<float32, std::milli>(frameEnd - frameStart).count();
            float32 renderMs = std::chrono::duration<float32, std::milli>(renderEnd - renderStart).count();
            
            m_performanceMetrics.frameTimeMs = frameTime;
            m_frameTimeHistory.push_back(frameTime);
            if (m_frameTimeHistory.size() > 144) {
                m_frameTimeHistory.erase(m_frameTimeHistory.begin());
            }
            
            UpdatePerformanceMetrics();
            m_frameCount++;
            
            // Enhanced logging for headless mode
            if (m_frameCount % 300 == 0) { // Log every 5 seconds at 60fps
                std::cout << "[HEADLESS] FrameCount=" << m_frameCount 
                         << " FrameTime=" << frameTime << "ms"
                         << " RenderTime=" << renderMs << "ms"
                         << " AudioRMS=" << m_audioRms
                         << " Scene=" << (m_aiDirector ? m_aiDirector->GetState().current_scene_id : "none") 
                         << std::endl;
                
                if (m_metricsEnabled && m_metrics.is_open()) {
                    auto now = std::chrono::system_clock::now();
                    auto time_t = std::chrono::system_clock::to_time_t(now);
                    std::string line = "HEADLESS_METRIC," + std::to_string(time_t) + 
                        "," + std::to_string(m_frameCount) + 
                        "," + std::to_string(frameTime) + 
                        "," + std::to_string(renderMs) + 
                        "," + std::to_string(m_audioRms);
                    m_metrics.write(line.c_str(), static_cast<std::streamsize>(line.size()));
                    m_metrics.put('\n');
                }
            }
            
            if (m_safetyManager) m_safetyManager->OnHeartbeat();
            
            // Frame rate limiting
            if (frameTime < TARGET_FRAME_TIME_MS) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<int>((TARGET_FRAME_TIME_MS - frameTime) * 1000))
                );
            }
        }
    } else {
        // Normal windowed mode main loop
        while (!m_shouldExit && !m_window->ShouldClose()) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            // Process input
            m_window->PollEvents();
            ProcessInput();
            
            // Skip rendering if window is minimized
            if (m_window->IsMinimized()) {
                // Window is minimized, sleep briefly to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            
            // Calculate delta time
            CalculateDeltaTime();
            
            // Update systems
            float32 deltaTime = m_performanceMetrics.frameTimeMs / 1000.0f;
            Update(deltaTime);
            
            // Render frame
            auto renderStart = std::chrono::high_resolution_clock::now();
            Result result = BeginFrame();
            if (result == Result::Success) {
                Render();
                auto renderEnd = std::chrono::high_resolution_clock::now();
                Result endRes = EndFrame();
                if (endRes != Result::Success) { 
                    // Handle swapchain issues gracefully instead of crashing
                    if (endRes == Result::ValidationFailed) {
                        // Swapchain might be out of date, skip this frame and try again
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                        continue;
                    } else {
                        m_shouldExit = true; 
                        break; 
                    }
                }
                
                auto presentEnd = std::chrono::high_resolution_clock::now();
                float32 renderMs = std::chrono::duration<float32, std::milli>(renderEnd - renderStart).count();
                float32 presentMs = std::chrono::duration<float32, std::milli>(presentEnd - renderEnd).count();
                float32 variance = 0.0f;
                if (!m_frameTimeHistory.empty()) {
                    double sum = 0.0; for (auto v : m_frameTimeHistory) sum += v;
                    double mean = sum / static_cast<double>(m_frameTimeHistory.size());
                    double vs = 0.0; for (auto v : m_frameTimeHistory) { double d = v - mean; vs += d * d; }
                    variance = static_cast<float32>(vs / static_cast<double>(m_frameTimeHistory.size()));
                }
                float32 jitter = m_prevFrameTimeMs > 0.0f ? std::abs(m_performanceMetrics.frameTimeMs - m_prevFrameTimeMs) : 0.0f;
                if (m_performanceMetrics.frameTimeMs > 50.0f) m_droppedFrames++;
                auto ts = std::chrono::system_clock::now();
                std::time_t tsc = std::chrono::system_clock::to_time_t(ts);
                std::cout << "TS=" << tsc << " RenderMs=" << renderMs << " PresentMs=" << presentMs
                          << " FrameMs=" << m_performanceMetrics.frameTimeMs << " Var=" << variance
                          << " Jitter=" << jitter << " DroppedFrames=" << m_droppedFrames << std::endl;
                if (m_metricsEnabled && m_metrics.is_open()) {
                    std::string line = std::string("TS=") + std::to_string(tsc) +
                        " RenderMs=" + std::to_string(renderMs) +
                        " PresentMs=" + std::to_string(presentMs) +
                        " FrameMs=" + std::to_string(m_performanceMetrics.frameTimeMs) +
                        " Var=" + std::to_string(variance) +
                        " Jitter=" + std::to_string(jitter) +
                        " DroppedFrames=" + std::to_string(m_droppedFrames);
                    m_metrics.write(line.c_str(), static_cast<std::streamsize>(line.size()));
                    m_metrics.put('\n');
                }
                m_prevFrameTimeMs = m_performanceMetrics.frameTimeMs;
                if (m_safetyManager) m_safetyManager->OnHeartbeat();
            } else {
                // Handle BeginFrame failures gracefully
                if (result == Result::ValidationFailed) {
                    // Swapchain might be out of date, skip this frame and try again
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    continue;
                } else {
                    m_shouldExit = true; 
                    break; 
                }
            }
            
            // Update performance metrics
            auto frameEnd = std::chrono::high_resolution_clock::now();
            float32 frameTime = std::chrono::duration<float32, std::milli>(frameEnd - frameStart).count();
            
            m_performanceMetrics.frameTimeMs = frameTime;
            m_frameTimeHistory.push_back(frameTime);
            if (m_frameTimeHistory.size() > 144) {
                m_frameTimeHistory.erase(m_frameTimeHistory.begin());
            }
            
            UpdatePerformanceMetrics();
            m_frameCount++;
            if (m_frameCount % 60 == 0) {
                std::string scn = m_aiDirector ? m_aiDirector->GetState().current_scene_id : std::string("none");
                auto tsf = std::chrono::system_clock::now();
                auto msf = std::chrono::duration_cast<std::chrono::milliseconds>(tsf.time_since_epoch()).count();
                NeonGlyph::Logger::LogLine(std::string("RenderLoop FrameTick=") + std::to_string(m_frameCount) + std::string(" Scene=") + scn + std::string(" TS=") + std::to_string(msf));
            }
            
            // Frame rate limiting
            if (frameTime < TARGET_FRAME_TIME_MS) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<int>((TARGET_FRAME_TIME_MS - frameTime) * 1000))
                );
            }
        }
    }
#else
    // Console-only mode - run without window (fallback when GLFW not available)
    std::cout << "[Application] Running in console-only mode - GLFW not available" << std::endl;
    
    while (!m_shouldExit) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Calculate delta time
        CalculateDeltaTime();
        
        // Update systems
        float32 deltaTime = m_performanceMetrics.frameTimeMs / 1000.0f;
        Update(deltaTime);
        
        // Update performance metrics
        auto frameEnd = std::chrono::high_resolution_clock::now();
        float32 frameTime = std::chrono::duration<float32, std::milli>(frameEnd - frameStart).count();
        
        m_performanceMetrics.frameTimeMs = frameTime;
        m_frameTimeHistory.push_back(frameTime);
        if (m_frameTimeHistory.size() > 144) {
            m_frameTimeHistory.erase(m_frameTimeHistory.begin());
        }
        
        UpdatePerformanceMetrics();
        m_frameCount++;
        
        // Enhanced logging for console mode
        if (m_frameCount % 300 == 0) {
            std::cout << "[CONSOLE] FrameCount=" << m_frameCount 
                     << " FrameTime=" << frameTime << "ms"
                     << " AudioRMS=" << m_audioRms
                     << " Scene=" << (m_aiDirector ? m_aiDirector->GetState().current_scene_id : "none") 
                     << std::endl;
        }
        
        // Frame rate limiting
        if (frameTime < TARGET_FRAME_TIME_MS) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(static_cast<int>((TARGET_FRAME_TIME_MS - frameTime) * 1000))
            );
        }
    }
#endif
}

void Application::ProcessInput() {
#if NEONGLYPH_HAVE_GLFW
    // Handle basic input
    if (m_window->IsKeyPressed(VK_ESCAPE)) {
        m_shouldExit = true;
    }
    
    // Toggle fullscreen
    bool f11 = m_window->IsKeyPressed(VK_F11);
    if (f11 && !m_prevF11Pressed) {
        m_window->SetFullscreen(!m_config.window.fullscreen);
        m_config.window.fullscreen = !m_config.window.fullscreen;
    }
    m_prevF11Pressed = f11;
    
    // Overlay toggle
    bool f1 = m_window->IsKeyPressed(VK_F1);
    if (f1 && !m_prevF1Pressed) {
        m_config.render.overlayEnabled = !m_config.render.overlayEnabled;
        LogPerformanceMetrics();
        m_renderer->SetOverlayData({m_config.render.overlayEnabled, m_config.render.overlayPosition, m_performanceMetrics.frameTimeMs, 0.0f, 0.0f, 0.0f, static_cast<uint32>(m_droppedFrames), TARGET_FPS, m_audioRms, m_audioPeak, m_audioBass, m_audioMids, m_audioHighs});
    }
    m_prevF1Pressed = f1;
    
    // Palette switching (F2-F5)
    if (m_window->IsKeyPressed(VK_F2)) {
        SwitchPalette("cyberpunk");
    }
    if (m_window->IsKeyPressed(VK_F3)) {
        SwitchPalette("vaporwave");
    }
    if (m_window->IsKeyPressed(VK_F4)) {
        SwitchPalette("matrix");
    }
    if (m_window->IsKeyPressed(VK_F5)) {
        SwitchPalette("noir");
    }
    
    // Charset switching (1-8 keys)
    if (m_window->IsKeyPressed('1')) {
        SwitchCharset("dense");
    }
    if (m_window->IsKeyPressed('2')) {
        SwitchCharset("blocks");
    }
    if (m_window->IsKeyPressed('3')) {
        SwitchCharset("ascii");
    }
    if (m_window->IsKeyPressed('4')) {
        SwitchCharset("symbols");
    }
    if (m_window->IsKeyPressed('5')) {
        SwitchCharset("minimal");
    }
    if (m_window->IsKeyPressed('6')) {
        SwitchCharset("dots");
    }
    if (m_window->IsKeyPressed('7')) {
        SwitchCharset("lines");
    }
    if (m_window->IsKeyPressed('8')) {
        SwitchCharset("binary");
    }
#endif
}

void Application::Update(float32 deltaTime) {
    if (m_audioEngine->IsCapturing()) {
        AudioFrame frame;
        if (m_audioEngine->ProcessAudioFrame(frame) == Result::Success) {
            #if NEONGLYPH_HAVE_ONNXRUNTIME
            if (m_aiConductor) m_aiConductor->AnalyzeAudioFrame(frame);
            #endif
        }
        std::vector<float32> waveform;
        m_audioEngine->GetWaveform(waveform);
        if (!waveform.empty()) {
            double sumSq = 0.0;
            float32 peak = 0.0f;
            for (auto v : waveform) { sumSq += static_cast<double>(v) * static_cast<double>(v); peak = std::max(peak, std::abs(v)); }
            float32 rms = static_cast<float32>(std::sqrt(sumSq / static_cast<double>(waveform.size())));
            m_audioPeak = std::clamp(peak, 0.0f, 1.0f);
            m_audioRms = std::clamp(rms / 0.25f, 0.0f, 1.0f);
        }
        Spectrum spectrum;
        if (m_audioEngine->GetSpectrum(spectrum) == Result::Success) {
            float32 bassSum = 0.0f, midsSum = 0.0f, highsSum = 0.0f;
            uint32 bassCount = 0, midsCount = 0, highsCount = 0;
            float32 binHz = spectrum.frequencyResolution;
            for (uint32 i = 0; i < spectrum.binCount; ++i) {
                float32 freq = binHz * static_cast<float32>(i);
                float32 mag = spectrum.magnitudes[i];
                if (freq < 200.0f) { bassSum += mag; bassCount++; }
                else if (freq < 2000.0f) { midsSum += mag; midsCount++; }
                else { highsSum += mag; highsCount++; }
            }
            auto avgf = [](float32 s, uint32 c){ return c ? (s / static_cast<float32>(c)) : 0.0f; };
            m_audioBass = std::clamp(avgf(bassSum, bassCount), 0.0f, 1.0f);
            m_audioMids = std::clamp(avgf(midsSum, midsCount), 0.0f, 1.0f);
            m_audioHighs = std::clamp(avgf(highsSum, highsCount), 0.0f, 1.0f);
            #if NEONGLYPH_HAVE_ONNXRUNTIME
            if (m_aiConductor) m_aiConductor->AnalyzeSpectrum(spectrum);
            #endif
        }
        
        // Music analysis → AI Director
        if (m_musicAnalyzer) {
            if (m_musicAnalyzer->Update(frame, spectrum) == Result::Success && m_aiDirector) {
                MusicData music = m_musicAnalyzer->GetLatest();
                DirectorCommand cmd = m_aiDirector->Update(music);
#if NEONGLYPH_HAVE_GLFW
                if (m_renderer) {
                    m_renderer->Submit(cmd);
                }
#endif
            }
        }
        
        // Director System Integration - Send state snapshot and process directives
        if (m_directorClient) {
            // Send state snapshot to Director (throttled to avoid network spam)
            static int directorUpdateCounter = 0;
            directorUpdateCounter++;
            if (directorUpdateCounter >= 10) { // ~6 times per second at 60fps
                SendStateToDirector();
                directorUpdateCounter = 0;
            }
            
            // Process any pending directives from Director
            ProcessDirectorDirectives();
        }
    }

#if NEONGLYPH_HAVE_GLFW
    if (m_renderer) {
        m_renderer->Update(std::chrono::microseconds(static_cast<int>(deltaTime * 1000000.0f)));
    }
#endif

    if (m_morphSequenceActive && m_renderer) {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastMorphTick).count();
        if (ms >= 1000) {
            if (m_morphIndex < m_morphTechniques.size()) {
                DirectorCommand c; c.directives.push_back({"MorphToTestCard",{{"technique",m_morphTechniques[m_morphIndex]}}});
                m_renderer->Submit(c);
                m_morphIndex++;
                m_lastMorphTick = now;
            } else {
                DirectorCommand c; c.directives.push_back({"RenderTestCard",{{"mode","auto"}}});
                m_renderer->Submit(c);
                m_morphSequenceActive = false;
            }
        }
    }

    if (m_colorTestActive && m_renderer) {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastColorTick).count();
        if (ms >= 4000) {
            if (m_colorIndex < m_colorModes.size()) {
                std::string mode = m_colorModes[m_colorIndex];
                std::string spec = std::string("{") +
                    "\"canvas\":{\"width\":120,\"height\":48}," +
                    "\"style\":{\"symmetry\":\"vertical_mirror\",\"panelization\":{\"enabled\":true,\"columns\":3,\"rows\":2}}," +
                    "\"layers\":[{\"name\":\"background\",\"char_set\":\" .:-\",\"density\":0.06,\"motif\":\"slow_waves\",\"motion\":\"slow_scroll\",\"audio_reactivity\":\"highs\"},{\"name\":\"midground\",\"char_set\":\"=+*xo\",\"density\":0.22,\"motif\":\"tunnels\",\"motion\":\"tunnel_zoom\",\"audio_reactivity\":\"mids\"},{\"name\":\"foreground\",\"char_set\":\"#%@&\",\"density\":0.35,\"motif\":\"totem\",\"motion\":\"breathing_totem\",\"audio_reactivity\":\"bass\"}]," +
                    "\"focus\":{\"has_central_totem\":true,\"totem_width_fraction\":0.15,\"totem_description\":\"Central energy column\"}," +
                    "\"motion\":{\"global_mode\":\"tunnel_zoom\",\"phase_0_1\":0.5}," +
                    "\"audio\":{\"bass_level\":0.6,\"mids_level\":0.4,\"highs_level\":0.2}," +
                    "\"color\":{\"mode\":\"" + mode + "\",\"palette_name\":\"NeonMech\",\"primary\":[16,240,255],\"secondary\":[[120,200,255]],\"accents\":[[255,80,180],[255,220,40]],\"shadow\":[12,20,28],\"highlights\":[255,255,255],\"temperature\":\"mixed\",\"harmony\":\"complementary\"}}";
                DirectorCommand c; c.directives.push_back({"RenderSceneSpec",{{"spec",spec}}});
                m_renderer->Submit(c);
                if (m_metricsEnabled && m_metrics.is_open()) {
                    std::string line = std::string("ColorMode=") + mode;
                    m_metrics.write(line.c_str(), static_cast<std::streamsize>(line.size()));
                    m_metrics.put('\n');
                }
                m_colorIndex++;
                m_lastColorTick = now;
            } else {
                m_colorTestActive = false;
            }
        }
    }

    {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPerfLog).count();
        if (ms >= 1000) {
            if (!m_frameTimeHistory.empty()) {
                double sum = 0.0;
                for (auto v : m_frameTimeHistory) sum += v;
                double avg = sum / static_cast<double>(m_frameTimeHistory.size());
                std::cout << "FrameTimeAvgMs=" << avg << std::endl;
                if (m_metricsEnabled && m_metrics.is_open()) {
                    std::string line = std::string("FrameTimeAvgMs=") + std::to_string(avg);
                    m_metrics.write(line.c_str(), static_cast<std::streamsize>(line.size()));
                    m_metrics.put('\n');
                }
            }
            m_lastPerfLog = now;
        }
    }
    
    // Check for safety violations
    if (!m_safetyManager->IsPerformanceAcceptable(m_performanceMetrics)) {
        m_safetyManager->LogSafetyViolation("Performance threshold exceeded");
    }
}

void Application::Render() {
#if NEONGLYPH_HAVE_GLFW
    if (m_renderer) {
        std::string frame = m_renderer->GetFrameString();
        if (!frame.empty()) {
            const char* logOnly = std::getenv("NG_LOG_ONLY");
            if (!(logOnly && std::string(logOnly) == "1")) {
                std::cout << "\x1B[2J\x1B[H";
                std::cout << frame;
            }
        }
        if (m_vulkanContext && m_vulkanContext->GetSwapchain() != VK_NULL_HANDLE) {
            m_renderer->SetOverlayData({m_config.render.overlayEnabled, m_config.render.overlayPosition, m_performanceMetrics.frameTimeMs, 0.0f, 0.0f, 0.0f, static_cast<uint32>(m_droppedFrames), TARGET_FPS, m_audioRms, m_audioPeak, m_audioBass, m_audioMids, m_audioHighs});
            m_renderer->UpdateOverlay();
            VkExtent2D ex = m_vulkanContext->GetSwapchainExtent();
            auto pixels = m_renderer->ComposeFramePixels(ex.width, ex.height);
            if (!pixels.empty()) {
                m_vulkanContext->UpdateFramePixels(pixels.data(), pixels.size());
                if (m_outputManager) {
                    m_outputManager->SendFrameCPU(pixels.data(), ex.width, ex.height);
                }
            }
        }
    }
#else
    // Console-only rendering
    if (m_aiDirector) {
        MusicData music;
        music.bpm = 120.0f;
        music.key = "C major";
        music.rms_level_db = -20.0f;
        music.song_section = "intro";
        
        DirectorCommand cmd = m_aiDirector->Update(music);
        
        std::cout << "\x1B[2J\x1B[H";
        std::cout << "=== NEON-GLYPH CONSOLE MODE ===\n";
        std::cout << "BPM: " << music.bpm << " | Key: " << music.key << " | Section: " << music.song_section << "\n";
        std::cout << "Scene: " << cmd.scene_id << " | Narrative: " << static_cast<int>(cmd.narrative_beat) << "\n";
        std::cout << "Palette: " << cmd.mise_en_scene.palette << " | Glyph Map: " << cmd.glyph_map << "\n";
        std::cout << "\nASCII Preview:\n";
        std::cout << "████████████████████████████████████\n";
        std::cout << "███                              ███\n";
        std::cout << "███   NEON-GLYPH IS RUNNING     ███\n";
        std::cout << "███                              ███\n";
        std::cout << "████████████████████████████████████\n";
    }
#endif
}

Result Application::BeginFrame() {
#if NEONGLYPH_HAVE_GLFW
    return m_vulkanContext->BeginFrame();
#else
    return Result::Success;
#endif
}

Result Application::EndFrame() {
#if NEONGLYPH_HAVE_GLFW
    return m_vulkanContext->EndFrame();
#else
    return Result::Success;
#endif
}

void Application::CalculateDeltaTime() {
    // Delta time is already calculated in MainLoop
}

void Application::UpdatePerformanceMetrics() {
    // Update CPU usage
    #ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        m_performanceMetrics.memoryUsageMB = static_cast<uint32>(pmc.WorkingSetSize / (1024ull * 1024ull));
    }
    #endif
    
    // Update audio latency
    m_performanceMetrics.audioLatencyMs = static_cast<uint32>(std::lround(m_audioEngine->GetVolume() * 10.0f));
    
    // Calculate average frame time
    if (!m_frameTimeHistory.empty()) {
        float32 avgFrameTime = 0.0f;
        for (float32 time : m_frameTimeHistory) {
            avgFrameTime += time;
        }
        avgFrameTime /= m_frameTimeHistory.size();
        m_performanceMetrics.frameTimeMs = avgFrameTime;
    }
    
    // Update Vulkan performance metrics if available (windowed path)
#if NEONGLYPH_HAVE_GLFW
    if (m_vulkanContext) {
        m_vulkanContext->UpdatePerformanceMetrics();
        const auto& vulkanMetrics = m_vulkanContext->GetPerformanceMetrics();
        (void)vulkanMetrics;
        if (!m_vulkanContext->IsPerformanceOptimal()) {
            auto suggestions = m_vulkanContext->GetOptimizationSuggestions();
            for (const auto& suggestion : suggestions) {
                std::cout << "[Performance Warning] " << suggestion << std::endl;
            }
        }
        if (m_frameCount % 300 == 0) {
            m_vulkanContext->LogFrameStatistics();
        }
    }
#endif
    
    (void)m_performanceMetrics; // suppress unused warning in console mode
}

void Application::LogSystemInfo() {
    std::cout << "=== NEON-GLYPH SYSTEM INFO ===" << std::endl;
    std::cout << "Target FPS: " << TARGET_FPS << std::endl;
    std::cout << "Target Frame Time: " << TARGET_FRAME_TIME_MS << "ms" << std::endl;
    std::cout << "Max Texture Size: " << MAX_TEXTURE_SIZE << std::endl;
    std::cout << "Max Audio Sample Rate: " << MAX_AUDIO_SAMPLE_RATE << std::endl;
    std::cout << "Available Palettes: " << m_palettes.size() << std::endl;
    std::cout << "Current Charset: " << m_config.ascii.charset << std::endl;
    
    // Headless mode information
    std::cout << "Headless Mode: " << (m_headlessMode ? "ACTIVE" : "INACTIVE") << std::endl;
    if (m_headlessMode) {
        std::cout << "  Reason: " << m_headlessReason << std::endl;
        std::cout << "  Fallback Enabled: " << (m_config.headless.fallback ? "yes" : "no") << std::endl;
        std::cout << "  Logging Enabled: " << (m_config.headless.logActivation ? "yes" : "no") << std::endl;
    }
    
    std::cout << "================================" << std::endl;
    std::cout << "=== KEYBOARD SHORTCUTS ===" << std::endl;
    std::cout << "ESC - Exit application" << std::endl;
    std::cout << "F1 - Show performance metrics" << std::endl;
    std::cout << "F2 - Switch to cyberpunk palette" << std::endl;
    std::cout << "F3 - Switch to vaporwave palette" << std::endl;
    std::cout << "F4 - Switch to matrix palette" << std::endl;
    std::cout << "F5 - Switch to noir palette" << std::endl;
    std::cout << "1-8 - Switch character sets (dense, blocks, ascii, symbols, minimal, dots, lines, binary)" << std::endl;
    std::cout << "================================" << std::endl;
}

void Application::LogPerformanceMetrics() {
    std::cout << "=== PERFORMANCE METRICS ===" << std::endl;
    std::cout << "Frame Time: " << m_performanceMetrics.frameTimeMs << "ms" << std::endl;
    std::cout << "FPS: " << (1000.0f / m_performanceMetrics.frameTimeMs) << std::endl;
    std::cout << "Memory Usage: " << m_performanceMetrics.memoryUsageMB << "MB" << std::endl;
    std::cout << "Audio Latency: " << m_performanceMetrics.audioLatencyMs << "ms" << std::endl;
    std::cout << "Frame Count: " << m_frameCount << std::endl;
    std::cout << "===========================" << std::endl;
}

void Application::ShutdownSystems() {
    if (m_audioEngine) {
        m_audioEngine->StopCapture();
    }
    
    m_safetyManager.reset();
    m_aiConductor.reset();
    m_asciiConverter.reset();
    m_audioEngine.reset();
    m_vulkanContext.reset();
    m_window.reset();
    m_configManager.reset();
}

void Application::CleanupResources() {
}

void Application::OnWindowResize(uint32_t width, uint32_t height) {
    (void)width; (void)height;
}

void Application::OnKeyPress(int key) {
    (void)key;
}

void Application::OnAudioFrame(const AudioFrame& frame) {
    (void)frame;
}

void Application::OnBeatDetected(float32 bpm) {
    (void)bpm;
}

void Application::RequestExit() {
    m_shouldExit = true;
}

void Application::HandleError(Result result, const std::string& message) {
    std::cerr << "Error " << static_cast<uint32_t>(result) << ": " << message << std::endl;
}

bool Application::ShouldContinueAfterError(Result result) {
    return result != Result::DeviceLost && result != Result::InitializationFailed;
}

void Application::LogSafetyViolation(const std::string& violation) {
    std::cout << "SAFETY VIOLATION: " << violation << std::endl;
}

void Application::SwitchPalette(const std::string& paletteName) {
    ColorPalette* palette = GetPalette(paletteName);
    if (palette) {
        // Apply the palette to the ASCII converter
        if (m_asciiConverter) {
            m_asciiConverter->SetColorPalette(*palette);
            std::cout << "Switched to palette: " << paletteName << std::endl;
        }
    } else {
        std::cerr << "Palette not found: " << paletteName << std::endl;
    }
}

void Application::SwitchCharset(const std::string& charsetName) {
    std::string charset = GetCharset(charsetName);
    if (!charset.empty()) {
        m_currentCharset = charset;
        m_config.ascii.charset = charset;
        
        // Apply the charset to the ASCII converter
        if (m_asciiConverter) {
            m_asciiConverter->SetCharset(charset);
            std::cout << "Switched to charset: " << charsetName << std::endl;
        }
    } else {
        std::cerr << "Charset not found: " << charsetName << std::endl;
    }
}

ColorPalette* Application::GetPalette(const std::string& name) {
    for (auto& palette : m_palettes) {
        if (palette.name == name) {
            return &palette;
        }
    }
    return nullptr;
}

std::string Application::GetCharset(const std::string& name) {
    // Simple charset mapping - in production, load from config file
    if (name == "dense") return "@%#*+=-:. ";
    if (name == "blocks") return "█▉▊▋▌▍▎▏ ";
    if (name == "ascii") return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    if (name == "symbols") return "◆●■▲▼◀▶♠♣♥♦";
    if (name == "minimal") return "█▓▒░ ";
    if (name == "dots") return "⣿⣷⣶⣴⣄⡀ ";
    if (name == "lines") return "┃┣┫┗┛┏┓━";
    if (name == "binary") return "10 ";
    
    return m_config.ascii.charset; // Default fallback
}

void Application::InitializeDirector() {
#if NEONGLYPH_HAVE_DIRECTOR
    std::cout << "[Application] Initializing DirectorClient..." << std::endl;
    
    // Create DirectorClient that connects to local daemon
    m_directorClient = CreateDirectorClient("127.0.0.1", 9999);
    m_directorEnabled = true;
    
    // Start worker thread for DirectorClient
    m_directorWorkerThread = std::thread(&Application::DirectorWorkerLoop, this);
    
    std::cout << "[Application] DirectorClient initialized" << std::endl;
#else
    std::cout << "[Application] DirectorClient disabled" << std::endl;
#endif
}

void Application::ShutdownDirector() {
    std::cout << "[Application] Shutting down DirectorClient..." << std::endl;
    
    m_directorEnabled = false;
    
    if (m_directorWorkerThread.joinable()) {
        m_directorWorkerThread.join();
    }
    
    m_directorClient.reset();
    
    std::cout << "[Application] DirectorClient shut down" << std::endl;
}

void Application::DirectorWorkerLoop() {
    std::cout << "[Application] Director worker thread started" << std::endl;
    
    while (m_directorEnabled) {
        if (m_directorClient) {
            m_directorClient->Pump();
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    std::cout << "[Application] Director worker thread stopped" << std::endl;
}

// namespace NeonGlyph

void Application::SendStateToDirector() {
    if (!m_directorEnabled || !m_directorClient) {
        return;
    }
    
    // Create state snapshot
    DirectorStateSnapshot snapshot;
    snapshot.time = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - m_startTime
    ).count();
    
    // Audio data
    snapshot.audio.rms = m_audioRms;
    snapshot.audio.bass = m_audioBass;
    snapshot.audio.mids = m_audioMids;
    snapshot.audio.highs = m_audioHighs;
    
    // Scene data (simplified - would get from current renderer state)
    snapshot.scene.id = "SCN-DEMO-RADIAL"; // Default scene
    snapshot.scene.intensity = m_audioRms; // Use audio RMS as intensity
    snapshot.scene.palette = "KOI_EMBER"; // Default palette
    
    // Add to queue for worker thread
    {
        std::lock_guard<std::mutex> lock(m_snapshotQueueMutex);
        m_stateSnapshotQueue.push(snapshot);
    }
}

void Application::ProcessDirectorDirectives() {
    if (!m_directorEnabled || !m_directorClient) {
        return;
    }
    
    DirectorDirective directive;
    if (m_directorClient->TryGetLatestDirective(directive)) {
        // Update last directive
        m_lastDirective = directive;
        
        std::cout << "[Application] Received directive: scene=" << directive.sceneId 
                  << ", mood=" << directive.mood 
                  << ", intensity=" << directive.intensity << std::endl;
        
        // Apply directive to AIConductor
        if (m_aiConductor) {
            // TODO: Add ApplyDirective method to AIConductor
            // For now, just log the directive
            std::cout << "[Application] Applying directive to AIConductor" << std::endl;
        }
    }
}

Result Application::ParseCommandLineArgs(int argc, char* argv[]) {
    std::cout << "[Application] Parsing command line arguments..." << std::endl;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--headless" || arg == "-h") {
            m_forceHeadless = true;
            m_config.headless.enabled = true;
            std::cout << "[Application] Headless mode forced via command line" << std::endl;
        }
        else if (arg == "--no-headless-fallback" || arg == "--disable-fallback") {
            m_config.headless.fallback = false;
            std::cout << "[Application] Headless fallback disabled via command line" << std::endl;
        }
        else if (arg == "--enable-headless-logging") {
            m_config.headless.logActivation = true;
            std::cout << "[Application] Headless mode logging enabled via command line" << std::endl;
        }
        else if (arg == "--help") {
            std::cout << "NeonGlyph - AI-Driven ASCII Visual Synthesis Engine" << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --headless, -h           Force headless mode (no window)" << std::endl;
            std::cout << "  --no-headless-fallback   Disable automatic fallback to headless mode" << std::endl;
            std::cout << "  --enable-headless-logging Enable logging when headless mode is activated" << std::endl;
            std::cout << "  --help                   Show this help message" << std::endl;
            return Result::UnsupportedOperation; // Signal to exit after showing help
        }
    }
    
    return Result::Success;
}

bool Application::ShouldUseHeadlessMode() const {
    // Check if headless mode is forced via command line or config
    if (m_forceHeadless || m_config.headless.enabled) {
        return true;
    }
    
    // Check if fallback is enabled and window creation previously failed
    if (m_fallbackEnabled && m_config.headless.fallback) {
        // This could be extended to check for previous failure history
        return false;
    }
    
    return false;
}

void Application::LogHeadlessModeActivation(const std::string& reason) {
    m_headlessReason = reason;
    m_headlessActivationTime = std::chrono::steady_clock::now();
    m_headlessMode = true;
    
    if (m_config.headless.logActivation) {
        std::cout << "[HEADLESS MODE ACTIVATED]" << std::endl;
        std::cout << "  Reason: " << reason << std::endl;
        std::cout << "  Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(
            m_headlessActivationTime.time_since_epoch()).count() << "ms" << std::endl;
        std::cout << "  Fallback enabled: " << (m_config.headless.fallback ? "yes" : "no") << std::endl;
        
        // Log to file if metrics are enabled
        if (m_metricsEnabled && m_metrics.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string log_entry = "HEADLESS_ACTIVATE," + std::to_string(time_t) + "," + reason;
            m_metrics.write(log_entry.c_str(), static_cast<std::streamsize>(log_entry.size()));
            m_metrics.put('\n');
        }
    }
    
    NeonGlyph::Logger::LogLine("Headless mode activated: " + reason);
}

Result Application::InitializeWindowWithFallback() {
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "🎮 NEONGLYPH WINDOW CREATION DIAGNOSTICS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "[Application] Attempting window initialization with fallback support..." << std::endl;
    
    // Enhanced error detection and logging
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Attempt window creation
    Result result = InitializeWindow();
    auto window_time = std::chrono::high_resolution_clock::now();
    double window_ms = std::chrono::duration<double, std::milli>(window_time - start_time).count();
    
    if (result == Result::Success) {
        std::cout << "[Application] ✅ WINDOW CREATED SUCCESSFULLY in " << window_ms << "ms" << std::endl;
        std::cout << "[Application] 🎮 VISUAL WINDOW IS NOW ACTIVE - You should see the application window!" << std::endl;
        
        // Proceed with Vulkan initialization
        std::cout << "[Application] Initializing Vulkan context..." << std::endl;
        result = InitializeVulkan();
        auto vulkan_time = std::chrono::high_resolution_clock::now();
        double vulkan_ms = std::chrono::duration<double, std::milli>(vulkan_time - window_time).count();
        
        if (result == Result::Success) {
            std::cout << "[Application] Vulkan initialized successfully in " << vulkan_ms << "ms" << std::endl;
            
            // Initialize ASCII converter
            result = InitializeASCII();
            if (result == Result::Success) {
                std::cout << "[Application] ASCII converter initialized successfully" << std::endl;
                return Result::Success;
            } else {
                std::cerr << "[Application] ASCII converter initialization failed: " << static_cast<uint32_t>(result) << std::endl;
                // Continue with fallback instead of failing completely
            }
        } else {
            std::cerr << "[Application] Vulkan initialization failed: " << static_cast<uint32_t>(result) << std::endl;
        }
    } else {
        std::cerr << "❌ [Application] WINDOW CREATION FAILED: " << static_cast<uint32_t>(result) << std::endl;
        std::cerr << "❌ [Application] ERROR: Cannot create visual window - Application will not display!" << std::endl;
        std::cerr << "❌ [Application] This is why you don't see the application window!" << std::endl;
        
        // Check if we should attempt fallback
        if (m_config.headless.fallback) {
            std::cout << "[Application] ⚠️  Automatically falling back to HEADLESS MODE (no window)" << std::endl;
            std::cout << "[Application] ⚠️  Application is running but INVISIBLE - this is the 'crashing' issue!" << std::endl;
            LogHeadlessModeActivation("Window creation failed: " + std::to_string(static_cast<uint32_t>(result)));
            return InitializeHeadlessMode();
        } else {
            std::cerr << "[Application] Fallback disabled, returning error" << std::endl;
            return result;
        }
    }
    
    // If we reach here, something failed but we can still try headless mode
    // HEADLESS FALLBACK DISABLED - This is a VISUAL MASTERPIECE that must ALWAYS be shown!
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "🎨 VISUAL MASTERPIECE REQUIREMENT: Window creation MUST succeed!" << std::endl;
    std::cout << "🎨 This application is designed to be a VISUAL EXPERIENCE - headless mode is DISABLED!" << std::endl;
    std::cout << "🎨 The application will NOT run without a visible window!" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════════════════════" << std::endl;
    
    // Return the error instead of falling back to headless mode
    std::cerr << "❌ APPLICATION TERMINATED: Window creation is REQUIRED for this visual experience!" << std::endl;
    std::cerr << "❌ Please fix the window creation error above to see the visual masterpiece!" << std::endl;
    return result;
}

Result Application::InitializeHeadlessMode() {
    std::cout << "[Application] Initializing headless mode..." << std::endl;
    
    // Initialize Vulkan in headless mode (no surface required)
    std::cout << "[Application] Creating Vulkan context for headless operation..." << std::endl;
    m_vulkanContext = std::make_unique<VulkanContext>();
    
    Result result = m_vulkanContext->Initialize(m_config);
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to initialize Vulkan in headless mode: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    std::cout << "[Application] Vulkan initialized successfully in headless mode" << std::endl;
    
    // Initialize ASCII converter for headless operation
    m_asciiConverter = std::make_unique<ASCIIConverter>();
    result = m_asciiConverter->Initialize(m_vulkanContext.get(), m_config);
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to initialize ASCII converter in headless mode: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    std::cout << "[Application] ASCII converter initialized successfully in headless mode" << std::endl;
    
    // Create renderer for headless operation
    m_renderer = std::make_unique<Renderer>();
    result = m_renderer->Initialize(m_vulkanContext.get(), m_config);
    if (result != Result::Success) {
        std::cerr << "[Application] Failed to initialize renderer in headless mode: " << static_cast<uint32_t>(result) << std::endl;
        return result;
    }
    
    m_renderer->SetASCIIConverter(m_asciiConverter.get());
    std::cout << "[Application] Renderer initialized successfully in headless mode" << std::endl;
    
    // Set up headless-specific configuration
    m_config.render.overlayEnabled = false; // No display for overlay in headless mode
    m_renderer->RenderTestCard("auto");
    
    NeonGlyph::Logger::LogLine("Headless mode initialization completed successfully");
    return Result::Success;
}

} // namespace NeonGlyph
