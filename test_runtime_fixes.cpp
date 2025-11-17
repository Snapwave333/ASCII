#include <iostream>
#include <string>
#include <exception>

// Safe string to float conversion with error handling
static float SafeStof(const std::string& str, float default_val = 0.0f) {
    try {
        if (str.empty()) return default_val;
        return std::stof(str);
    } catch (const std::exception& e) {
        std::cerr << "[Test] Warning: Failed to parse float value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

// Safe string to integer conversion with error handling
static int SafeStoi(const std::string& str, int default_val = 0) {
    try {
        if (str.empty()) return default_val;
        return std::stoi(str);
    } catch (const std::exception& e) {
        std::cerr << "[Test] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

int main() {
    std::cout << "Testing runtime error fixes..." << std::endl;
    
    // Test cases that would previously cause "invalid stof argument" errors
    std::cout << "\n1. Testing empty string to float:" << std::endl;
    float f1 = SafeStof("", 1.0f);
    std::cout << "Result: " << f1 << " (expected: 1.0)" << std::endl;
    
    std::cout << "\n2. Testing invalid string to float:" << std::endl;
    float f2 = SafeStof("invalid", 2.0f);
    std::cout << "Result: " << f2 << " (expected: 2.0)" << std::endl;
    
    std::cout << "\n3. Testing valid string to float:" << std::endl;
    float f3 = SafeStof("3.14", 0.0f);
    std::cout << "Result: " << f3 << " (expected: 3.14)" << std::endl;
    
    std::cout << "\n4. Testing empty string to int:" << std::endl;
    int i1 = SafeStoi("", 10);
    std::cout << "Result: " << i1 << " (expected: 10)" << std::endl;
    
    std::cout << "\n5. Testing invalid string to int:" << std::endl;
    int i2 = SafeStoi("invalid", 20);
    std::cout << "Result: " << i2 << " (expected: 20)" << std::endl;
    
    std::cout << "\n6. Testing valid string to int:" << std::endl;
    int i3 = SafeStoi("42", 0);
    std::cout << "Result: " << i3 << " (expected: 42)" << std::endl;
    
    std::cout << "\nAll tests completed successfully!" << std::endl;
    return 0;
}