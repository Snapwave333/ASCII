#include "Theme.h"
#include "ASCIIConverter.h"
#include "NeonGlyph.h"
#include <nlohmann/json.hpp>
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <exception>

namespace NeonGlyph {

ThemeManager::ThemeManager() {}
ThemeManager::~ThemeManager() {}

Result ThemeManager::Initialize(const Config& config) {
    m_config = config;
    return Result::Success;
}

Result ThemeManager::LoadThemeFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return Result::FileNotFound;
    std::stringstream ss; ss << f.rdbuf();
    ThemeConfig t;
    Result r = ParseJSON(ss.str(), t);
    if (r != Result::Success) return r;
    m_theme = t;
    return Result::Success;
}

Result ThemeManager::ApplyToASCII(ASCIIConverter* converter) {
    if (!converter) return Result::InvalidArgument;
    converter->SetBrightness(m_theme.brightness);
    converter->SetContrast(m_theme.contrast);
    return Result::Success;
}

Result ThemeManager::AutoDetectSystemTheme() {
    DWORD value = 1;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0; DWORD size = sizeof(DWORD);
        if (RegQueryValueExA(hKey, "AppsUseLightTheme", nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size) != ERROR_SUCCESS) {
            value = 1;
        }
        RegCloseKey(hKey);
    }
    const char* themeFile = value ? "config/themes/light.json" : "config/themes/dark.json";
    return LoadThemeFile(themeFile);
}

uint32 ThemeManager::HexToRGBA(const std::string& hex) {
    try {
        std::string h = hex;
        if (h.size() && h[0] == '#') h.erase(0,1);
        while (h.size() < 8) h += 'F';
        
        // Validate hex string length
        if (h.size() != 8) {
            std::cerr << "[ThemeManager] Error: Invalid hex color length: expected 8, got " << h.size() << " for input '" << hex << "'" << std::endl;
            return 0xFFFFFFFF; // Return white as fallback
        }
        
        // Validate hex characters
        for (char c : h) {
            if (!isxdigit(c)) {
                std::cerr << "[ThemeManager] Error: Invalid hex character: '" << c << "' in '" << h << "' (original: '" << hex << "')" << std::endl;
                return 0xFFFFFFFF; // Return white as fallback
            }
        }
        
        // Parse individual components with error handling
        uint32 r, g, b, a;
        try {
            r = std::stoul(h.substr(0,2), nullptr, 16);
            g = std::stoul(h.substr(2,2), nullptr, 16);
            b = std::stoul(h.substr(4,2), nullptr, 16);
            a = std::stoul(h.substr(6,2), nullptr, 16);
        } catch (const std::invalid_argument& e) {
            std::cerr << "[ThemeManager] Error: Invalid hex color component in '" << h << "' (original: '" << hex << "'): " << e.what() << std::endl;
            return 0xFFFFFFFF; // Return white as fallback
        } catch (const std::out_of_range& e) {
            std::cerr << "[ThemeManager] Error: Hex color component out of range in '" << h << "' (original: '" << hex << "'): " << e.what() << std::endl;
            return 0xFFFFFFFF; // Return white as fallback
        }
        
        return (a<<24) | (r<<16) | (g<<8) | b;
        
    } catch (const std::exception& e) {
        std::cerr << "[ThemeManager] Error: Unexpected exception parsing hex color '" << hex << "': " << e.what() << std::endl;
        return 0xFFFFFFFF; // Return white as fallback
    }
}

Result ThemeManager::ParseJSON(const std::string& content, ThemeConfig& out) {
    auto findVal = [&](const std::string& key) -> std::string {
        size_t k = content.find("\"" + key + "\"");
        if (k == std::string::npos) return "";
        size_t c = content.find(':', k);
        if (c == std::string::npos) return "";
        
        // Skip whitespace after colon
        size_t val_start = content.find_first_not_of(" \t\r\n", c + 1);
        if (val_start == std::string::npos) return "";
        
        // Check if value is quoted string or unquoted number
        if (content[val_start] == '"') {
            // Quoted string
            size_t q2 = content.find('"', val_start + 1);
            if (q2 == std::string::npos) return "";
            return content.substr(val_start + 1, q2 - (val_start + 1));
        } else {
            // Unquoted number - find end of value (comma, newline, or closing brace)
            size_t val_end = content.find_first_of(",\r\n}", val_start);
            if (val_end == std::string::npos) return "";
            return content.substr(val_start, val_end - val_start);
        }
    };
    
    auto safe_stof = [&](const std::string& str, float default_val) -> float {
        if (str.empty()) return default_val;
        try {
            // Trim whitespace
            size_t start = str.find_first_not_of(" \t\r\n");
            size_t end = str.find_last_not_of(" \t\r\n");
            if (start == std::string::npos || end == std::string::npos) return default_val;
            std::string trimmed = str.substr(start, end - start + 1);
            return std::stof(trimmed);
        } catch (const std::exception& e) {
            std::cerr << "[ThemeManager] Warning: Failed to parse float value '" << str << "': " << e.what() << std::endl;
            return default_val;
        }
    };
    
    auto safe_stoi = [&](const std::string& str, int default_val) -> int {
        if (str.empty()) return default_val;
        try {
            // Trim whitespace
            size_t start = str.find_first_not_of(" \t\r\n");
            size_t end = str.find_last_not_of(" \t\r\n");
            if (start == std::string::npos || end == std::string::npos) return default_val;
            std::string trimmed = str.substr(start, end - start + 1);
            return std::stoi(trimmed);
        } catch (const std::exception& e) {
            std::cerr << "[ThemeManager] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
            return default_val;
        }
    };
    
    auto safe_stoi_external = [&](const std::string& str, int default_val) -> int {
        if (str.empty()) return default_val;
        try {
            // Trim whitespace
            size_t start = str.find_first_not_of(" \t\r\n");
            size_t end = str.find_last_not_of(" \t\r\n");
            if (start == std::string::npos || end == std::string::npos) return default_val;
            std::string trimmed = str.substr(start, end - start + 1);
            return std::stoi(trimmed);
        } catch (const std::exception& e) {
            std::cerr << "[ThemeManager] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
            return default_val;
        }
    };
    
    out.name = findVal("name");
    std::string brightness = findVal("brightness");
    std::string contrast = findVal("contrast");
    out.brightness = safe_stof(brightness, 0.0f);
    out.contrast = safe_stof(contrast, 1.0f);
    out.glyph_map = findVal("glyph_map");
    size_t p = content.find("palette");
    if (p != std::string::npos) {
        auto parseColor = [&](const std::string& k){
            size_t pk = content.find("\""+k+"\"", p);
            if (pk==std::string::npos) return;
            size_t c = content.find(':', pk);
            size_t q1 = content.find('"', c+1);
            size_t q2 = content.find('"', q1+1);
            if (q1==std::string::npos || q2==std::string::npos) return;
            std::string hex = content.substr(q1+1, q2-(q1+1));
            out.palette[k] = HexToRGBA(hex);
        };
        parseColor("foreground");
        parseColor("background");
        parseColor("accent");
    }
    return Result::Success;
}

Result ThemeManager::GenerateThemeFromLLM(const std::string& name, const std::string& hint) {
#ifdef _WIN32
    // Safe string to integer conversion with error handling
    auto safe_stoi_external = [&](const std::string& str, int default_val) -> int {
        if (str.empty()) return default_val;
        try {
            // Trim whitespace
            size_t start = str.find_first_not_of(" \t\r\n");
            size_t end = str.find_last_not_of(" \t\r\n");
            if (start == std::string::npos || end == std::string::npos) return default_val;
            std::string trimmed = str.substr(start, end - start + 1);
            return std::stoi(trimmed);
        } catch (const std::exception& e) {
            std::cerr << "[ThemeManager] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
            return default_val;
        }
    };
    
    nlohmann::json prompt;
    prompt["task"] = "generate_theme";
    prompt["name"] = name;
    prompt["hint"] = hint;
    std::string role;
    {
        std::ifstream f("config/llm_role_prompt.txt", std::ios::binary);
        if (f.is_open()) { std::ostringstream ss; ss << f.rdbuf(); role = ss.str(); }
    }
    std::string promptText = role.empty() ? prompt.dump() : (role + "\n\n" + prompt.dump());
    nlohmann::json req;
    req["model"] = m_config.llm.model;
    req["prompt"] = promptText;
    req["stream"] = false;
    HINTERNET hSession = WinHttpOpen(L"NeonGlyph/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return Result::NetworkError;
    std::wstring host;
    INTERNET_PORT port = 80;
    std::string ep = m_config.llm.endpoint;
    size_t schemeEnd = ep.find("://");
    std::string hostPort = schemeEnd != std::string::npos ? ep.substr(schemeEnd + 3) : ep;
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
        host = std::wstring(hostPort.begin(), hostPort.begin() + colonPos);
        port = static_cast<INTERNET_PORT>(safe_stoi_external(hostPort.substr(colonPos + 1), 80));
    } else {
        host = std::wstring(hostPort.begin(), hostPort.end());
        port = 80;
    }
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return Result::NetworkError; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/generate",
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return Result::NetworkError; }
    std::string body = req.dump();
    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(hRequest,
                                   headers.c_str(), (DWORD)headers.size(),
                                   (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return Result::NetworkError; }
    if (!WinHttpReceiveResponse(hRequest, NULL)) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return Result::NetworkError; }
    std::string resp;
    DWORD dwSize = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        if (dwSize == 0) break;
        std::string chunk;
        chunk.resize(dwSize);
        DWORD dwDownloaded = 0;
        if (!WinHttpReadData(hRequest, &chunk[0], dwSize, &dwDownloaded)) break;
        resp.append(chunk.data(), dwDownloaded);
    } while (dwSize > 0);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    auto j = nlohmann::json::parse(resp, nullptr, false);
    if (j.is_discarded() || !j.contains("response")) return Result::ValidationFailed;
    std::string content = j["response"].get<std::string>();
    ThemeConfig t;
    Result r = ParseJSON(content, t);
    if (r != Result::Success) return r;
    m_theme = t;
    std::string path = "config/themes/generated_" + name + ".json";
    std::ofstream f(path, std::ios::trunc);
    if (f.is_open()) f << content;
    return Result::Success;
#else
    return Result::NotImplemented;
#endif
}

} // namespace NeonGlyph
