#include "WindowsIntegration.h"
#include "Window.h"
#include "Application.h"
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <shlobj.h>
#include <strsafe.h>
#endif

namespace NeonGlyph {

// Constants for keyboard shortcuts
constexpr int KEY_F11 = 294;  // GLFW key code for F11
constexpr int KEY_ESC = 256;  // GLFW key code for ESC
constexpr int MOD_ALT = 4;    // GLFW modifier for Alt

WindowsIntegration::WindowsIntegration() 
    : m_window(nullptr)
    , m_application(nullptr)
    , m_currentMode(WindowMode::Windowed)
    , m_aiState(AIState::Stopped)
    , m_displayMode(DisplayMode::Normal)
#ifdef _WIN32
    , m_hwnd(nullptr)
    , m_trayMenu(nullptr)
    , m_trayIconCreated(false)
    , m_wasMaximized(false)
    , m_startupRegistered(false)
#endif
{
#ifdef _WIN32
    ZeroMemory(&m_trayIcon, sizeof(m_trayIcon));
    ZeroMemory(&m_windowedPlacement, sizeof(m_windowedPlacement));
#endif
}

WindowsIntegration::~WindowsIntegration() {
    RemoveSystemTrayIcon();
}

Result WindowsIntegration::Initialize(Window* window, Application* app) {
    if (!window || !app) {
        return Result::Error("Invalid window or application pointer");
    }

    m_window = window;
    m_application = app;

#ifdef _WIN32
    // Get native window handle
    m_hwnd = static_cast<HWND>(m_window->GetNativeHandle());
    if (!m_hwnd) {
        return Result::Error("Failed to get native window handle");
    }

    // Create system tray icon
    Result result = CreateSystemTrayIcon();
    if (!result.IsSuccess()) {
        std::cerr << "Failed to create system tray icon: " << result.GetError() << std::endl;
    }

    // Check startup registration status
    m_startupRegistered = IsRegisteredForStartup();
#endif

    return Result::Success();
}

Result WindowsIntegration::CreateSystemTrayIcon() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Create tray menu
    Result menuResult = CreateTrayMenu();
    if (!menuResult.IsSuccess()) {
        return menuResult;
    }

    // Setup tray icon data
    m_trayIcon.cbSize = sizeof(NOTIFYICONDATA);
    m_trayIcon.hWnd = m_hwnd;
    m_trayIcon.uID = 1;
    m_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_trayIcon.uCallbackMessage = WM_USER + 1;
    
    // Load tray icon (you'll need to add a tray icon resource)
    m_trayIcon.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(101)); // IDI_TRAY_ICON
    if (!m_trayIcon.hIcon) {
        // Fallback to default application icon
        m_trayIcon.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    StringCchCopy(m_trayIcon.szTip, ARRAYSIZE(m_trayIcon.szTip), TEXT("NeonGlyph - AI ASCII Visuals"));

    // Add tray icon
    if (!Shell_NotifyIcon(NIM_ADD, &m_trayIcon)) {
        return Result::Error("Failed to add system tray icon");
    }

    m_trayIconCreated = true;
    return Result::Success();
#else
    return Result::Error("System tray not supported on this platform");
#endif
}

Result WindowsIntegration::RemoveSystemTrayIcon() {
#ifdef _WIN32
    if (m_trayIconCreated) {
        Shell_NotifyIcon(NIM_DELETE, &m_trayIcon);
        m_trayIconCreated = false;
    }

    if (m_trayMenu) {
        DestroyMenu(m_trayMenu);
        m_trayMenu = nullptr;
    }
#endif
    return Result::Success();
}

void WindowsIntegration::UpdateSystemTrayTooltip(const std::string& tooltip) {
#ifdef _WIN32
    if (m_trayIconCreated) {
        StringCchCopy(m_trayIcon.szTip, ARRAYSIZE(m_trayIcon.szTip), 
                      std::wstring(tooltip.begin(), tooltip.end()).c_str());
        Shell_NotifyIcon(NIM_MODIFY, &m_trayIcon);
    }
#endif
}

void WindowsIntegration::UpdateSystemTrayMenu(AIState state) {
#ifdef _WIN32
    if (!m_trayMenu) return;

    // Update menu items based on AI state
    EnableMenuItem(m_trayMenu, 1001, MF_BYCOMMAND | 
                   (state == AIState::Running ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(m_trayMenu, 1002, MF_BYCOMMAND | 
                   (state == AIState::Stopped ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(m_trayMenu, 1003, MF_BYCOMMAND | 
                   (state == AIState::Break ? MF_GRAYED : MF_ENABLED));
#endif
}

Result WindowsIntegration::CreateTrayMenu() {
#ifdef _WIN32
    m_trayMenu = CreatePopupMenu();
    if (!m_trayMenu) {
        return Result::Error("Failed to create tray menu");
    }

    AppendMenu(m_trayMenu, MF_STRING, 1001, TEXT("Start AI Show (Alt+P)"));
    AppendMenu(m_trayMenu, MF_STRING, 1002, TEXT("Stop AI Show (Alt+S)"));
    AppendMenu(m_trayMenu, MF_STRING, 1003, TEXT("Take AI Break (Alt+B)"));
    AppendMenu(m_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_trayMenu, MF_STRING, 1004, TEXT("Toggle Fullscreen (F11)"));
    AppendMenu(m_trayMenu, MF_STRING, 1005, TEXT("Toggle Borderless"));
    AppendMenu(m_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_trayMenu, MF_STRING, 1006, TEXT("Pure Visual Mode"));
    AppendMenu(m_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_trayMenu, MF_STRING, 1007, TEXT("Settings..."));
    AppendMenu(m_trayMenu, MF_STRING, 1008, TEXT("About"));
    AppendMenu(m_trayMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_trayMenu, MF_STRING, 1009, TEXT("Exit"));

    return Result::Success();
#else
    return Result::Error("Tray menu not supported on this platform");
#endif
}

void WindowsIntegration::ShowTrayMenu() {
#ifdef _WIN32
    if (!m_trayMenu || !m_hwnd) return;

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(m_trayMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
                    pt.x, pt.y, 0, m_hwnd, nullptr);
    PostMessage(m_hwnd, WM_NULL, 0, 0);
#endif
}

Result WindowsIntegration::RegisterForStartup() {
#ifdef _WIN32
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, 
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                              0, KEY_SET_VALUE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return Result::Error("Failed to open registry key");
    }

    // Get executable path
    TCHAR szPath[MAX_PATH];
    if (!GetModuleFileName(nullptr, szPath, MAX_PATH)) {
        RegCloseKey(hKey);
        return Result::Error("Failed to get executable path");
    }

    // Add to startup
    result = RegSetValueEx(hKey, TEXT("NeonGlyph"), 0, REG_SZ,
                          (LPBYTE)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
    
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        return Result::Error("Failed to set registry value");
    }

    m_startupRegistered = true;
    return Result::Success();
#else
    return Result::Error("Startup registration not supported on this platform");
#endif
}

Result WindowsIntegration::UnregisterFromStartup() {
#ifdef _WIN32
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                              0, KEY_SET_VALUE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return Result::Error("Failed to open registry key");
    }

    // Remove from startup
    result = RegDeleteValue(hKey, TEXT("NeonGlyph"));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
        return Result::Error("Failed to delete registry value");
    }

    m_startupRegistered = false;
    return Result::Success();
#else
    return Result::Error("Startup unregistration not supported on this platform");
#endif
}

bool WindowsIntegration::IsRegisteredForStartup() const {
#ifdef _WIN32
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
                              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                              0, KEY_QUERY_VALUE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }

    TCHAR szPath[MAX_PATH];
    DWORD dwSize = sizeof(szPath);
    result = RegQueryValueEx(hKey, TEXT("NeonGlyph"), nullptr, nullptr,
                            (LPBYTE)szPath, &dwSize);
    
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
#else
    return false;
#endif
}

void WindowsIntegration::ProcessKeyboardShortcuts(int key, int mods) {
    // Handle fullscreen toggle (F11)
    if (key == KEY_F11) {
        HandleFullscreenShortcut();
        return;
    }

    // Handle escape from fullscreen
    if (key == KEY_ESC && m_currentMode == WindowMode::Fullscreen) {
        SetWindowMode(WindowMode::Windowed);
        return;
    }

    // Handle Alt key combinations
    if (mods & MOD_ALT) {
        HandleAIControlShortcuts(key);
    }
}

void WindowsIntegration::HandleFullscreenShortcut() {
    if (m_currentMode == WindowMode::Fullscreen) {
        SetWindowMode(WindowMode::Windowed);
    } else {
        SetWindowMode(WindowMode::Fullscreen);
    }
}

void WindowsIntegration::HandleAIControlShortcuts(int key) {
    // Alt+P - Start/Pause AI Show
    if (key == 'P' || key == 'p') {
        if (m_aiState == AIState::Stopped || m_aiState == AIState::Break) {
            StartAIShow();
        } else {
            TakeAIBreak();
        }
    }
    // Alt+S - Stop AI Show
    else if (key == 'S' || key == 's') {
        StopAIShow();
    }
    // Alt+B - Take AI Break
    else if (key == 'B' || key == 'b') {
        TakeAIBreak();
    }
}

void WindowsIntegration::StartAIShow() {
    m_aiState = AIState::Running;
    UpdateSystemTrayMenu(m_aiState);
    UpdateSystemTrayTooltip("NeonGlyph - AI Show Running");
    
    if (m_stateCallback) {
        m_stateCallback(m_aiState);
    }
    
    std::cout << "AI Show started" << std::endl;
}

void WindowsIntegration::StopAIShow() {
    m_aiState = AIState::Stopped;
    UpdateSystemTrayMenu(m_aiState);
    UpdateSystemTrayTooltip("NeonGlyph - AI Show Stopped");
    
    if (m_stateCallback) {
        m_stateCallback(m_aiState);
    }
    
    std::cout << "AI Show stopped" << std::endl;
}

void WindowsIntegration::TakeAIBreak() {
    m_aiState = AIState::Break;
    UpdateSystemTrayMenu(m_aiState);
    UpdateSystemTrayTooltip("NeonGlyph - AI Taking Break");
    
    if (m_stateCallback) {
        m_stateCallback(m_aiState);
    }
    
    std::cout << "AI taking break" << std::endl;
}

Result WindowsIntegration::SetWindowMode(WindowMode mode) {
    if (mode == m_currentMode) {
        return Result::Success();
    }

    switch (mode) {
        case WindowMode::Windowed:
            return ExitFullscreen();
        case WindowMode::Borderless:
            return EnterBorderless();
        case WindowMode::Fullscreen:
            return EnterFullscreen();
        default:
            return Result::Error("Invalid window mode");
    }
}

Result WindowsIntegration::EnterFullscreen() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Save current window placement
    m_windowedPlacement.length = sizeof(WINDOWPLACEMENT);
    if (!GetWindowPlacement(m_hwnd, &m_windowedPlacement)) {
        return Result::Error("Failed to get window placement");
    }

    m_wasMaximized = (m_windowedPlacement.showCmd == SW_SHOWMAXIMIZED);

    // Get window style
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);

    // Remove window decorations
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

    SetWindowLong(m_hwnd, GWL_STYLE, style);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);

    // Get monitor info
    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = {0};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(monitor, &monitorInfo);

    // Set window to fullscreen
    SetWindowPos(m_hwnd, HWND_TOP, 
                  monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                  monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                  monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                  SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    m_currentMode = WindowMode::Fullscreen;
    
    if (m_windowModeCallback) {
        m_windowModeCallback(m_currentMode);
    }

    return Result::Success();
#else
    return Result::Error("Fullscreen not supported on this platform");
#endif
}

Result WindowsIntegration::ExitFullscreen() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Restore window style
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);

    style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    exStyle |= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

    SetWindowLong(m_hwnd, GWL_STYLE, style);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);

    // Restore window placement
    SetWindowPlacement(m_hwnd, &m_windowedPlacement);
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    m_currentMode = WindowMode::Windowed;
    
    if (m_windowModeCallback) {
        m_windowModeCallback(m_currentMode);
    }

    return Result::Success();
#else
    return Result::Error("Fullscreen exit not supported on this platform");
#endif
}

Result WindowsIntegration::EnterBorderless() {
#ifdef _WIN32
    if (!m_hwnd) {
        return Result::Error("Window handle not available");
    }

    // Get window style
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);

    // Remove window decorations but keep some functionality
    style &= ~(WS_CAPTION | WS_THICKFRAME);
    style |= (WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

    SetWindowLong(m_hwnd, GWL_STYLE, style);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);

    // Set window position
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    m_currentMode = WindowMode::Borderless;
    
    if (m_windowModeCallback) {
        m_windowModeCallback(m_currentMode);
    }

    return Result::Success();
#else
    return Result::Error("Borderless mode not supported on this platform");
#endif
}

Result WindowsIntegration::ExitBorderless() {
    // Borderless exit is same as windowed mode restoration
    return SetWindowMode(WindowMode::Windowed);
}

Result WindowsIntegration::ToggleFullscreen() {
    if (m_currentMode == WindowMode::Fullscreen) {
        return SetWindowMode(WindowMode::Windowed);
    } else {
        return SetWindowMode(WindowMode::Fullscreen);
    }
}

Result WindowsIntegration::SetBorderlessMode(bool borderless) {
    if (borderless) {
        return SetWindowMode(WindowMode::Borderless);
    } else {
        return SetWindowMode(WindowMode::Windowed);
    }
}

void WindowsIntegration::SetPureVisualMode(bool enabled) {
    m_displayMode = enabled ? DisplayMode::PureVisual : DisplayMode::Normal;
    
    if (enabled) {
        std::cout << "Pure Visual Mode: HUD elements hidden from viewers" << std::endl;
        UpdateSystemTrayTooltip("NeonGlyph - Pure Visual Mode");
    } else {
        std::cout << "Normal Mode: All UI elements visible" << std::endl;
        UpdateSystemTrayTooltip("NeonGlyph - Normal Mode");
    }
}

} // namespace NeonGlyph