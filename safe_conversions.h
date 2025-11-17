#ifndef SAFE_CONVERSIONS_H
#define SAFE_CONVERSIONS_H

#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <optional>
#include <iostream>

// Safe string conversion utilities with detailed error reporting
namespace SafeConversions {
    
    template<typename T>
    struct ConversionResult {
        bool success;
        T value;
        std::string error;
        std::string context;
        
        ConversionResult() : success(false), value(T{}), error(""), context("") {}
        ConversionResult(T val, const std::string& ctx = "") : success(true), value(val), error(""), context(ctx) {}
        ConversionResult(const std::string& err, const std::string& ctx) : success(false), value(T{}), error(err), context(ctx) {}
    };
    
    // Safe std::stof wrapper with detailed error context
    inline ConversionResult<float> SafeStof(const std::string& str, const std::string& context = "") {
        try {
            if (str.empty()) {
                return ConversionResult<float>("Empty string provided", context);
            }
            
            // Check for invalid characters
            size_t pos = 0;
            float result = std::stof(str, &pos);
            
            // Check if entire string was consumed
            if (pos != str.length()) {
                std::string remaining = str.substr(pos);
                // Allow trailing whitespace
                if (remaining.find_first_not_of(" \t\r\n") != std::string::npos) {
                    std::stringstream ss;
                    ss << "Invalid characters after number: '" << remaining << "'";
                    return ConversionResult<float>(ss.str(), context);
                }
            }
            
            return ConversionResult<float>(result, context);
            
        } catch (const std::invalid_argument& e) {
            std::stringstream ss;
            ss << "Invalid argument: '" << str << "' - " << e.what();
            return ConversionResult<float>(ss.str(), context);
        } catch (const std::out_of_range& e) {
            std::stringstream ss;
            ss << "Out of range: '" << str << "' - " << e.what();
            return ConversionResult<float>(ss.str(), context);
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Unexpected error: '" << str << "' - " << e.what();
            return ConversionResult<float>(ss.str(), context);
        }
    }
    
    // Safe std::stoi wrapper
    inline ConversionResult<int> SafeStoi(const std::string& str, const std::string& context = "") {
        try {
            if (str.empty()) {
                return ConversionResult<int>("Empty string provided", context);
            }
            
            size_t pos = 0;
            int result = std::stoi(str, &pos);
            
            if (pos != str.length()) {
                std::string remaining = str.substr(pos);
                if (remaining.find_first_not_of(" \t\r\n") != std::string::npos) {
                    std::stringstream ss;
                    ss << "Invalid characters after number: '" << remaining << "'";
                    return ConversionResult<int>(ss.str(), context);
                }
            }
            
            return ConversionResult<int>(result, context);
            
        } catch (const std::invalid_argument& e) {
            std::stringstream ss;
            ss << "Invalid argument: '" << str << "' - " << e.what();
            return ConversionResult<int>(ss.str(), context);
        } catch (const std::out_of_range& e) {
            std::stringstream ss;
            ss << "Out of range: '" << str << "' - " << e.what();
            return ConversionResult<int>(ss.str(), context);
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Unexpected error: '" << str << "' - " << e.what();
            return ConversionResult<int>(ss.str(), context);
        }
    }
    
    // Safe std::stoul wrapper for hex parsing
    inline ConversionResult<unsigned long> SafeStoul(const std::string& str, int base = 10, const std::string& context = "") {
        try {
            if (str.empty()) {
                return ConversionResult<unsigned long>("Empty string provided", context);
            }
            
            size_t pos = 0;
            unsigned long result = std::stoul(str, &pos, base);
            
            if (pos != str.length()) {
                std::string remaining = str.substr(pos);
                if (remaining.find_first_not_of(" \t\r\n") != std::string::npos) {
                    std::stringstream ss;
                    ss << "Invalid characters after number: '" << remaining << "'";
                    return ConversionResult<unsigned long>(ss.str(), context);
                }
            }
            
            return ConversionResult<unsigned long>(result, context);
            
        } catch (const std::invalid_argument& e) {
            std::stringstream ss;
            ss << "Invalid argument: '" << str << "' (base=" << base << ") - " << e.what();
            return ConversionResult<unsigned long>(ss.str(), context);
        } catch (const std::out_of_range& e) {
            std::stringstream ss;
            ss << "Out of range: '" << str << "' (base=" << base << ") - " << e.what();
            return ConversionResult<unsigned long>(ss.str(), context);
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Unexpected error: '" << str << "' (base=" << base << ") - " << e.what();
            return ConversionResult<unsigned long>(ss.str(), context);
        }
    }
    
    // Safe hex color parsing (specifically for Theme.cpp)
    inline ConversionResult<uint32_t> SafeHexColorParse(const std::string& hex, const std::string& context = "") {
        std::string h = hex;
        
        // Remove # prefix if present
        if (!h.empty() && h[0] == '#') {
            h.erase(0, 1);
        }
        
        // Pad to 8 characters with F
        while (h.size() < 8) {
            h += 'F';
        }
        
        // Validate length
        if (h.size() != 8) {
            std::stringstream ss;
            ss << "Invalid hex color length: expected 8, got " << h.size();
            return ConversionResult<uint32_t>(ss.str(), context);
        }
        
        // Validate hex characters
        for (char c : h) {
            if (!isxdigit(c)) {
                std::stringstream ss;
                ss << "Invalid hex character: '" << c << "' in '" << h << "'";
                return ConversionResult<uint32_t>(ss.str(), context);
            }
        }
        
        // Parse individual components
        auto rResult = SafeStoul(h.substr(0, 2), 16, context + " (red component)");
        auto gResult = SafeStoul(h.substr(2, 2), 16, context + " (green component)");
        auto bResult = SafeStoul(h.substr(4, 2), 16, context + " (blue component)");
        auto aResult = SafeStoul(h.substr(6, 2), 16, context + " (alpha component)");
        
        if (!rResult.success) return ConversionResult<uint32_t>(rResult.error, rResult.context);
        if (!gResult.success) return ConversionResult<uint32_t>(gResult.error, gResult.context);
        if (!bResult.success) return ConversionResult<uint32_t>(bResult.error, bResult.context);
        if (!aResult.success) return ConversionResult<uint32_t>(aResult.error, aResult.context);
        
        // Combine into RGBA format
        uint32_t result = (aResult.value << 24) | (rResult.value << 16) | (gResult.value << 8) | bResult.value;
        return ConversionResult<uint32_t>(result, context);
    }
    
    // Safe port parsing (specifically for ScenarioManager.cpp)
    inline ConversionResult<int> SafePortParse(const std::string& portStr, const std::string& context = "") {
        auto result = SafeStoi(portStr, context);
        
        if (!result.success) {
            return result;
        }
        
        // Validate port range
        if (result.value < 1 || result.value > 65535) {
            std::stringstream ss;
            ss << "Invalid port number: " << result.value << " (must be 1-65535)";
            return ConversionResult<int>(ss.str(), context);
        }
        
        return result;
    }
    
    // Helper to log conversion errors
    inline void LogConversionError(const std::string& error, const std::string& context) {
        std::cerr << "[SafeConversions] ERROR in " << context << ": " << error << std::endl;
        
        // Call the runtime error monitor if available
        extern void LogStofError(const std::string& str, const std::string& ctx);
        LogStofError(error, context);
    }
    
    // Helper to validate string before conversion
    inline bool ValidateNumericString(const std::string& str, bool allowHex = false) {
        if (str.empty()) return false;
        
        size_t start = 0;
        bool hasSign = false;
        
        // Handle sign
        if (str[0] == '+' || str[0] == '-') {
            hasSign = true;
            start = 1;
        }
        
        // Handle hex prefix
        if (allowHex && start + 2 <= str.length() && str[start] == '0' && (str[start + 1] == 'x' || str[start + 1] == 'X')) {
            start += 2;
            
            // Validate hex digits
            for (size_t i = start; i < str.length(); i++) {
                if (!isxdigit(str[i]) && !isspace(str[i])) {
                    return false;
                }
            }
            return true;
        }
        
        // Validate decimal digits
        bool hasDigit = false;
        bool hasDot = false;
        bool hasExponent = false;
        
        for (size_t i = start; i < str.length(); i++) {
            char c = str[i];
            
            if (isdigit(c)) {
                hasDigit = true;
            } else if (c == '.' && !hasDot && !hasExponent) {
                hasDot = true;
            } else if ((c == 'e' || c == 'E') && hasDigit && !hasExponent) {
                hasExponent = true;
                hasDigit = false; // Need digits after exponent
            } else if ((c == '+' || c == '-') && hasExponent && (i == start || str[i-1] == 'e' || str[i-1] == 'E')) {
                // Valid exponent sign
            } else if (isspace(c)) {
                // Allow trailing whitespace
                for (size_t j = i + 1; j < str.length(); j++) {
                    if (!isspace(str[j])) return false;
                }
                return hasDigit;
            } else {
                return false;
            }
        }
        
        return hasDigit;
    }
}

#endif // SAFE_CONVERSIONS_H