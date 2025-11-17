#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <sstream>
#include "safe_conversions.h"

// Include runtime error monitor
extern void InstallRuntimeErrorMonitor();
extern void LogStofError(const std::string& str, const std::string& context);
extern void LogRuntimeError(const std::string& error);
extern void PrintRuntimeErrorSummary();

// Test function to simulate potential runtime errors
void TestStringConversions() {
    std::cout << "[RuntimeErrorTest] Testing string conversion functions..." << std::endl;
    
    // Test cases that might cause "invalid stof argument" errors
    std::vector<std::string> testStrings = {
        "123.45",      // Valid float
        "abc",         // Invalid - letters
        "",            // Empty string
        "123.45.67",   // Invalid - multiple dots
        "123abc",      // Invalid - mixed alphanumeric
        " 123.45 ",    // Valid with whitespace
        "123.45e10",   // Valid scientific notation
        "123.45e",     // Invalid - incomplete scientific notation
        "-123.45",     // Valid negative
        "+123.45",     // Valid positive
        ".123",        // Valid decimal starting with dot
        "123.",        // Valid decimal ending with dot
        "NaN",         // Invalid (unless specifically handled)
        "inf",         // Invalid (unless specifically handled)
        "Infinity",    // Invalid (unless specifically handled)
    };
    
    std::cout << "[RuntimeErrorTest] Testing std::stof conversions:" << std::endl;
    for (const auto& testStr : testStrings) {
        try {
            float result = std::stof(testStr);
            std::cout << "[RuntimeErrorTest]   SUCCESS: '" << testStr << "' -> " << result << std::endl;
        } catch (const std::invalid_argument& e) {
            std::cout << "[RuntimeErrorTest]   ERROR: '" << testStr << "' -> invalid_argument: " << e.what() << std::endl;
            LogStofError(testStr, "TestStringConversions/stof");
        } catch (const std::out_of_range& e) {
            std::cout << "[RuntimeErrorTest]   ERROR: '" << testStr << "' -> out_of_range: " << e.what() << std::endl;
            LogStofError(testStr, "TestStringConversions/stof");
        } catch (const std::exception& e) {
            std::cout << "[RuntimeErrorTest]   ERROR: '" << testStr << "' -> exception: " << e.what() << std::endl;
            LogStofError(testStr, "TestStringConversions/stof");
        }
    }
    
    std::cout << "[RuntimeErrorTest] Testing SafeStof conversions:" << std::endl;
    for (const auto& testStr : testStrings) {
        auto result = SafeConversions::SafeStof(testStr, "TestStringConversions/SafeStof");
        if (result.success) {
            std::cout << "[RuntimeErrorTest]   SUCCESS: '" << testStr << "' -> " << result.value << std::endl;
        } else {
            std::cout << "[RuntimeErrorTest]   ERROR: '" << testStr << "' -> " << result.error << std::endl;
            LogStofError(testStr, result.context);
        }
    }
    
    std::cout << "[RuntimeErrorTest] Testing hex color parsing:" << std::endl;
    std::vector<std::string> hexColors = {
        "#FF0000FF",   // Valid red
        "FF0000FF",    // Valid red without #
        "#GG0000FF",   // Invalid - G is not hex
        "#FF00",       // Invalid - too short
        "#FF0000FF00", // Invalid - too long
        "",            // Empty
        "#FFFFFFFF",   // Valid white
    };
    
    for (const auto& hex : hexColors) {
        auto result = SafeConversions::SafeHexColorParse(hex, "TestStringConversions/HexColor");
        if (result.success) {
            std::cout << "[RuntimeErrorTest]   SUCCESS: '" << hex << "' -> 0x" << std::hex << result.value << std::dec << std::endl;
        } else {
            std::cout << "[RuntimeErrorTest]   ERROR: '" << hex << "' -> " << result.error << std::endl;
            LogRuntimeError(result.context + ": " + result.error);
        }
    }
    
    std::cout << "[RuntimeErrorTest] Testing port parsing:" << std::endl;
    std::vector<std::string> ports = {
        "80",     // Valid
        "8080",   // Valid
        "0",      // Invalid - too low
        "70000",  // Invalid - too high
        "abc",    // Invalid - not numeric
        "80.5",   // Invalid - decimal
        "",       // Empty
    };
    
    for (const auto& port : ports) {
        auto result = SafeConversions::SafePortParse(port, "TestStringConversions/Port");
        if (result.success) {
            std::cout << "[RuntimeErrorTest]   SUCCESS: '" << port << "' -> " << result.value << std::endl;
        } else {
            std::cout << "[RuntimeErrorTest]   ERROR: '" << port << "' -> " << result.error << std::endl;
            LogRuntimeError(result.context + ": " + result.error);
        }
    }
}

// Test configuration file parsing
void TestConfigParsing() {
    std::cout << "[RuntimeErrorTest] Testing configuration file parsing..." << std::endl;
    
    // Create test config files with potential issues
    std::vector<std::pair<std::string, std::string>> testConfigs = {
        {"good_config.json", R"({"resolution_width": "1920", "resolution_height": "1080", "fps_target": "60.0"})"},
        {"bad_numeric_config.json", R"({"resolution_width": "1920.5", "resolution_height": "abc", "fps_target": ""})"},
        {"empty_config.json", ""},
        {"malformed_config.json", R"({"resolution_width": "1920", "resolution_height": })"},
    };
    
    for (const auto& config : testConfigs) {
        std::cout << "[RuntimeErrorTest] Testing config: " << config.first << std::endl;
        
        // Simulate parsing the config
        std::string content = config.second;
        
        // Look for numeric values that might need parsing
        size_t pos = 0;
        while ((pos = content.find("\"", pos)) != std::string::npos) {
            size_t key_end = content.find("\"", pos + 1);
            if (key_end == std::string::npos) break;
            
            size_t colon = content.find(":", key_end);
            if (colon == std::string::npos) break;
            
            size_t value_start = content.find("\"", colon);
            if (value_start == std::string::npos) break;
            
            size_t value_end = content.find("\"", value_start + 1);
            if (value_end == std::string::npos) break;
            
            std::string key = content.substr(pos + 1, key_end - pos - 1);
            std::string value = content.substr(value_start + 1, value_end - value_start - 1);
            
            if (key.find("width") != std::string::npos || key.find("height") != std::string::npos) {
                // Test integer conversion
                try {
                    int intValue = std::stoi(value);
                    std::cout << "[RuntimeErrorTest]   Integer conversion: '" << key << "' = '" << value << "' -> " << intValue << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "[RuntimeErrorTest]   Integer conversion failed: '" << key << "' = '" << value << "' -> " << e.what() << std::endl;
                    LogStofError(value, "TestConfigParsing/" + key);
                }
            } else if (key.find("fps") != std::string::npos || key.find("target") != std::string::npos) {
                // Test float conversion
                try {
                    float floatValue = std::stof(value);
                    std::cout << "[RuntimeErrorTest]   Float conversion: '" << key << "' = '" << value << "' -> " << floatValue << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "[RuntimeErrorTest]   Float conversion failed: '" << key << "' = '" << value << "' -> " << e.what() << std::endl;
                    LogStofError(value, "TestConfigParsing/" + key);
                }
            }
            
            pos = value_end + 1;
        }
    }
}

int main() {
    std::cout << "[RuntimeErrorTest] Starting runtime error analysis..." << std::endl;
    
    // Install runtime error monitoring
    InstallRuntimeErrorMonitor();
    
    try {
        // Test string conversions
        TestStringConversions();
        
        // Test configuration parsing
        TestConfigParsing();
        
        std::cout << "[RuntimeErrorTest] Testing completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "[RuntimeErrorTest] Exception during testing: " << e.what() << std::endl;
        LogRuntimeError(std::string("Test exception: ") + e.what());
    } catch (...) {
        std::cout << "[RuntimeErrorTest] Unknown exception during testing" << std::endl;
        LogRuntimeError("Unknown test exception");
    }
    
    // Print error summary
    PrintRuntimeErrorSummary();
    
    std::cout << "[RuntimeErrorTest] Runtime error analysis completed." << std::endl;
    return 0;
}