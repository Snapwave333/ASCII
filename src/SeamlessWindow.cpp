#include "SeamlessWindow.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

#if NEONGLYPH_HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace NeonGlyph {

// Static member for Windows callback
#ifdef _WIN32
static SeamlessWindow* g_currentWindow = nullptr;
#endif

SeamlessWindow::SeamlessWindow()
    : m_window(nullptr)
    , m_monitor(nullptr)
#ifdef _WIN32
    , m_hwnd(nullptr)
    , m_originalStyle(0)
    , m_originalExStyle(0)
    , m_wasMaximized(false)
#endif
    , m_width(800)
    , m_height(600)
    , m_x(0)
    , m_y(0)
    , m_currentMode(DisplayMode::Seamless)
    , m_scalingMode(ScalingMode::Adaptive)
    , m_shouldClose(false)
    , m_currentMonitor(0)
{
#ifdef _WIN32
    ZeroMemory(&m_originalRect, sizeof(m_originalRect));
#endif
}

SeamlessWindow::~SeamlessWindow() {
    Destroy();
}

Result SeamlessWindow::Create(const std::string& title, int width, int height, bool borderless) {
    m_title = title;
    m_width = width;
    m_height = height;

#if NEONGLYPH_HAVE_GLFW
    if (!glfwInit()) {
        return Result::Error("Failed to initialize GLFW");
    }

    // Set GLFW hints for seamless operation
    glfwWindowHint(GLFW_DECORATED, borderless ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    
    // Create window
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return Result::Error("Failed to create GLFW window");
    }

    // Get native handle
#ifdef _WIN32
    m_hwnd = glfwGetWin32Window(m_window);
    if (!m_hwnd) {
        glfwDestroyWindow(m_window);
        return Result::Error("Failed to get native Windows handle");
    }
    g_currentWindow = this;
#endif

    // Setup callbacks
    SetupCallbacks();
    
    // Detect monitors
    DetectMonitors();
    
    // Apply initial seamless mode if requested
    if (borderless) {
        return EnterSeamlessMode();
    }

    return Result::Success();
#else
    return Result::Error("GLFW not available");
#endif
}

void SeamlessWindow::Destroy() {
#if NEONGLYPH_HAVE_GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
#endif
}

Result SeamlessWindow::SetDisplayMode(DisplayMode mode) {
    if (mode == m_currentMode) {
        return Result::Success();
    }

    switch (mode) {
        case DisplayMode::Seamless:
            return EnterSeamlessMode();
        case DisplayMode::Fullscreen:
            return EnterTrueFullscreen();
        case DisplayMode::Borderless:
            return EnterBorderlessWindowed();
        case DisplayMode::Windowed:
            return ExitSeamlessMode();
        default:
            return Result::Error("Invalid display mode");
    }
}

Result SeamlessWindow::EnterSeamlessMode() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Store original window state
    m_originalStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    m_originalExStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    GetWindowRect(m_hwnd, &m_originalRect);
    
    // Remove ALL window decorations and borders
    LONG newStyle = WS_POPUP | WS_VISIBLE;  // Minimal style
    LONG newExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;  // Hide from taskbar, stay on top
    
    // Apply new style
    SetWindowLong(m_hwnd, GWL_STYLE, newStyle);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, newExStyle);
    
    // Get monitor info for proper sizing
    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {0};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(monitor, &monitorInfo);
    
    // Set window to cover entire monitor area
    SetWindowPos(m_hwnd, HWND_TOPMOST,
                 monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                 monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                 monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    
    // Hide from taskbar and make truly seamless
    ShowWindow(m_hwnd, SW_SHOW);
    
    m_currentMode = DisplayMode::Seamless;
    m_width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    m_height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
    
    return Result::Success();
#else
    return Result::Error("Seamless mode not supported on this platform");
#endif
}

Result SeamlessWindow::EnterTrueFullscreen() {
#if NEONGLYPH_HAVE_GLFW
    if (!m_window) {
        return Result::Error("Window not available");
    }

    // Get primary monitor
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    if (!primary) {
        return Result::Error("Failed to get primary monitor");
    }

    // Get video mode for proper fullscreen
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    if (!mode) {
        return Result::Error("Failed to get video mode");
    }

    // Set fullscreen
    glfwSetWindowMonitor(m_window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
    
    m_currentMode = DisplayMode::Fullscreen;
    m_width = mode->width;
    m_height = mode->height;
    
    return Result::Success();
#else
    return Result::Error("Fullscreen not supported on this platform");
#endif
}

Result SeamlessWindow::EnterBorderlessWindowed() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Remove decorations but keep windowed mode
    LONG style = WS_POPUP | WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE;
    LONG exStyle = 0;
    
    SetWindowLong(m_hwnd, GWL_STYLE, style);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);
    
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    m_currentMode = DisplayMode::Borderless;
    
    return Result::Success();
#else
    return Result::Error("Borderless mode not supported on this platform");
#endif
}

Result SeamlessWindow::ExitSeamlessMode() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Restore original window style
    SetWindowLong(m_hwnd, GWL_STYLE, m_originalStyle);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, m_originalExStyle);
    
    // Restore original window placement
    SetWindowPlacement(m_hwnd, &m_windowedPlacement);
    SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    m_currentMode = DisplayMode::Windowed;
    
    return Result::Success();
#else
    return Result::Error("Exit seamless mode not supported on this platform");
#endif
}

Result SeamlessWindow::RemoveAllBorders() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Remove ALL possible borders and chrome
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    
    // Remove every possible border and decoration
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | 
               WS_SYSMENU | WS_BORDER | WS_DLGFRAME | WS_VSCROLL | WS_HSCROLL);
    
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE |
                 WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_APPWINDOW);
    
    SetWindowLong(m_hwnd, GWL_STYLE, style);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);
    
    return Result::Success();
#else
    return Result::Error("Remove borders not supported on this platform");
#endif
}

Result SeamlessWindow::DisableWindowDecorations() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Disable window decorations completely
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    style &= ~WS_CAPTION;  // Remove title bar
    style &= ~WS_SYSMENU;  // Remove system menu
    style &= ~WS_MINIMIZEBOX;  // Remove minimize button
    style &= ~WS_MAXIMIZEBOX;  // Remove maximize button
    
    SetWindowLong(m_hwnd, GWL_STYLE, style);
    
    return Result::Success();
#else
    return Result::Error("Disable decorations not supported on this platform");
#endif
}

SeamlessWindow::ScalingInfo SeamlessWindow::CalculateOptimalScaling(int sourceWidth, int sourceHeight) const {
    ScalingInfo info;
    info.sourceWidth = sourceWidth;
    info.sourceHeight = sourceHeight;
    
    // Get current monitor dimensions
    MonitorInfo monitor = GetCurrentMonitor();
    info.targetWidth = monitor.width;
    info.targetHeight = monitor.height;
    
    switch (m_scalingMode) {
        case ScalingMode::Stretch:
            // Fill entire screen (may distort aspect ratio)
            info.scaleX = static_cast<float>(monitor.width) / sourceWidth;
            info.scaleY = static_cast<float>(monitor.height) / sourceHeight;
            info.offsetX = 0;
            info.offsetY = 0;
            info.letterbox = false;
            break;
            
        case ScalingMode::AspectRatio:
            // Maintain aspect ratio with letterboxing
            {
                float sourceAspect = static_cast<float>(sourceWidth) / sourceHeight;
                float targetAspect = static_cast<float>(monitor.width) / monitor.height;
                
                if (sourceAspect > targetAspect) {
                    // Source is wider - fit to width
                    info.scaleX = info.scaleY = static_cast<float>(monitor.width) / sourceWidth;
                    info.offsetX = 0;
                    info.offsetY = (monitor.height - sourceHeight * info.scaleY) / 2;
                } else {
                    // Source is taller - fit to height
                    info.scaleX = info.scaleY = static_cast<float>(monitor.height) / sourceHeight;
                    info.offsetX = (monitor.width - sourceWidth * info.scaleX) / 2;
                    info.offsetY = 0;
                }
                info.letterbox = true;
            }
            break;
            
        case ScalingMode::PixelPerfect:
            // Integer scaling only (no fractional scaling)
            {
                float scaleX = static_cast<float>(monitor.width) / sourceWidth;
                float scaleY = static_cast<float>(monitor.height) / sourceHeight;
                float minScale = std::min(scaleX, scaleY);
                int integerScale = static_cast<int>(std::floor(minScale));
                
                info.scaleX = info.scaleY = static_cast<float>(integerScale);
                info.offsetX = (monitor.width - sourceWidth * integerScale) / 2;
                info.offsetY = (monitor.height - sourceHeight * integerScale) / 2;
                info.letterbox = true;
            }
            break;
            
        case ScalingMode::Adaptive:
            // Smart scaling based on content and screen size
            {
                float scaleX = static_cast<float>(monitor.width) / sourceWidth;
                float scaleY = static_cast<float>(monitor.height) / sourceHeight;
                float optimalScale = std::min(scaleX, scaleY) * 0.95f; // 95% to leave small border
                
                info.scaleX = info.scaleY = optimalScale;
                info.offsetX = (monitor.width - sourceWidth * optimalScale) / 2;
                info.offsetY = (monitor.height - sourceHeight * optimalScale) / 2;
                info.letterbox = true;
            }
            break;
    }
    
    return info;
}

void SeamlessWindow::DetectMonitors() {
    m_monitors.clear();
    
#if NEONGLYPH_HAVE_GLFW
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    
    for (int i = 0; i < count; i++) {
        MonitorInfo info;
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        
        info.width = mode->width;
        info.height = mode->height;
        info.refreshRate = mode->refreshRate;
        info.aspectRatio = static_cast<float>(mode->width) / mode->height;
        info.isPrimary = (i == 0);
        info.name = glfwGetMonitorName(monitors[i]) ? glfwGetMonitorName(monitors[i]) : "Unknown";
        
        m_monitors.push_back(info);
    }
#endif
}

SeamlessWindow::MonitorInfo SeamlessWindow::GetCurrentMonitor() const {
    if (m_currentMonitor < m_monitors.size()) {
        return m_monitors[m_currentMonitor];
    }
    
    // Return primary monitor as fallback
    for (const auto& monitor : m_monitors) {
        if (monitor.isPrimary) {
            return monitor;
        }
    }
    
    // Return first monitor as ultimate fallback
    if (!m_monitors.empty()) {
        return m_monitors[0];
    }
    
    // Return default values
    return MonitorInfo{1920, 1080, 60, 16.0f/9.0f, true, "Default"};
}

void SeamlessWindow::SetupCallbacks() {
#if NEONGLYPH_HAVE_GLFW
    glfwSetWindowUserPointer(m_window, this);
    
    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        SeamlessWindow* self = static_cast<SeamlessWindow*>(glfwGetWindowUserPointer(window));
        if (self && self->m_keyboardCallback) {
            self->m_keyboardCallback(key, scancode, action, mods);
        }
    });
    
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
        SeamlessWindow* self = static_cast<SeamlessWindow*>(glfwGetWindowUserPointer(window));
        if (self && self->m_mouseCallback) {
            self->m_mouseCallback(x, y);
        }
    });
    
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        SeamlessWindow* self = static_cast<SeamlessWindow*>(glfwGetWindowUserPointer(window));
        if (self) {
            self->m_width = width;
            self->m_height = height;
            if (self->m_resizeCallback) {
                self->m_resizeCallback(width, height);
            }
        }
    });
#endif
}

void SeamlessWindow::PollEvents() {
#if NEONGLYPH_HAVE_GLFW
    glfwPollEvents();
    m_shouldClose = glfwWindowShouldClose(m_window);
#endif
}

bool SeamlessWindow::ShouldClose() const {
    return m_shouldClose;
}

void* SeamlessWindow::GetNativeHandle() const {
#ifdef _WIN32
    return m_hwnd;
#else
    return m_window;
#endif
}

void* SeamlessWindow::GetDisplayHandle() const {
    return m_monitor;
}

void SeamlessWindow::SetTitle(const std::string& title) {
    m_title = title;
#if NEONGLYPH_HAVE_GLFW
    if (m_window) {
        glfwSetWindowTitle(m_window, title.c_str());
    }
#endif
}

void SeamlessWindow::SetPosition(int x, int y) {
    m_x = x;
    m_y = y;
#if NEONGLYPH_HAVE_GLFW
    if (m_window) {
        glfwSetWindowPos(m_window, x, y);
    }
#endif
}

void SeamlessWindow::GetPosition(int& x, int& y) const {
    x = m_x;
    y = m_y;
}

void SeamlessWindow::SetSize(int width, int height) {
    m_width = width;
    m_height = height;
#if NEONGLYPH_HAVE_GLFW
    if (m_window) {
        glfwSetWindowSize(m_window, width, height);
    }
#endif
}

void SeamlessWindow::GetSize(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

void SeamlessWindow::SetKeyboardCallback(std::function<void(int, int, int, int)> callback) {
    m_keyboardCallback = callback;
}

void SeamlessWindow::SetMouseCallback(std::function<void(double, double)> callback) {
    m_mouseCallback = callback;
}

void SeamlessWindow::SetResizeCallback(std::function<void(int, int)> callback) {
    m_resizeCallback = callback;
}

void SeamlessWindow::GetDebugInfo(std::string& info) const {
    std::stringstream ss;
    ss << "SeamlessWindow Debug Info:\n";
    ss << "  Mode: " << (m_currentMode == DisplayMode::Seamless ? "Seamless" : 
                     m_currentMode == DisplayMode::Fullscreen ? "Fullscreen" :
                     m_currentMode == DisplayMode::Borderless ? "Borderless" : "Windowed") << "\n";
    ss << "  Size: " << m_width << "x" << m_height << "\n";
    ss << "  Position: " << m_x << "," << m_y << "\n";
    ss << "  Monitors: " << m_monitors.size() << "\n";
    
    if (!m_monitors.empty()) {
        ss << "  Primary Monitor: " << m_monitors[0].width << "x" << m_monitors[0].height 
           << " @ " << m_monitors[0].refreshRate << "Hz\n";
    }
    
    info = ss.str();
}

} // namespace NeonGlyph