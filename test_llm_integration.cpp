#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Simple test to verify LLM prompt integration
int main() {
    std::cout << "Testing LLM Prompt Integration..." << std::endl;
    
    // Test loading the role prompt
    std::string role;
    {
        std::ifstream f("config/llm_role_prompt.txt", std::ios::binary);
        if (f.is_open()) {
            std::ostringstream ss;
            ss << f.rdbuf();
            role = ss.str();
            std::cout << "✓ Successfully loaded LLM role prompt (" << role.length() << " characters)" << std::endl;
        } else {
            std::cout << "✗ Failed to load LLM role prompt file" << std::endl;
            return 1;
        }
    }
    
    // Test that the prompt contains expected content
    if (role.find("ASCII animation director") != std::string::npos) {
        std::cout << "✓ Role prompt contains expected director persona" << std::endl;
    } else {
        std::cout << "✗ Role prompt missing director persona" << std::endl;
        return 1;
    }
    
    if (role.find("1080p") != std::string::npos && role.find("8K") != std::string::npos) {
        std::cout << "✓ Role prompt contains resolution scaling guidance" << std::endl;
    } else {
        std::cout << "✗ Role prompt missing resolution guidance" << std::endl;
        return 1;
    }
    
    // Test prompt prefix functionality
    std::string testPrompt = "Generate a dancing character";
    std::string fullPrompt = role.empty() ? testPrompt : (role + "\n\n" + testPrompt);
    
    std::cout << "✓ Generated full prompt (" << fullPrompt.length() << " characters)" << std::endl;
    std::cout << "✓ First 200 characters of prompt: " << fullPrompt.substr(0, 200) << "..." << std::endl;
    
    std::cout << "\nLLM Prompt Integration Test: PASSED" << std::endl;
    return 0;
}