#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

// Include the audio engine headers
#include "include/AudioEngine.h"
#include <cmath>

using namespace NeonGlyph;

int main() {
    std::cout << "=== NeonGlyph Audio Capture Test ===" << std::endl;
    std::cout << "This test will capture system audio and display analysis" << std::endl;
    std::cout << "Start playing audio (YouTube, Spotify, etc.) and watch the output!" << std::endl;
    std::cout << std::endl;
    
    // Initialize audio engine
    AudioEngine audioEngine;
    Config cfg;
    ConfigManager cm;
    cm.LoadDefaultConfig(cfg);
    auto result = audioEngine.Initialize(cfg);
    if (result != Result::Success) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        return 1;
    }
    
    // Start audio capture
    result = audioEngine.StartCapture();
    if (result != Result::Success) {
        std::cerr << "Failed to start audio capture" << std::endl;
        return 1;
    }
    
    std::cout << "Audio capture started successfully!" << std::endl;
    std::cout << "Capturing system audio using WASAPI loopback..." << std::endl;
    std::cout << std::endl;
    
    // Run for 30 seconds to demonstrate audio analysis
    auto startTime = std::chrono::steady_clock::now();
    std::atomic<bool> running{true};
    std::atomic<float> lastAvgEnergy{0.0f};
    std::atomic<float> lastPeakEnergy{0.0f};
    
    // Display thread
    std::thread displayThread([&]() {
        while (running) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
            
            // Query analysis from AudioEngine
            float bpm = audioEngine.GetBPM();
            std::string key = audioEngine.GetMusicalKey();
            
            // Get spectrum data
            Spectrum spectrum;
            audioEngine.GetSpectrum(spectrum);
            
            // Calculate basic metrics
            float avgEnergy = 0.0f;
            float peakEnergy = 0.0f;
            if (spectrum.magnitudes && spectrum.binCount > 0) {
                for (uint32 i = 0; i < spectrum.binCount; ++i) {
                    float sample = spectrum.magnitudes[i];
                    avgEnergy += sample;
                    peakEnergy = std::max(peakEnergy, sample);
                }
                avgEnergy /= static_cast<float>(spectrum.binCount);
            }
            lastAvgEnergy.store(avgEnergy);
            lastPeakEnergy.store(peakEnergy);
            
            // Create a simple ASCII visualization
            std::cout << "\r[";
            int bars = static_cast<int>(avgEnergy * 50);
            bars = std::min(50, std::max(0, bars));
            for (int i = 0; i < 50; ++i) {
                if (i < bars) {
                    std::cout << "█";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "] ";
            std::cout << "Energy: " << std::fixed << std::setprecision(1) << (avgEnergy * 100) << "% ";
            std::cout << "BPM: " << static_cast<int>(std::round(bpm)) << " ";
            std::cout << "Key: " << key << " ";
            std::cout << "Time: " << elapsed << "s";
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Run for 30 seconds
    std::this_thread::sleep_for(std::chrono::seconds(30));
    running = false;
    
    displayThread.join();
    
    // Stop audio capture
    audioEngine.StopCapture();
    
    // Optional Supabase export (after performance)
#ifdef _WIN32
    {
        const char* enable = std::getenv("SUPABASE_ENABLE");
        const bool doExport = enable && std::string(enable) == "1";
        const char* url = std::getenv("SUPABASE_URL");
        const char* key = std::getenv("SUPABASE_ANON_KEY");
        const char* table = std::getenv("SUPABASE_TABLE");
        if (doExport && url && key && table) {
            float bpm = audioEngine.GetBPM();
            std::string keyStr = audioEngine.GetMusicalKey();
            float sc = audioEngine.GetSpectralCentroid();
            float vol = audioEngine.GetVolume();
            float rms_db = 20.0f * std::log10(std::max(vol, 1e-7f));
            float e = lastAvgEnergy.load();
            float p = lastPeakEnergy.load();
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            std::ostringstream body;
            body << '{'
                 << "\"timestamp_ms\":" << ms << ','
                 << "\"bpm\":" << static_cast<int>(std::round(bpm)) << ','
                 << "\"key\":\"" << keyStr << "\"," 
                 << "\"energy\":" << e << ','
                 << "\"peak\":" << p << ','
                 << "\"rms_level_db\":" << rms_db << ','
                 << "\"section\":\"" << "unknown" << "\"," 
                 << "\"spectral_centroid\":" << sc
                 << '}';

            auto parseHostPort = [](const std::string& u, std::wstring& host, INTERNET_PORT& port, bool& secure) {
                secure = false;
                port = 80;
                std::string s = u;
                std::string scheme;
                size_t p = s.find("://");
                if (p != std::string::npos) { scheme = s.substr(0, p); s = s.substr(p + 3); }
                if (scheme == "https") { secure = true; port = 443; }
                size_t slash = s.find('/');
                std::string hostPort = slash == std::string::npos ? s : s.substr(0, slash);
                size_t colon = hostPort.find(':');
                if (colon != std::string::npos) {
                    host = std::wstring(hostPort.begin(), hostPort.begin() + colon);
                    port = static_cast<INTERNET_PORT>(std::stoi(hostPort.substr(colon + 1)));
                } else {
                    host = std::wstring(hostPort.begin(), hostPort.end());
                }
            };

            std::wstring whost;
            INTERNET_PORT wport;
            bool secure;
            parseHostPort(url, whost, wport, secure);
            HINTERNET hSession = WinHttpOpen(L"NeonGlyph/1.0",
                                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS, 0);
            if (hSession) {
                HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), wport, 0);
                if (hConnect) {
                    std::wstring path = std::wstring(L"/rest/v1/") + std::wstring(table, table + std::strlen(table));
                    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
                    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL,
                                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
                    if (hRequest) {
                        std::wstring headers = L"Content-Type: application/json\r\n";
                        std::wstring wkey(key, key + std::strlen(key));
                        headers += L"apikey: " + wkey + L"\r\n";
                        headers += L"Authorization: Bearer " + wkey + L"\r\n";
                        headers += L"Prefer: return=minimal\r\n";
                        std::string b = body.str();
                        BOOL sent = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.size(),
                                                       (LPVOID)b.data(), (DWORD)b.size(), (DWORD)b.size(), 0);
                        if (sent) {
                            WinHttpReceiveResponse(hRequest, NULL);
                        }
                        WinHttpCloseHandle(hRequest);
                    }
                    WinHttpCloseHandle(hConnect);
                }
                WinHttpCloseHandle(hSession);
            }
        }
    }
#endif
    
    std::cout << std::endl << std::endl;
    std::cout << "Audio test completed!" << std::endl;
    std::cout << "The system successfully captured and analyzed system audio." << std::endl;
    
    return 0;
}
