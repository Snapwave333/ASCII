#pragma once

#include "NeonGlyph.h"
#include <string>
#include <memory>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#if NEONGLYPH_HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace NeonGlyph {

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
    DisplayMode GetDisplayMode() const { return m_currentMode; }
    
    // Seamless scaling
    Result SetScalingMode(ScalingMode mode);
    ScalingMode GetScalingMode() const { return m_scalingMode; }
    
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
    std::string GetTitle() const { return m_title; }
    
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
    bool IsSeamless() const { return m_currentMode == DisplayMode::Seamless; }
    bool IsFullscreen() const { return m_currentMode == DisplayMode::Fullscreen; }

private:
#if NEONGLYPH_HAVE_GLFW
    GLFWwindow* m_window;
    GLFWmonitor* m_monitor;
#else
    void* m_window;
    void* m_monitor;
#endif

#ifdef _WIN32
    HWND m_hwnd;
    DWORD m_originalStyle;
    DWORD m_originalExStyle;
    RECT m_originalRect;
    bool m_wasMaximized;
#endif

    // Window properties
    std::string m_title;
    int m_width, m_height;
    int m_x, m_y;
    DisplayMode m_currentMode;
    ScalingMode m_scalingMode;
    bool m_shouldClose;
    
    // Monitor info
    int m_currentMonitor;
    std::vector<MonitorInfo> m_monitors;
    
    // Callbacks
    std::function<void(int, int, int, int)> m_keyboardCallback;
    std::function<void(double, double)> m_mouseCallback;
    std::function<void(int, int)> m_resizeCallback;
    
    // Private methods
    void DetectMonitors();
    void SetupCallbacks();
    void HandleResize(int width, int height);
    void HandleKey(int key, int scancode, int action, int mods);
    void HandleMouse(double x, double y);
    
    // Windows-specific seamless implementation
#ifdef _WIN32
    Result ApplySeamlessWindowsStyle();
    Result RemoveWindowsBorders();
    Result SetWindowsFullscreen(bool fullscreen);
    Result SetWindowsTransparency();
    LRESULT HandleWindowsMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
};

} // namespace NeonGlyph