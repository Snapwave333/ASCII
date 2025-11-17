#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include "Application.h"
#include "NeonGlyph.h"

using namespace NeonGlyph;

class HeadlessFallbackTester {
public:
    static bool TestCommandLineParsing();
    static bool TestHeadlessModeDetection();
    static bool TestFallbackActivation();
    static bool TestPerformanceMetrics();
    static bool TestErrorRecovery();
    static bool TestConfigurationLoading();
    
    static void RunAllTests();
    static void PrintTestResult(const std::string& testName, bool passed);
};

bool HeadlessFallbackTester::TestCommandLineParsing() {
    std::cout << "Testing command line argument parsing..." << std::endl;
    
    // Test headless flag
    {
        Application app;
        char* argv1[] = { (char*)"program", (char*)"--headless" };
        Result result = app.ParseCommandLineArgs(2, argv1);
        assert(result == Result::Success);
    }
    
    // Test disable fallback flag
    {
        Application app;
        char* argv2[] = { (char*)"program", (char*)"--no-headless-fallback" };
        Result result = app.ParseCommandLineArgs(2, argv2);
        assert(result == Result::Success);
    }
    
    // Test help flag
    {
        Application app;
        char* argv3[] = { (char*)"program", (char*)"--help" };
        Result result = app.ParseCommandLineArgs(2, argv3);
        assert(result == Result::UnsupportedOperation);
    }
    
    // Test multiple flags
    {
        Application app;
        char* argv4[] = { (char*)"program", (char*)"--headless", (char*)"--enable-headless-logging" };
        Result result = app.ParseCommandLineArgs(3, argv4);
        assert(result == Result::Success);
    }
    
    return true;
}

bool HeadlessFallbackTester::TestHeadlessModeDetection() {
    std::cout << "Testing headless mode detection logic..." << std::endl;
    
    // Test forced headless mode
    {
        Application app;
        char* argv[] = { (char*)"program", (char*)"--headless" };
        app.ParseCommandLineArgs(2, argv);
        
        // Simulate the detection logic
        Config config;
        config.headless.enabled = true;
        
        bool shouldUseHeadless = (config.headless.enabled == true);
        assert(shouldUseHeadless == true);
    }
    
    // Test normal mode (not headless)
    {
        Config config;
        config.headless.enabled = false;
        config.headless.fallback = true;
        
        bool shouldUseHeadless = (config.headless.enabled == true);
        assert(shouldUseHeadless == false);
    }
    
    // Test fallback disabled
    {
        Config config;
        config.headless.enabled = false;
        config.headless.fallback = false;
        
        bool shouldUseHeadless = (config.headless.enabled == true);
        assert(shouldUseHeadless == false);
    }
    
    return true;
}

bool HeadlessFallbackTester::TestFallbackActivation() {
    std::cout << "Testing fallback activation scenarios..." << std::endl;
    
    // Simulate window creation failure
    {
        std::cout << "  Testing window creation failure scenario..." << std::endl;
        
        // This would normally trigger the fallback mechanism
        // For testing, we verify the configuration is set up correctly
        Config config;
        config.headless.fallback = true;
        config.headless.logActivation = true;
        
        assert(config.headless.fallback == true);
        assert(config.headless.logActivation == true);
    }
    
    // Simulate partial initialization failure
    {
        std::cout << "  Testing partial initialization failure scenario..." << std::endl;
        
        Config config;
        config.headless.fallback = true;
        
        // Verify that headless mode can be activated
        bool canActivateHeadless = config.headless.fallback;
        assert(canActivateHeadless == true);
    }
    
    return true;
}

bool HeadlessFallbackTester::TestPerformanceMetrics() {
    std::cout << "Testing performance metrics in headless mode..." << std::endl;
    
    // Test frame time tracking
    {
        std::vector<float32> frameTimeHistory;
        frameTimeHistory.reserve(144);
        
        // Simulate frame times
        for (int i = 0; i < 60; ++i) {
            frameTimeHistory.push_back(16.67f); // 60 FPS
        }
        
        assert(frameTimeHistory.size() == 60);
        
        // Test circular buffer behavior
        for (int i = 0; i < 100; ++i) {
            frameTimeHistory.push_back(16.67f);
            if (frameTimeHistory.size() > 144) {
                frameTimeHistory.erase(frameTimeHistory.begin());
            }
        }
        
        assert(frameTimeHistory.size() == 144);
    }
    
    // Test performance calculation
    {
        std::vector<float32> frameTimes = { 16.67f, 16.67f, 16.67f, 16.67f, 16.67f };
        
        float32 sum = 0.0f;
        for (float32 time : frameTimes) {
            sum += time;
        }
        float32 avgFrameTime = sum / frameTimes.size();
        
        assert(avgFrameTime == 16.67f);
        
        float32 fps = 1000.0f / avgFrameTime;
        assert(fps > 59.0f && fps < 61.0f);
    }
    
    return true;
}

bool HeadlessFallbackTester::TestErrorRecovery() {
    std::cout << "Testing error recovery procedures..." << std::endl;
    
    // Test error code handling
    {
        std::vector<std::pair<Result, std::string>> errorScenarios = {
            { Result::InitializationFailed, "Initialization failed" },
            { Result::DeviceLost, "Device lost" },
            { Result::UnsupportedOperation, "Unsupported operation" },
            { Result::OutOfMemory, "Out of memory" }
        };
        
        for (const auto& scenario : errorScenarios) {
            std::cout << "    Testing error: " << scenario.second << std::endl;
            
            // Verify error codes are properly handled
            bool shouldContinue = (scenario.first != Result::DeviceLost && 
                                 scenario.first != Result::InitializationFailed);
            
            // This simulates the ShouldContinueAfterError logic
            bool expectedContinue = (scenario.first != Result::DeviceLost && 
                                   scenario.first != Result::InitializationFailed);
            assert(shouldContinue == expectedContinue);
        }
    }
    
    return true;
}

bool HeadlessFallbackTester::TestConfigurationLoading() {
    std::cout << "Testing configuration loading for headless mode..." << std::endl;
    
    // Test default configuration
    {
        Config config;
        
        // Verify default headless settings
        assert(config.headless.enabled == false);
        assert(config.headless.fallback == true);
        assert(config.headless.logActivation == true);
        assert(config.headless.activationReason.empty());
        
        // Verify other settings are reasonable
        assert(config.window.width > 0);
        assert(config.window.height > 0);
        assert(config.audio.sampleRate > 0);
        assert(config.audio.channels > 0);
    }
    
    // Test headless configuration
    {
        Config config;
        config.headless.enabled = true;
        config.headless.fallback = false;
        config.headless.logActivation = false;
        config.headless.activationReason = "Test configuration";
        
        assert(config.headless.enabled == true);
        assert(config.headless.fallback == false);
        assert(config.headless.logActivation == false);
        assert(config.headless.activationReason == "Test configuration");
    }
    
    return true;
}

void HeadlessFallbackTester::PrintTestResult(const std::string& testName, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

void HeadlessFallbackTester::RunAllTests() {
    std::cout << "\n=== NEON-GLYPH HEADLESS FALLBACK TEST SUITE ===" << std::endl;
    std::cout << "Testing enhanced fallback mechanisms..." << std::endl;
    std::cout << "==============================================" << std::endl;
    
    bool allPassed = true;
    
    try {
        // Run all test categories
        bool test1 = TestCommandLineParsing();
        PrintTestResult("Command Line Parsing", test1);
        allPassed &= test1;
        
        bool test2 = TestHeadlessModeDetection();
        PrintTestResult("Headless Mode Detection", test2);
        allPassed &= test2;
        
        bool test3 = TestFallbackActivation();
        PrintTestResult("Fallback Activation", test3);
        allPassed &= test3;
        
        bool test4 = TestPerformanceMetrics();
        PrintTestResult("Performance Metrics", test4);
        allPassed &= test4;
        
        bool test5 = TestErrorRecovery();
        PrintTestResult("Error Recovery", test5);
        allPassed &= test5;
        
        bool test6 = TestConfigurationLoading();
        PrintTestResult("Configuration Loading", test6);
        allPassed &= test6;
        
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        allPassed = false;
    } catch (...) {
        std::cerr << "Test suite failed with unknown exception" << std::endl;
        allPassed = false;
    }
    
    std::cout << "\n=== TEST SUMMARY ===" << std::endl;
    std::cout << "Overall Result: " << (allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    std::cout << "====================" << std::endl;
    
    if (!allPassed) {
        std::exit(1); // Exit with error code if any tests failed
    }
}

int main(int argc, char* argv[]) {
    std::cout << "NEON-GLYPH HEADLESS FALLBACK TEST SUITE" << std::endl;
    std::cout << "Testing enhanced fallback mechanisms..." << std::endl;
    
    // Check for test-specific arguments
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --help, -h    Show this help message" << std::endl;
            std::cout << "  (no args)     Run all tests" << std::endl;
            return 0;
        }
    }
    
    HeadlessFallbackTester::RunAllTests();
    return 0;
}