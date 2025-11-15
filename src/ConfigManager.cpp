#include "NeonGlyph.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#endif

namespace NeonGlyph {

// Helper function to trim whitespace
static std::string Trim(const std::string& str) {
    const char* whitespace = " \t\n\r";
    size_t first = str.find_first_not_of(whitespace);
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(whitespace);
    return str.substr(first, last - first + 1);
}

// Safe string to float conversion with error handling
static float SafeStof(const std::string& str, float default_val = 0.0f) {
    try {
        std::string trimmed = Trim(str);
        if (trimmed.empty()) return default_val;
        return std::stof(trimmed);
    } catch (const std::exception& e) {
        std::cerr << "[ConfigManager] Warning: Failed to parse float value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

// Safe string to unsigned long conversion with error handling
static uint32 SafeStoul(const std::string& str, uint32 default_val = 0) {
    try {
        std::string trimmed = Trim(str);
        if (trimmed.empty()) return default_val;
        return std::stoul(trimmed);
    } catch (const std::exception& e) {
        std::cerr << "[ConfigManager] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

// Helper function to parse hex color
static uint32 ParseHexColor(const std::string& hex) {
    std::string cleanHex = hex;
    if (cleanHex[0] == '#') cleanHex = cleanHex.substr(1);
    
    // Parse as RGBA (add alpha if not present)
    if (cleanHex.length() == 6) {
        cleanHex += "FF"; // Add full alpha
    }
    
    try {
        uint32 rgba = static_cast<uint32>(std::stoul(cleanHex, nullptr, 16));
        // Convert from RRGGBBAA to AARRGGBB internal format
        uint32 rr = (rgba >> 24) & 0xFFu;
        uint32 gg = (rgba >> 16) & 0xFFu;
        uint32 bb = (rgba >> 8) & 0xFFu;
        uint32 aa = rgba & 0xFFu;
        return (aa << 24) | (rr << 16) | (gg << 8) | bb;
    } catch (...) {
        return 0xFFFFFFFF; // White fallback
    }
}

// Helper function to convert uint32 to hex string
static std::string ToHexColor(uint32 color) {
    std::stringstream ss;
    ss << "#" << std::hex << std::setfill('0') << std::setw(8) << color;
    return ss.str();
}

Result ConfigManager::LoadConfig(const std::string& filename, Config& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return Result::FileNotFound;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return ParseConfigFile(buffer.str(), config);
}

std::string ConfigManager::FindConfigFile(const std::string& filename) {
    // Try multiple possible locations for the config file
    std::vector<std::string> searchPaths = {
        filename,  // Original path
        "./" + filename,  // Relative to current directory
        "../" + filename,  // One level up
        "../../" + filename,  // Two levels up
    };
    
    // Try environment variable override
    const char* configDir = std::getenv("NEONGLYPH_CONFIG_DIR");
    if (configDir) {
        searchPaths.insert(searchPaths.begin(), std::string(configDir) + "/" + filename);
    }
    
    // Try to get executable directory
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) > 0) {
        std::string exeDir = exePath;
        size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            exeDir = exeDir.substr(0, lastSlash);
            searchPaths.push_back(exeDir + "/" + filename);
            searchPaths.push_back(exeDir + "/../" + filename);
            searchPaths.push_back(exeDir + "/config/" + filename.substr(filename.find_last_of("/\\") + 1));
        }
    }
    
    // Check each path
    for (const auto& path : searchPaths) {
        std::ifstream testFile(path);
        if (testFile.good()) {
            testFile.close();
            return path;
        }
    }
    
    return ""; // Not found
}

Result ConfigManager::SaveConfig(const std::string& filename, const Config& config) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return Result::PermissionDenied;
    }
    
    file << SerializeConfig(config);
    file.close();
    
    return Result::Success;
}

Result ConfigManager::LoadDefaultConfig(Config& config) {
    // Try to find the default config file in multiple locations
    std::string configPath = FindConfigFile("config/default.json");
    
    if (configPath.empty()) {
        // Log the error and use built-in defaults
        std::cerr << "[ConfigManager] Warning: Could not find config/default.json in any location." << std::endl;
        std::cerr << "[ConfigManager] Using built-in default configuration." << std::endl;
        
        // Initialize with default values
        config = Config(); // This will use the struct's default values
        return Result::Success; // Continue with defaults instead of failing
    }
    
    std::cout << "[ConfigManager] Loading configuration from: " << configPath << std::endl;
    return LoadConfig(configPath, config);
}

Result ConfigManager::ParseConfigFile(const std::string& content, Config& config) {
    // Simple JSON-like parser for our configuration format
    // This is a basic implementation - in production, consider using a proper JSON library
    
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;
    
    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '{') continue;
        
        // Check for section headers
        if (line.find('"') != std::string::npos && line.find(':') != std::string::npos && line.find('{') != std::string::npos) {
            size_t start = line.find('"') + 1;
            size_t end = line.find('"', start);
            if (end != std::string::npos) {
                currentSection = line.substr(start, end - start);
            }
            continue;
        }
        
        // Parse key-value pairs
        if (line.find('"') != std::string::npos && line.find(':') != std::string::npos) {
            size_t keyStart = line.find('"') + 1;
            size_t keyEnd = line.find('"', keyStart);
            size_t valueStart = line.find(':', keyEnd) + 1;
            
            if (keyEnd == std::string::npos || valueStart == std::string::npos) continue;
            
            std::string key = line.substr(keyStart, keyEnd - keyStart);
            std::string value = Trim(line.substr(valueStart));
            
            // Remove trailing comma and quotes
            if (value.back() == ',') value.pop_back();
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            // Apply configuration based on section and key
            if (currentSection == "window") {
                if (key == "width") config.window.width = SafeStoul(value);
                else if (key == "height") config.window.height = SafeStoul(value);
                else if (key == "fullscreen") config.window.fullscreen = (value == "true");
                else if (key == "vsync") config.window.vsync = (value == "true");
                else if (key == "targetFPS") config.window.targetFPS = SafeStoul(value);
            }
            else if (currentSection == "audio") {
                if (key == "sampleRate") config.audio.sampleRate = SafeStoul(value);
                else if (key == "channels") config.audio.channels = SafeStoul(value);
                else if (key == "bufferSize") config.audio.bufferSize = SafeStoul(value);
                else if (key == "deviceId") config.audio.deviceId = value;
            }
            else if (currentSection == "ascii") {
                if (key == "charset") config.ascii.charset = value;
                else if (key == "fontSize") config.ascii.fontSize = SafeStoul(value);
                else if (key == "brightness") config.ascii.brightness = SafeStof(value);
                else if (key == "contrast") config.ascii.contrast = SafeStof(value);
                else if (key == "invert") config.ascii.invert = (value == "true");
            }
            else if (currentSection == "ai") {
                if (key == "enabled") config.ai.enabled = (value == "true");
                else if (key == "sensitivity") config.ai.sensitivity = SafeStof(value);
                else if (key == "modelPath") config.ai.modelPath = value;
            }
            else if (currentSection == "safety") {
                if (key == "photosensitiveMode") config.safety.photosensitiveMode = (value == "true");
                else if (key == "luminanceThreshold") config.safety.luminanceThreshold = SafeStof(value);
                else if (key == "nsfwFilter") config.safety.nsfwFilter = (value == "true");
                else if (key == "crashRecovery") config.safety.crashRecovery = (value == "true");
            }
            else if (currentSection == "render") {
                if (key == "startupBgColor") {
                    uint32 rgba = ParseHexColor(value);
                    config.render.startupBgColor = rgba;
                } else if (key == "blackStartup") {
                    config.render.blackStartup = (value == "true");
                } else if (key == "overlayEnabled") {
                    config.render.overlayEnabled = (value == "true");
                } else if (key == "overlayPosition") {
                    config.render.overlayPosition = value;
                }
            }
        else if (currentSection == "output") {
            if (key == "spoutEnabled") config.output.spoutEnabled = (value == "true");
            else if (key == "ndiEnabled") config.output.ndiEnabled = (value == "true");
            else if (key == "spoutName") config.output.spoutName = value;
            else if (key == "ndiName") config.output.ndiName = value;
        }
        else if (currentSection == "llm") {
            if (key == "enabled") config.llm.enabled = (value == "true");
            else if (key == "endpoint") config.llm.endpoint = value;
            else if (key == "model") config.llm.model = value;
            else if (key == "maxLatencyMs") config.llm.maxLatencyMs = SafeStoul(value);
            else if (key == "liveIntervalMs") config.llm.liveIntervalMs = SafeStoul(value);
            else if (key == "safeEpilepsy") config.llm.safeEpilepsy = (value == "true");
        }
        }
    }
    
    return Result::Success;
}

std::string ConfigManager::SerializeConfig(const Config& config) {
    std::stringstream json;
    json << "{\n";
    
    // Window configuration
    json << "    \"window\": {\n";
    json << "        \"width\": " << config.window.width << ",\n";
    json << "        \"height\": " << config.window.height << ",\n";
    json << "        \"fullscreen\": " << (config.window.fullscreen ? "true" : "false") << ",\n";
    json << "        \"vsync\": " << (config.window.vsync ? "true" : "false") << ",\n";
    json << "        \"targetFPS\": " << config.window.targetFPS << "\n";
    json << "    },\n";
    
    // Audio configuration
    json << "    \"audio\": {\n";
    json << "        \"sampleRate\": " << config.audio.sampleRate << ",\n";
    json << "        \"channels\": " << config.audio.channels << ",\n";
    json << "        \"bufferSize\": " << config.audio.bufferSize << ",\n";
    json << "        \"deviceId\": \"" << config.audio.deviceId << "\"\n";
    json << "    },\n";
    
    // ASCII configuration
    json << "    \"ascii\": {\n";
    json << "        \"charset\": \"" << config.ascii.charset << "\",\n";
    json << "        \"fontSize\": " << config.ascii.fontSize << ",\n";
    json << "        \"brightness\": " << config.ascii.brightness << ",\n";
    json << "        \"contrast\": " << config.ascii.contrast << ",\n";
    json << "        \"invert\": " << (config.ascii.invert ? "true" : "false") << "\n";
    json << "    },\n";
    
    // AI configuration
    json << "    \"ai\": {\n";
    json << "        \"enabled\": " << (config.ai.enabled ? "true" : "false") << ",\n";
    json << "        \"sensitivity\": " << config.ai.sensitivity << ",\n";
    json << "        \"modelPath\": \"" << config.ai.modelPath << "\"\n";
    json << "    },\n";
    
    // Safety configuration
    json << "    \"safety\": {\n";
    json << "        \"photosensitiveMode\": " << (config.safety.photosensitiveMode ? "true" : "false") << ",\n";
    json << "        \"luminanceThreshold\": " << config.safety.luminanceThreshold << ",\n";
    json << "        \"nsfwFilter\": " << (config.safety.nsfwFilter ? "true" : "false") << ",\n";
    json << "        \"crashRecovery\": " << (config.safety.crashRecovery ? "true" : "false") << "\n";
    json << "    },\n";
    
    // Output configuration
    json << "    \"output\": {\n";
    json << "        \"spoutEnabled\": " << (config.output.spoutEnabled ? "true" : "false") << ",\n";
    json << "        \"ndiEnabled\": " << (config.output.ndiEnabled ? "true" : "false") << ",\n";
    json << "        \"spoutName\": \"" << config.output.spoutName << "\",\n";
    json << "        \"ndiName\": \"" << config.output.ndiName << "\"\n";
    json << "    },\n";

    // LLM configuration
    json << "    \"llm\": {\n";
    json << "        \"enabled\": " << (config.llm.enabled ? "true" : "false") << ",\n";
    json << "        \"endpoint\": \"" << config.llm.endpoint << "\",\n";
    json << "        \"model\": \"" << config.llm.model << "\",\n";
    json << "        \"maxLatencyMs\": " << config.llm.maxLatencyMs << ",\n";
    json << "        \"liveIntervalMs\": " << config.llm.liveIntervalMs << ",\n";
    json << "        \"safeEpilepsy\": " << (config.llm.safeEpilepsy ? "true" : "false") << "\n";
    json << "    }\n";

    json << "    ,\n\n    \"render\": {\n";
    {
        std::stringstream r;
        r << "        \"startupBgColor\": \"#" << std::hex << std::uppercase
          << ((config.render.startupBgColor >> 16) & 0xFFu)
          << ((config.render.startupBgColor >> 8) & 0xFFu)
          << (config.render.startupBgColor & 0xFFu)
          << ((config.render.startupBgColor >> 24) & 0xFFu)
          << std::nouppercase << std::dec << "\",\n";
        json << r.str();
    }
    json << "        \"blackStartup\": " << (config.render.blackStartup ? "true" : "false") << ",\n";
    json << "        \"overlayEnabled\": " << (config.render.overlayEnabled ? "true" : "false") << ",\n";
    json << "        \"overlayPosition\": \"" << config.render.overlayPosition << "\"\n";
    json << "    }\n";

    json << "}\n";
    
    return json.str();
}

// Palette management functions
Result ConfigManager::LoadPaletteConfig(const std::string& filename, std::vector<ColorPalette>& palettes) {
    // Try to find the palette config file in multiple locations
    std::string configPath = FindConfigFile(filename);
    
    if (configPath.empty()) {
        // Log the error and return empty palettes (caller should handle this)
        std::cerr << "[ConfigManager] Warning: Could not find palette config '" << filename << "' in any location." << std::endl;
        return Result::FileNotFound;
    }
    
    std::cout << "[ConfigManager] Loading palette configuration from: " << configPath << std::endl;
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return Result::FileNotFound;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return ParsePaletteConfig(buffer.str(), palettes);
}

Result ConfigManager::SavePaletteConfig(const std::string& filename, const std::vector<ColorPalette>& palettes) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return Result::PermissionDenied;
    }
    
    file << SerializePaletteConfig(palettes);
    file.close();
    
    return Result::Success;
}

Result ConfigManager::ParsePaletteConfig(const std::string& content, std::vector<ColorPalette>& palettes) {
    std::istringstream stream(content);
    std::string line;
    std::string currentPalette;
    
    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '{') continue;
        
        // Check for palette section
        if (line.find('"') != std::string::npos && line.find(':') != std::string::npos && line.find('{') != std::string::npos) {
            size_t start = line.find('"') + 1;
            size_t end = line.find('"', start);
            if (end != std::string::npos) {
                currentPalette = line.substr(start, end - start);
                palettes.emplace_back(currentPalette, std::vector<uint32>{});
            }
            continue;
        }
        
        // Parse color arrays
        if (line.find("colors") != std::string::npos && line.find('[') != std::string::npos) {
            // Find the colors array
            std::string colorsLine;
            while (std::getline(stream, colorsLine)) {
                colorsLine = Trim(colorsLine);
                if (colorsLine.find(']') != std::string::npos) break;
                
                // Parse individual colors
                if (colorsLine.find('"') != std::string::npos) {
                    size_t start = colorsLine.find('"') + 1;
                    size_t end = colorsLine.find('"', start);
                    if (end != std::string::npos) {
                        std::string colorStr = colorsLine.substr(start, end - start);
                        if (!palettes.empty()) {
                            palettes.back().colors.push_back(ParseHexColor(colorStr));
                        }
                    }
                }
            }
        }
    }
    
    return Result::Success;
}

std::string ConfigManager::SerializePaletteConfig(const std::vector<ColorPalette>& palettes) {
    std::stringstream json;
    json << "{\n";
    json << "    \"palettes\": {\n";
    
    for (size_t i = 0; i < palettes.size(); ++i) {
        const auto& palette = palettes[i];
        json << "        \"" << palette.name << "\": {\n";
        json << "            \"colors\": [";
        
        for (size_t j = 0; j < palette.colors.size(); ++j) {
            if (j > 0) json << ", ";
            json << "\"" << ToHexColor(palette.colors[j]) << "\"";
        }
        
        json << "]\n";
        json << "        }";
        if (i < palettes.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "    }\n";
    json << "}\n";
    
    return json.str();
}

// Predefined palettes for cyberpunk aesthetic
std::vector<ColorPalette> ConfigManager::GetDefaultPalettes() {
    return {
        {"cyberpunk", {
            0xFF00FF00, // Bright green
            0xFF008000, // Medium green  
            0xFF004000, // Dark green
            0xFF002000, // Very dark green
            0xFF001000, // Almost black green
            0xFF000800  // Nearly black green
        }},
        {"vaporwave", {
            0xFFFF00FF, // Magenta
            0xFF00FFFF, // Cyan
            0xFFFF69B4, // Hot pink
            0xFF4169E1, // Royal blue
            0xFF9370DB, // Medium purple
            0xFFFF1493  // Deep pink
        }},
        {"matrix", {
            0xFF00FF00, // Matrix green
            0xFF32CD32, // Lime green
            0xFF228B22, // Forest green
            0xFF006400, // Dark green
            0xFF004000, // Very dark green
            0xFF002000  // Nearly black green
        }},
        {"noir", {
            0xFFFFFFFF, // White
            0xFFCCCCCC, // Light gray
            0xFF999999, // Medium gray
            0xFF666666, // Dark gray
            0xFF333333, // Very dark gray
            0xFF000000  // Black
        }}
    };
}

} // namespace NeonGlyph
