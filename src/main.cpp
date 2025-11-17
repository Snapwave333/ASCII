#include "Application.h"
#include "NeonGlyph.h"
#include <iostream>
#include <exception>
#include <csignal>

using namespace NeonGlyph;

static Application* g_app = nullptr;

void SignalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down gracefully..." << std::endl;
    if (g_app) {
        g_app->RequestExit();
    }
}

void PrintUsage(const char* programName) {
    std::cout << "NeonGlyph - AI-Driven ASCII Visual Synthesis Engine" << std::endl;
    std::cout << "Version 3.0.1 - Enhanced Headless Mode Support" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --headless, -h              Force headless mode (no window)" << std::endl;
    std::cout << "  --no-headless-fallback      Disable automatic fallback to headless mode" << std::endl;
    std::cout << "  --enable-headless-logging   Enable detailed logging when headless mode activates" << std::endl;
    std::cout << "  --config <file>             Load configuration from specified file" << std::endl;
    std::cout << "  --test-headless-fallback    Test headless fallback mechanism (forces window failure)" << std::endl;
    std::cout << "  --log-level <level>         Set log level (trace, debug, info, warning, error, fatal)" << std::endl;
    std::cout << "  --help                      Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Headless Mode:" << std::endl;
    std::cout << "  When headless mode is active, the application runs without a graphical window." << std::endl;
    std::cout << "  This is useful for server deployments, automated testing, or when display" << std::endl;
    std::cout << "  hardware is unavailable. All audio processing and AI functionality remains" << std::endl;
    std::cout << "  fully operational in headless mode." << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --headless                          # Run in headless mode" << std::endl;
    std::cout << "  " << programName << " --no-headless-fallback              # Disable automatic fallback" << std::endl;
    std::cout << "  " << programName << " --test-headless-fallback            # Test fallback mechanism" << std::endl;
    std::cout << "  " << programName << " --config myconfig.json              # Use custom config file" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "PROJECT NEON-GLYPH - AI-Driven ASCII Visual Synthesis Engine" << std::endl;
    std::cout << "Enhanced Headless Mode Support - Version 3.0.1" << std::endl;
    std::cout << "=== ENHANCED FALLBACK MECHANISMS ACTIVE ===" << std::endl;
    
    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    try {
        Application app;
        g_app = &app;
        
        // Parse command line arguments
        Result parseResult = app.ParseCommandLineArgs(argc, argv);
        if (parseResult == Result::UnsupportedOperation) {
            // Help was displayed, exit gracefully
            return 0;
        } else if (parseResult != Result::Success) {
            std::cerr << "Failed to parse command line arguments" << std::endl;
            return static_cast<int>(parseResult);
        }
        
        // Run the application
        Result result = app.Run();
        
        if (result != Result::Success) {
            std::cerr << "Application failed with error: " << static_cast<uint32_t>(result) << std::endl;
            return static_cast<int>(result);
        }
        
        std::cout << "Application terminated successfully" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return static_cast<int>(Result::Error);
    } catch (...) {
        std::cerr << "Unknown unhandled exception" << std::endl;
        return static_cast<int>(Result::Error);
    }
}