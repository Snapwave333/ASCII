#pragma once

#include "NeonGlyph.h"
#include <string>

namespace NeonGlyph {

class WinIntegration {
public:
    static Result EnableDPIAwareness();
    static Result SetAutostart(bool enable, const std::string& appPath);
};

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();
    Result Initialize(void* hwnd, const std::string& tooltip);
    void Shutdown();
private:
    void* m_hwnd;
    bool m_initialized;
};

} // namespace NeonGlyph

