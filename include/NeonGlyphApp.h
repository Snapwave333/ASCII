#pragma once

#include "NeonGlyph.h"
#include "Application.h"
#include "Window.h"
#include "WindowsIntegration.h"
#include <memory>
#include <iostream>

namespace NeonGlyph {

class NeonGlyphApp {
public:
    NeonGlyphApp();
    ~NeonGlyphApp();
    
    Result Initialize(const Config& config);
    Result Run();
    void Shutdown();

private:
    Result InitializeWindowsIntegration();
    void HandleKeyboardInput(int key, int scancode, int action, int mods);
    void OnAIStateChange(WindowsIntegration::AIState state);
    void OnWindowModeChange(WindowsIntegration::WindowMode mode);
    
    std::unique_ptr<Application> m_application;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<WindowsIntegration> m_windowsIntegration;
    Config m_config;
    bool m_running;
};

} // namespace NeonGlyph