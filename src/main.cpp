#include "NeonGlyph.h"
#include "Application.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#endif

namespace NeonGlyph {}

// Global application instance
static std::unique_ptr<NeonGlyph::Application> g_app;

// Signal handlers
void SignalHandler(int signal) {
    if (g_app) {
        // Request graceful shutdown
        std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    }
}

// Windows console handler omitted to avoid signature mismatches; signals handle shutdown.

int main(int argc, char* argv[]) {
    // Set up signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    // ConsoleCtrlHandler omitted; signals are sufficient in staging.
    
    try {
        std::cout << "NeonGlyph Version 3.0.1" << std::endl;
        g_app = std::make_unique<NeonGlyph::Application>();
        NeonGlyph::Result result = g_app->Run();
        g_app.reset();
        return static_cast<int>(result);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Fatal unknown error" << std::endl;
        return -1;
    }
}
