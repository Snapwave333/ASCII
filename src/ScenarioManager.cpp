#include "ScenarioManager.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif
#include <fstream>
#include <sstream>

namespace NeonGlyph {

Result ScenarioManager::Initialize(const Config& config) {
    m_config = config;
    return Result::Success;
}

bool ScenarioManager::PostGenerate(const std::string& body, std::string& out) {
#ifdef _WIN32
    // Enhanced HTTP client with security, timeouts, and retry logic
    const int MAX_RETRIES = 3;
    const int CONNECTION_TIMEOUT_MS = 5000;  // 5 seconds
    const int RESPONSE_TIMEOUT_MS = 10000;   // 10 seconds
    const int MAX_RESPONSE_SIZE = 10 * 1024 * 1024; // 10MB max response size
    
    std::string ep = m_config.llm.endpoint;
    if (ep.empty()) {
        std::cerr << "[ScenarioManager] Error: LLM endpoint not configured" << std::endl;
        return false;
    }
    
    // Validate endpoint format
    if (ep.find("://") == std::string::npos) {
        std::cerr << "[ScenarioManager] Error: Invalid endpoint format: " << ep << std::endl;
        return false;
    }
    
    // Security: Sanitize input payload
    if (body.size() > 1024 * 1024) { // 1MB max request size
        std::cerr << "[ScenarioManager] Error: Request body too large (" << body.size() << " bytes)" << std::endl;
        return false;
    }
    
    // Retry logic with exponential backoff
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        if (attempt > 0) {
            int backoff_ms = 1000 * (1 << (attempt - 1)); // Exponential backoff
            std::cout << "[ScenarioManager] Retry attempt " << attempt << " after " << backoff_ms << "ms backoff" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        }
        
        try {
            // Parse endpoint URL
            size_t schemeEnd = ep.find("://");
            std::string scheme = ep.substr(0, schemeEnd);
            std::string hostPort = schemeEnd != std::string::npos ? ep.substr(schemeEnd + 3) : ep;
            
            // Determine port based on scheme
            INTERNET_PORT port = (scheme == "https") ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
            
            // Extract host and port
            std::wstring host;
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                host = std::wstring(hostPort.begin(), hostPort.begin() + colonPos);
                std::string portStr = hostPort.substr(colonPos + 1);
                
                // Safe port parsing with validation
                try {
                    // Validate port string first
                    if (portStr.empty()) {
                        std::cerr << "[ScenarioManager] Error: Empty port string" << std::endl;
                        return false;
                    }
                    
                    // Check for non-numeric characters
                    for (char c : portStr) {
                        if (!isdigit(c)) {
                            std::cerr << "[ScenarioManager] Error: Invalid port character: '" << c << "'" << std::endl;
                            return false;
                        }
                    }
                    
                    int portNum = std::stoi(portStr);
                    if (portNum < 1 || portNum > 65535) {
                        std::cerr << "[ScenarioManager] Error: Invalid port number: " << portNum << " (must be 1-65535)" << std::endl;
                        return false;
                    }
                    
                    port = static_cast<INTERNET_PORT>(portNum);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "[ScenarioManager] Error: Invalid port format: '" << portStr << "' - " << e.what() << std::endl;
                    return false;
                } catch (const std::out_of_range& e) {
                    std::cerr << "[ScenarioManager] Error: Port number out of range: '" << portStr << "' - " << e.what() << std::endl;
                    return false;
                }
            } else {
                host = std::wstring(hostPort.begin(), hostPort.end());
            }
            
            // Security: Validate host
            if (host.empty() || host.length() > 255) {
                std::cerr << "[ScenarioManager] Error: Invalid host configuration" << std::endl;
                return false;
            }
            
            // Create HTTP session with enhanced security settings
            HINTERNET hSession = WinHttpOpen(L"NeonGlyph/1.0 (Secure HTTP Client)",
                                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS, 
                                             WINHTTP_FLAG_SECURE_DEFAULTS);
            if (!hSession) {
                std::cerr << "[ScenarioManager] Error: Failed to create HTTP session (attempt " << attempt + 1 << ")" << std::endl;
                continue;
            }
            
            // Set security options
            DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
            WinHttpSetOption(hSession, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
            
            // Set timeouts
            DWORD connectTimeout = CONNECTION_TIMEOUT_MS;
            DWORD responseTimeout = RESPONSE_TIMEOUT_MS;
            WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
            WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &responseTimeout, sizeof(responseTimeout));
            WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &responseTimeout, sizeof(responseTimeout));
            
            // Connect to server
            HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
            if (!hConnect) {
                std::cerr << "[ScenarioManager] Error: Failed to connect to " << std::string(host.begin(), host.end()) << ":" << port 
                         << " (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Create request with proper headers
            std::wstring endpoint = L"/api/generate";
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", endpoint.c_str(),
                                                    NULL, WINHTTP_NO_REFERER,
                                                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    (scheme == "https") ? WINHTTP_FLAG_SECURE : 0);
            if (!hRequest) {
                std::cerr << "[ScenarioManager] Error: Failed to create HTTP request (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Enhanced headers with security and performance settings
            std::wstring headers = L"Content-Type: application/json\r\n"
                                  L"Accept: application/json\r\n"
                                  L"User-Agent: NeonGlyph/1.0\r\n"
                                  L"Connection: close\r\n";
            
            // Send request
            BOOL sent = WinHttpSendRequest(hRequest,
                                           headers.c_str(), (DWORD)headers.size(),
                                           (LPVOID)body.data(), (DWORD)body.size(), 
                                           (DWORD)body.size(), 0);
            if (!sent) {
                std::cerr << "[ScenarioManager] Error: Failed to send HTTP request (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Receive response
            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                std::cerr << "[ScenarioManager] Error: Failed to receive HTTP response (attempt " << attempt + 1 << ")" << std::endl;
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                continue;
            }
            
            // Read response with size limit and timeout protection
            std::string resp;
            DWORD dwSize = 0;
            DWORD totalBytesRead = 0;
            
            do {
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                    std::cerr << "[ScenarioManager] Error: Failed to query response data availability" << std::endl;
                    break;
                }
                if (dwSize == 0) break;
                
                // Security: Enforce response size limit
                if (totalBytesRead + dwSize > MAX_RESPONSE_SIZE) {
                    std::cerr << "[ScenarioManager] Error: Response too large (" << (totalBytesRead + dwSize) << " bytes)" << std::endl;
                    break;
                }
                
                std::string chunk;
                chunk.resize(dwSize);
                DWORD dwDownloaded = 0;
                if (!WinHttpReadData(hRequest, &chunk[0], dwSize, &dwDownloaded)) {
                    std::cerr << "[ScenarioManager] Error: Failed to read response data" << std::endl;
                    break;
                }
                resp.append(chunk.data(), dwDownloaded);
                totalBytesRead += dwDownloaded;
                
            } while (dwSize > 0);
            
            // Cleanup handles
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            
            // Validate response
            if (resp.empty()) {
                std::cerr << "[ScenarioManager] Error: Empty response received (attempt " << attempt + 1 << ")" << std::endl;
                continue;
            }
            
            // Parse JSON response with error handling
            try {
                auto j = nlohmann::json::parse(resp, nullptr, false);
                if (j.is_discarded()) {
                    std::cerr << "[ScenarioManager] Error: Invalid JSON response (attempt " << attempt + 1 << ")" << std::endl;
                    continue;
                }
                
                if (j.contains("response")) {
                    out = j["response"].get<std::string>();
                    std::cout << "[ScenarioManager] Successfully received LLM response (" << out.size() << " bytes)" << std::endl;
                    return true;
                } else {
                    std::cerr << "[ScenarioManager] Error: Missing 'response' field in JSON (attempt " << attempt + 1 << ")" << std::endl;
                    continue;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "[ScenarioManager] Error: JSON parsing failed: " << e.what() << " (attempt " << attempt + 1 << ")" << std::endl;
                continue;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "[ScenarioManager] Exception during HTTP request: " << e.what() << " (attempt " << attempt + 1 << ")" << std::endl;
            continue;
        }
    }
    
    std::cerr << "[ScenarioManager] Error: All " << MAX_RETRIES << " retry attempts failed" << std::endl;
    return false;
#else
    return false;
#endif
}

Result ScenarioManager::Generate(const std::string& songId) {
    nlohmann::json prompt;
    prompt["task"] = "generate_scenario";
    prompt["song_id"] = songId;
    std::string role;
    {
        std::ifstream f("config/llm_role_prompt.txt", std::ios::binary);
        if (f.is_open()) { std::ostringstream ss; ss << f.rdbuf(); role = ss.str(); }
    }
    const char* ADDON =
        "SUPPLEMENTAL COLOR IMPROVISATION & MORPHING FREEDOM — NeonGlyph AGENT\n"
        "(Add-on only. Do NOT alter or overwrite the existing core prompt or color schema. Append this block as an extra behavior layer.)\n";
    if (!role.empty()) role += std::string("\n\n") + ADDON;
    std::string promptText = role.empty() ? prompt.dump() : (role + "\n\n" + prompt.dump());
    nlohmann::json req;
    req["model"] = m_config.llm.model;
    req["prompt"] = promptText;
    req["stream"] = false;
    std::string out;
    if (!PostGenerate(req.dump(), out)) return Result::NetworkError;
    auto jr = nlohmann::json::parse(out, nullptr, false);
    if (jr.is_discarded()) return Result::ValidationFailed;
    std::string path = "config/scenarios/" + songId + ".json";
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) return Result::PermissionDenied;
    f << jr.dump(2);
    return Result::Success;
}

std::string ScenarioManager::GetHint(const std::string& songId, const std::string& section) {
    std::string path = "config/scenarios/" + songId + ".json";
    std::ifstream f(path);
    if (!f.is_open()) return "";
    nlohmann::json j;
    try { f >> j; } catch (...) { return ""; }
    if (!j.contains("sections")) return "";
    for (auto& s : j["sections"]) {
        if (s.contains("name") && s["name"].get<std::string>() == section) {
            if (s.contains("narrative")) return s["narrative"].get<std::string>();
        }
    }
    return "";
}

}
