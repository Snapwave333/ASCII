#include "NeonGlyphApp.h"
#include "ConfigManager.h"

namespace NeonGlyph {

NeonGlyphApp::NeonGlyphApp() 
    : m_running(false) {
}

NeonGlyphApp::~NeonGlyphApp() {
    Shutdown();
}

Result NeonGlyphApp::Initialize(const Config& config) {
    m_config = config;
    
    std::cout << "Initializing NeonGlyph Application..." << std::endl;
    
    // Create window
    m_window = std::make_unique<Window>();
    Result result = m_window->Create(config);
    if (result != Result::Success) {
        std::cerr << "Failed to create window" << std::endl;
        return result;
    }
    
    // Set up keyboard callback
    m_window->SetKeyboardCallback([this](int key, int scancode, int action, int mods) {
        this->HandleKeyboardInput(key, scancode, action, mods);
    });
    
    // Create application
    m_application = std::make_unique<Application>();
    result = m_application->Initialize(config, m_window.get());
    if (result != Result::Success) {
        std::cerr << "Failed to initialize application" << std::endl;
        return result;
    }
    
    // Initialize Windows integration
    result = InitializeWindowsIntegration();
    if (result != Result::Success) {
        std::cerr << "Failed to initialize Windows integration" << std::endl;
        return result;
    }
    
    m_running = true;
    std::cout << "NeonGlyph Application initialized successfully" << std::endl;
    
    return Result::Success;
}

Result NeonGlyphApp::InitializeWindowsIntegration() {
#ifdef _WIN32
    m_windowsIntegration = std::make_unique<WindowsIntegration>();
    
    Result result = m_windowsIntegration->Initialize(m_window.get(), m_application.get());
    if (result != Result::Success) {
        return result;
    }
    
    // Set up callbacks
    m_windowsIntegration->SetStateChangeCallback([this](WindowsIntegration::AIState state) {
        this->OnAIStateChange(state);
    });
    
    m_windowsIntegration->SetWindowModeCallback([this](WindowsIntegration::WindowMode mode) {
        this->OnWindowModeChange(mode);
    });
    
    // Create system tray icon
    result = m_windowsIntegration->CreateSystemTrayIcon();
    if (result != Result::Success) {
        std::cout << "Warning: Failed to create system tray icon" << std::endl;
    }
    
    // Register for startup if configured
    if (m_config.startup.autoStart) {
        result = m_windowsIntegration->RegisterForStartup();
        if (result != Result::Success) {
            std::cout << "Warning: Failed to register for startup" << std::endl;
        }
    }
    
    // Set initial window mode
    if (m_config.window.fullscreen) {
        m_windowsIntegration->SetWindowMode(WindowsIntegration::WindowMode::Fullscreen);
    } else if (m_config.window.borderless) {
        m_windowsIntegration->SetWindowMode(WindowsIntegration::WindowMode::Borderless);
    }
    
#endif
    return Result::Success;
}

void NeonGlyphApp::HandleKeyboardInput(int key, int scancode, int action, int mods) {
    // Handle global keyboard shortcuts
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
#ifdef _WIN32
        if (m_windowsIntegration) {
            m_windowsIntegration->ProcessKeyboardShortcuts(key, mods);
        }
#endif
        
        // Handle Escape key to exit fullscreen
        if (key == GLFW_KEY_ESCAPE) {
            if (m_windowsIntegration) {
                auto currentMode = m_windowsIntegration->GetWindowMode();
                if (currentMode == WindowsIntegration::WindowMode::Fullscreen) {
                    m_windowsIntegration->SetWindowMode(WindowsIntegration::WindowMode::Windowed);
                }
            }
        }
        
        // Handle F11 for fullscreen toggle
        if (key == GLFW_KEY_F11) {
            if (m_windowsIntegration) {
                m_windowsIntegration->ToggleFullscreen();
            }
        }
        
        // Handle Alt combinations for AI control
        if (mods & GLFW_MOD_ALT) {
            switch (key) {
                case GLFW_KEY_B: // Alt+B - AI Break
                    if (m_windowsIntegration) {
                        m_windowsIntegration->TakeAIBreak();
                    }
                    break;
                case GLFW_KEY_S: // Alt+S - Stop AI Show
                    if (m_windowsIntegration) {
                        m_windowsIntegration->StopAIShow();
                    }
                    break;
                case GLFW_KEY_P: // Alt+P - Start AI Show
                    if (m_windowsIntegration) {
                        m_windowsIntegration->StartAIShow();
                    }
                    break;
            }
        }
    }
}

void NeonGlyphApp::OnAIStateChange(WindowsIntegration::AIState state) {
    switch (state) {
        case WindowsIntegration::AIState::Running:
            std::cout << "AI Show started" << std::endl;
            // Notify application to start AI processing
            if (m_application) {
                // Application-specific AI start logic
            }
            break;
        case WindowsIntegration::AIState::Stopped:
            std::cout << "AI Show stopped" << std::endl;
            // Notify application to stop AI processing
            if (m_application) {
                // Application-specific AI stop logic
            }
            break;
        case WindowsIntegration::AIState::Break:
            std::cout << "AI taking break" << std::endl;
            // Notify application to pause AI processing
            if (m_application) {
                // Application-specific AI pause logic
            }
            break;
    }
}

void NeonGlyphApp::OnWindowModeChange(WindowsIntegration::WindowMode mode) {
    switch (mode) {
        case WindowsIntegration::WindowMode::Fullscreen:
            std::cout << "Switched to fullscreen mode" << std::endl;
            break;
        case WindowsIntegration::WindowMode::Borderless:
            std::cout << "Switched to borderless mode" << std::endl;
            break;
        case WindowsIntegration::WindowMode::Windowed:
            std::cout << "Switched to windowed mode" << std::endl;
            break;
    }
}

Result NeonGlyphApp::Run() {
    if (!m_running) {
        return Result::Failure;
    }
    
    std::cout << "Starting NeonGlyph main loop..." << std::endl;
    
    // Main application loop
    while (!m_window->ShouldClose() && m_running) {
        m_window->PollEvents();
        
        if (m_application) {
            Result result = m_application->Update();
            if (result != Result::Success) {
                std::cerr << "Application update failed" << std::endl;
                break;
            }
            
            result = m_application->Render();
            if (result != Result::Success) {
                std::cerr << "Application render failed" << std::endl;
                break;
            }
        }
    }
    
    return Result::Success;
}

void NeonGlyphApp::Shutdown() {
    std::cout << "Shutting down NeonGlyph Application..." << std::endl;
    
    m_running = false;
    
#ifdef _WIN32
    if (m_windowsIntegration) {
        m_windowsIntegration->RemoveSystemTrayIcon();
    }
#endif
    
    if (m_application) {
        m_application->Shutdown();
        m_application.reset();
    }
    
    if (m_window) {
        m_window.reset();
    }
    
    m_windowsIntegration.reset();
    
    std::cout << "NeonGlyph Application shutdown complete" << std::endl;
}

} // namespace NeonGlyph