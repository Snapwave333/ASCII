#pragma once

#include "NeonGlyph.h"
#include <string>
#include <memory>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace NeonGlyph {

enum class WindowMode {
    Windowed,
    Borderless,
    Fullscreen
};

enum class AIState {
    Stopped,
    Running,
    Break
};

enum class DisplayMode {
    Normal,     // Show all UI elements
    PureVisual  // Hide all HUD elements - pure ASCII visuals only
};

class WindowsIntegration {
public:
    WindowsIntegration();
    ~WindowsIntegration();

    // Initialize Windows integration
    Result Initialize(class Window* window, class Application* app);
    
    // System tray functionality
    Result CreateSystemTrayIcon();
    Result RemoveSystemTrayIcon();
    void UpdateSystemTrayTooltip(const std::string& tooltip);
    void UpdateSystemTrayMenu(AIState state);

    // Startup integration
    Result RegisterForStartup();
    Result UnregisterFromStartup();
    bool IsRegisteredForStartup() const;

    // Window management
    Result SetWindowMode(WindowMode mode);
    WindowMode GetWindowMode() const { return m_currentMode; }
    
    Result ToggleFullscreen();
    Result SetBorderlessMode(bool borderless);
    
    // Keyboard shortcuts
    void ProcessKeyboardShortcuts(int key, int mods);
    
    // AI control functions
    void StartAIShow();
    void StopAIShow();
    void TakeAIBreak();

    // Pure visual mode - hide all HUD elements
    void SetPureVisualMode(bool enabled);
    bool IsPureVisualMode() const { return m_displayMode == DisplayMode::PureVisual; }

    // Callbacks
    using StateChangeCallback = std::function<void(AIState)>;
    using WindowModeCallback = std::function<void(WindowMode)>;
    
    void SetStateChangeCallback(StateChangeCallback callback) { m_stateCallback = callback; }
    void SetWindowModeCallback(WindowModeCallback callback) { m_windowModeCallback = callback; }

private:
#ifdef _WIN32
    // Windows-specific members
    HWND m_hwnd;
    HMENU m_trayMenu;
    NOTIFYICONDATA m_trayIcon;
    bool m_trayIconCreated;
    
    // Window state
    WINDOWPLACEMENT m_windowedPlacement;
    bool m_wasMaximized;
    
    // Registration for startup
    bool m_startupRegistered;
#endif

    // Core members
    Window* m_window;
    Application* m_application;
    WindowMode m_currentMode;
    AIState m_aiState;
    DisplayMode m_displayMode;
    
    // Callbacks
    StateChangeCallback m_stateCallback;
    WindowModeCallback m_windowModeCallback;

    // Helper functions
    Result CreateTrayMenu();
    void ShowTrayMenu();
    
    // Keyboard shortcut handlers
    void HandleFullscreenShortcut();
    void HandleAIControlShortcuts(int key);
    
    // Window mode transitions
    Result EnterFullscreen();
    Result ExitFullscreen();
    Result EnterBorderless();
    Result ExitBorderless();
};

} // namespace NeonGlyph