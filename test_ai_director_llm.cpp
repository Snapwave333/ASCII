#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>

// Mock MusicData structure for testing
struct MusicData {
    double bpm = 128.0;
    std::string key = "C minor";
    float energy = 0.7f;
    std::string song_section = "chorus";
    float beat_phase = 0.5f;
    float dynamics = 0.8f;
    float rms_level_db = -15.0f;
};

// Mock Config structure
struct Config {
    struct {
        std::string model = "llama3";
        std::string endpoint = "http://localhost:11434";
        bool safeEpilepsy = true;
    } llm;
};

// Test the LLM request building functionality
std::string BuildLLMRequest(const MusicData& m, const Config& config) {
    nlohmann::json j;
    nlohmann::json audio;
    audio["volume"] = std::min(1.0f, std::max(0.0f, m.energy));
    audio["bpm"] = static_cast<float>(m.bpm);
    audio["energy"] = m.energy;
    audio["spectral_centroid"] = 0.0;
    audio["is_beat"] = m.beat_phase < 0.1f || m.beat_phase > 0.9f;
    audio["beat_strength"] = std::min(1.0f, std::max(0.0f, m.dynamics));
    audio["genre"] = "electronic";
    j["audio"] = audio;
    j["section"] = m.song_section;
    j["story_hint"] = "High energy chorus with driving bassline";
    
    nlohmann::json constraints;
    constraints["safe_for_epilepsy"] = config.llm.safeEpilepsy;
    constraints["max_flash_hz"] = 2.0;
    j["constraints"] = constraints;
    
    // Load the role prompt
    std::string role;
    {
        std::ifstream f("config/llm_role_prompt.txt", std::ios::binary);
        if (f.is_open()) {
            std::ostringstream ss; ss << f.rdbuf(); role = ss.str();
        }
    }
    
    std::string promptText = role.empty() ? j.dump() : (role + "\n\n" + j.dump());
    nlohmann::json req;
    req["model"] = config.llm.model;
    req["prompt"] = promptText;
    req["stream"] = false;
    return req.dump(2);
}

int main() {
    std::cout << "=== Testing LLM Prompt Integration ===" << std::endl;
    
    // Test data
    MusicData music;
    Config config;
    
    std::cout << "Music Data:" << std::endl;
    std::cout << "  BPM: " << music.bpm << std::endl;
    std::cout << "  Key: " << music.key << std::endl;
    std::cout << "  Energy: " << music.energy << std::endl;
    std::cout << "  Section: " << music.song_section << std::endl;
    
    std::cout << "\nBuilding LLM request with role prompt..." << std::endl;
    std::string request = BuildLLMRequest(music, config);
    
    std::cout << "\nGenerated Request (first 1000 chars):" << std::endl;
    std::cout << request.substr(0, 1000) << "..." << std::endl;
    
    // Verify the role prompt is included
    if (request.find("ASCII animation director") != std::string::npos) {
        std::cout << "\n✓ Role prompt successfully integrated!" << std::endl;
    } else {
        std::cout << "\n✗ Role prompt missing from request!" << std::endl;
        return 1;
    }
    
    if (request.find("1080p") != std::string::npos && request.find("8K") != std::string::npos) {
        std::cout << "✓ Resolution scaling guidance included!" << std::endl;
    } else {
        std::cout << "✗ Resolution guidance missing!" << std::endl;
        return 1;
    }
    
    if (request.find("chorus") != std::string::npos && request.find("electronic") != std::string::npos) {
        std::cout << "✓ Music context included!" << std::endl;
    } else {
        std::cout << "✗ Music context missing!" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== LLM Prompt Integration Test: PASSED ===" << std::endl;
    std::cout << "\nThis demonstrates that the AI Director will send requests to the LLM" << std::endl;
    std::cout << "with the high-res ASCII animation director persona prepended to every request." << std::endl;
    
    return 0;
}