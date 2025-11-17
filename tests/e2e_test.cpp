#include <nlohmann/json.hpp>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif
#include <iostream>
#include <fstream>
#include <string>

int main() {
#ifdef _WIN32
    HINTERNET hSession = WinHttpOpen(L"e2e_test/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { std::cout << "Failed to open WinHTTP session" << std::endl; return 0; }
    HINTERNET hConnect = WinHttpConnect(hSession, L"host.docker.internal", 11434, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); std::cout << "Connect failed" << std::endl; return 0; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/tags",
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); std::cout << "OpenRequest failed" << std::endl; return 0; }
    BOOL sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!sent || !WinHttpReceiveResponse(hRequest, NULL)) {
        std::cout << "LLM backend not reachable, skipping LLM path" << std::endl;
    } else {
        DWORD dwSize = 0; std::string resp;
        do {
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;
            std::string chunk; chunk.resize(dwSize);
            DWORD dwDownloaded = 0;
            if (!WinHttpReadData(hRequest, &chunk[0], dwSize, &dwDownloaded)) break;
            resp.append(chunk.data(), dwDownloaded);
        } while (dwSize > 0);
        std::cout << "Response size: " << resp.size() << std::endl;
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
#else
    std::cout << "Windows-only e2e test" << std::endl;
#endif
    nlohmann::json j; j["test"] = "ok"; std::cout << j.dump() << std::endl;

#ifdef _WIN32
    STARTUPINFOA si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::string exe = "build64\\Release\\NeonGlyph.exe";
    BOOL ok = CreateProcessA(NULL, const_cast<char*>(exe.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (ok) {
        Sleep(3000);
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        std::ifstream in("build64/Release/staging_run.log");
        bool foundStartup = false;
        bool foundFrameTick = false;
        std::string line;
        while (std::getline(in, line)) {
            if (line.find("Startup HeroCinematic queued") != std::string::npos) foundStartup = true;
            if (line.find("RenderLoop FrameTick=") != std::string::npos) foundFrameTick = true;
        }
        std::cout << (foundStartup ? "PASS" : "FAIL") << ": first scene queued" << std::endl;
        std::cout << (foundFrameTick ? "PASS" : "FAIL") << ": render loop running" << std::endl;
    } else {
        std::cout << "FAIL: could not launch NeonGlyph.exe" << std::endl;
    }
#endif
    return 0;
}
