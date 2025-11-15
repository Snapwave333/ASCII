#include "WinIntegration.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <strsafe.h>

namespace NeonGlyph {

Result WinIntegration::EnableDPIAwareness() {
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) return Result::Success;
    return Result::Error;
}

Result WinIntegration::SetAutostart(bool enable, const std::string& appPath) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return Result::PermissionDenied;
    }
    LONG res;
    if (enable) {
        res = RegSetValueExA(hKey, "NeonGlyphDirector", 0, REG_SZ, reinterpret_cast<const BYTE*>(appPath.c_str()), static_cast<DWORD>(appPath.size()+1));
    } else {
        res = RegDeleteValueA(hKey, "NeonGlyphDirector");
    }
    RegCloseKey(hKey);
    return res == ERROR_SUCCESS ? Result::Success : Result::Error;
}

TrayIcon::TrayIcon() : m_hwnd(nullptr), m_initialized(false) {}
TrayIcon::~TrayIcon() { Shutdown(); }

Result TrayIcon::Initialize(void* hwnd, const std::string& tooltip) {
    m_hwnd = hwnd;
    NOTIFYICONDATAA nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = reinterpret_cast<HWND>(hwnd);
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_APP + 1;
    nid.hIcon = LoadIconA(GetModuleHandleA(nullptr), IDI_APPLICATION);
    StringCchCopyA(nid.szTip, ARRAYSIZE(nid.szTip), tooltip.c_str());
    if (!Shell_NotifyIconA(NIM_ADD, &nid)) return Result::Error;
    m_initialized = true;
    return Result::Success;
}

void TrayIcon::Shutdown() {
    if (!m_initialized) return;
    NOTIFYICONDATAA nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = reinterpret_cast<HWND>(m_hwnd);
    nid.uID = 1;
    Shell_NotifyIconA(NIM_DELETE, &nid);
    m_initialized = false;
}

} // namespace NeonGlyph

