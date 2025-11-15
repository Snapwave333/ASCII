/**
 * NeonGlyph Production API
 * Enhanced API endpoints with comprehensive error handling and security
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Enhanced API Response structure with comprehensive error handling
struct APIResponse {
    bool success;
    std::string message;
    int status_code;
    std::map<std::string, std::string> data;
    std::vector<std::string> errors;
    std::chrono::system_clock::time_point timestamp;
    std::string request_id;
    
    APIResponse() : success(false), status_code(500), timestamp(std::chrono::system_clock::now()) {
        // Generate unique request ID for tracking
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        request_id = "REQ_" + std::to_string(millis);
    }
};

// Enhanced API Security Manager
class APISecurityManager {
private:
    std::mutex security_mutex;
    std::map<std::string, int> rate_limit_tracker;
    std::map<std::string, std::chrono::system_clock::time_point> session_tracker;
    
public:
    // Rate limiting implementation
    bool checkRateLimit(const std::string& client_id, int max_requests = 100, int time_window_seconds = 60) {
        std::lock_guard<std::mutex> lock(security_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto window_start = now - std::chrono::seconds(time_window_seconds);
        
        // Clean up old entries
        auto it = rate_limit_tracker.begin();
        while (it != rate_limit_tracker.end()) {
            if (session_tracker[client_id] < window_start) {
                it = rate_limit_tracker.erase(it);
                session_tracker.erase(client_id);
            } else {
                ++it;
            }
        }
        
        // Check current rate
        if (rate_limit_tracker[client_id] >= max_requests) {
            return false;
        }
        
        // Increment counter
        rate_limit_tracker[client_id]++;
        session_tracker[client_id] = now;
        return true;
    }
    
    // Input validation with comprehensive sanitization
    std::string sanitizeInput(const std::string& input) {
        if (input.empty()) {
            throw std::invalid_argument("Input cannot be empty");
        }
        
        if (input.length() > 10000) {
            throw std::invalid_argument("Input too long (max 10000 characters)");
        }
        
        std::string sanitized = input;
        
        // Remove potentially dangerous characters
        const std::string dangerous_chars = "<>\"'&;\\x00\\x1a";
        for (char c : dangerous_chars) {
            sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), c), sanitized.end());
        }
        
        // Additional SQL injection prevention
        const std::vector<std::string> sql_keywords = {
            "SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", 
            "ALTER", "EXEC", "UNION", "SCRIPT", "JAVASCRIPT"
        };
        
        std::string upper_input = sanitized;
        std::transform(upper_input.begin(), upper_input.end(), upper_input.begin(), ::toupper);
        
        for (const auto& keyword : sql_keywords) {
            if (upper_input.find(keyword) != std::string::npos) {
                throw std::invalid_argument("Input contains potentially dangerous SQL keywords");
            }
        }
        
        return sanitized;
    }
    
    // Validate configuration parameters with type checking
    bool validateConfigParam(const std::string& key, const std::string& value, const std::string& expected_type) {
        try {
            if (expected_type == "boolean") {
                std::string lower_val = value;
                std::transform(lower_val.begin(), lower_val.end(), lower_val.begin(), ::tolower);
                if (lower_val != "true" && lower_val != "false") {
                    return false;
                }
            } else if (expected_type == "integer") {
                size_t pos;
                int int_val = std::stoi(value, &pos);
                if (pos != value.length()) {
                    return false;
                }
                // Additional range validation
                if (int_val < 0 || int_val > 1000000) {
                    return false;
                }
            } else if (expected_type == "float") {
                size_t pos;
                float float_val = std::stof(value, &pos);
                if (pos != value.length()) {
                    return false;
                }
                // Additional range validation
                if (float_val < 0.0f || float_val > 1000.0f) {
                    return false;
                }
            } else if (expected_type == "string") {
                // String validation - check length and content
                if (value.length() > 1000) {
                    return false;
                }
                // Use sanitization function
                sanitizeInput(value);
            }
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
};

// Enhanced Configuration API
class ConfigurationAPI {
private:
    APISecurityManager security_manager;
    std::mutex config_mutex;
    
public:
    APIResponse getConfiguration(const std::string& client_id, const std::string& config_key) {
        APIResponse response;
        
        try {
            // Security checks
            if (!security_manager.checkRateLimit(client_id)) {
                response.success = false;
                response.status_code = 429;
                response.message = "Rate limit exceeded";
                response.errors.push_back("Too many requests from client: " + client_id);
                return response;
            }
            
            // Input validation
            std::string sanitized_key = security_manager.sanitizeInput(config_key);
            
            // Simulate database query with error handling
            std::map<std::string, std::string> mock_config = {
                {"vulkan.validation.enabled", "true"},
                {"vulkan.multi_gpu.safety_mode", "true"},
                {"logging.level", "INFO"},
                {"performance.monitoring.enabled", "true"},
                {"error.tracking.enabled", "true"},
                {"ascii.font.size", "12"},
                {"ascii.quality.threshold", "0.8"},
                {"security.audit.enabled", "true"},
                {"deployment.auto_rollback.enabled", "true"},
                {"monitoring.alert.threshold.error_rate", "0.05"},
                {"monitoring.alert.threshold.response_time", "1000"}
            };
            
            auto it = mock_config.find(sanitized_key);
            if (it != mock_config.end()) {
                response.success = true;
                response.status_code = 200;
                response.message = "Configuration retrieved successfully";
                response.data["config_key"] = sanitized_key;
                response.data["config_value"] = it->second;
                response.data["config_type"] = "string"; // Would be determined from schema
            } else {
                response.success = false;
                response.status_code = 404;
                response.message = "Configuration key not found";
                response.errors.push_back("Key not found: " + sanitized_key);
            }
            
        } catch (const std::invalid_argument& e) {
            response.success = false;
            response.status_code = 400;
            response.message = "Invalid input";
            response.errors.push_back(e.what());
        } catch (const std::exception& e) {
            response.success = false;
            response.status_code = 500;
            response.message = "Internal server error";
            response.errors.push_back("Unexpected error: " + std::string(e.what()));
        }
        
        return response;
    }
    
    APIResponse updateConfiguration(const std::string& client_id, const std::string& config_key, 
                                   const std::string& config_value, const std::string& config_type) {
        APIResponse response;
        
        try {
            // Security checks
            if (!security_manager.checkRateLimit(client_id, 50)) { // Lower limit for updates
                response.success = false;
                response.status_code = 429;
                response.message = "Rate limit exceeded";
                response.errors.push_back("Too many update requests from client: " + client_id);
                return response;
            }
            
            // Input validation
            std::string sanitized_key = security_manager.sanitizeInput(config_key);
            std::string sanitized_value = security_manager.sanitizeInput(config_value);
            
            // Type validation
            if (!security_manager.validateConfigParam(sanitized_key, sanitized_value, config_type)) {
                response.success = false;
                response.status_code = 400;
                response.message = "Invalid configuration value";
                response.errors.push_back("Value '" + sanitized_value + "' is not valid for type " + config_type);
                return response;
            }
            
            // Simulate database update with transaction simulation
            std::lock_guard<std::mutex> lock(config_mutex);
            
            // In a real implementation, this would be a database transaction
            // For now, we'll simulate success
            response.success = true;
            response.status_code = 200;
            response.message = "Configuration updated successfully";
            response.data["config_key"] = sanitized_key;
            response.data["config_value"] = sanitized_value;
            response.data["config_type"] = config_type;
            response.data["updated_at"] = "2024-01-01T00:00:00Z"; // Would be actual timestamp
            
        } catch (const std::invalid_argument& e) {
            response.success = false;
            response.status_code = 400;
            response.message = "Invalid input";
            response.errors.push_back(e.what());
        } catch (const std::exception& e) {
            response.success = false;
            response.status_code = 500;
            response.message = "Internal server error";
            response.errors.push_back("Unexpected error: " + std::string(e.what()));
        }
        
        return response;
    }
};

// Enhanced Error Reporting API
class ErrorReportingAPI {
private:
    APISecurityManager security_manager;
    std::mutex error_mutex;
    
public:
    APIResponse reportError(const std::string& client_id, const std::string& error_code,
                           const std::string& error_message, const std::string& error_type,
                           const std::string& severity, const std::map<std::string, std::string>& context) {
        APIResponse response;
        
        try {
            // Security checks
            if (!security_manager.checkRateLimit(client_id, 200)) { // Higher limit for error reporting
                response.success = false;
                response.status_code = 429;
                response.message = "Rate limit exceeded";
                response.errors.push_back("Too many error reports from client: " + client_id);
                return response;
            }
            
            // Input validation
            std::string sanitized_code = security_manager.sanitizeInput(error_code);
            std::string sanitized_message = security_manager.sanitizeInput(error_message);
            std::string sanitized_type = security_manager.sanitizeInput(error_type);
            std::string sanitized_severity = security_manager.sanitizeInput(severity);
            
            // Validate severity level
            std::vector<std::string> valid_severities = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG"};
            if (std::find(valid_severities.begin(), valid_severities.end(), sanitized_severity) == valid_severities.end()) {
                response.success = false;
                response.status_code = 400;
                response.message = "Invalid severity level";
                response.errors.push_back("Severity must be one of: CRITICAL, ERROR, WARNING, INFO, DEBUG");
                return response;
            }
            
            // Simulate error storage with comprehensive logging
            std::lock_guard<std::mutex> lock(error_mutex);
            
            // Create error record (would be database insert in real implementation)
            std::string error_id = "ERR_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            
            // Log error for immediate visibility
            std::cout << "[ERROR_REPORT] " << sanitized_severity << " - " << sanitized_code << ": " 
                     << sanitized_message << " (Client: " << client_id << ", ID: " << error_id << ")" << std::endl;
            
            response.success = true;
            response.status_code = 201;
            response.message = "Error reported successfully";
            response.data["error_id"] = error_id;
            response.data["severity"] = sanitized_severity;
            response.data["reported_at"] = "2024-01-01T00:00:00Z"; // Would be actual timestamp
            
        } catch (const std::invalid_argument& e) {
            response.success = false;
            response.status_code = 400;
            response.message = "Invalid input";
            response.errors.push_back(e.what());
        } catch (const std::exception& e) {
            response.success = false;
            response.status_code = 500;
            response.message = "Internal server error";
            response.errors.push_back("Unexpected error: " + std::string(e.what()));
        }
        
        return response;
    }
    
    APIResponse getErrorSummary(const std::string& client_id, const std::string& time_range = "24h") {
        APIResponse response;
        
        try {
            // Security checks
            if (!security_manager.checkRateLimit(client_id)) {
                response.success = false;
                response.status_code = 429;
                response.message = "Rate limit exceeded";
                response.errors.push_back("Too many requests from client: " + client_id);
                return response;
            }
            
            // Input validation
            std::string sanitized_range = security_manager.sanitizeInput(time_range);
            
            // Validate time range
            std::vector<std::string> valid_ranges = {"1h", "6h", "24h", "7d", "30d"};
            if (std::find(valid_ranges.begin(), valid_ranges.end(), sanitized_range) == valid_ranges.end()) {
                response.success = false;
                response.status_code = 400;
                response.message = "Invalid time range";
                response.errors.push_back("Time range must be one of: 1h, 6h, 24h, 7d, 30d");
                return response;
            }
            
            // Simulate error summary generation
            response.success = true;
            response.status_code = 200;
            response.message = "Error summary retrieved successfully";
            
            // Mock summary data (would come from database in real implementation)
            response.data["time_range"] = sanitized_range;
            response.data["total_errors"] = "42";
            response.data["critical_errors"] = "2";
            response.data["error_errors"] = "15";
            response.data["warning_errors"] = "25";
            response.data["top_error_types"] = "VulkanInitialization, StringConversion, MemoryAccess";
            response.data["error_rate"] = "0.03"; // 3% error rate
            
        } catch (const std::invalid_argument& e) {
            response.success = false;
            response.status_code = 400;
            response.message = "Invalid input";
            response.errors.push_back(e.what());
        } catch (const std::exception& e) {
            response.success = false;
            response.status_code = 500;
            response.message = "Internal server error";
            response.errors.push_back("Unexpected error: " + std::string(e.what()));
        }
        
        return response;
    }
};

// Performance Metrics API
class PerformanceMetricsAPI {
private:
    APISecurityManager security_manager;
    std::mutex metrics_mutex;
    
public:
    APIResponse recordMetric(const std::string& client_id, const std::string& metric_name,
                            double metric_value, const std::string& metric_unit,
                            const std::string& category, const std::map<std::string, std::string>& tags) {
        APIResponse response;
        
        try {
            // Security checks
            if (!security_manager.checkRateLimit(client_id, 1000)) { // High limit for metrics
                response.success = false;
                response.status_code = 429;
                response.message = "Rate limit exceeded";
                response.errors.push_back("Too many metric reports from client: " + client_id);
                return response;
            }
            
            // Input validation
            std::string sanitized_name = security_manager.sanitizeInput(metric_name);
            std::string sanitized_unit = security_manager.sanitizeInput(metric_unit);
            std::string sanitized_category = security_manager.sanitizeInput(category);
            
            // Validate metric value
            if (metric_value < 0.0 || metric_value > 1000000.0) {
                response.success = false;
                response.status_code = 400;
                response.message = "Invalid metric value";
                response.errors.push_back("Metric value must be between 0 and 1000000");
                return response;
            }
            
            // Validate category
            std::vector<std::string> valid_categories = {"performance", "error", "usage", "system"};
            if (std::find(valid_categories.begin(), valid_categories.end(), sanitized_category) == valid_categories.end()) {
                response.success = false;
                response.status_code = 400;
                response.message = "Invalid category";
                response.errors.push_back("Category must be one of: performance, error, usage, system");
                return response;
            }
            
            // Record metric (would be database insert in real implementation)
            std::lock_guard<std::mutex> lock(metrics_mutex);
            
            std::string metric_id = "METRIC_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            
            // Log metric for immediate visibility
            std::cout << "[METRIC] " << sanitized_category << " - " << sanitized_name << ": " 
                     << metric_value << " " << sanitized_unit << " (ID: " << metric_id << ")" << std::endl;
            
            response.success = true;
            response.status_code = 201;
            response.message = "Metric recorded successfully";
            response.data["metric_id"] = metric_id;
            response.data["category"] = sanitized_category;
            response.data["recorded_at"] = "2024-01-01T00:00:00Z"; // Would be actual timestamp
            
        } catch (const std::invalid_argument& e) {
            response.success = false;
            response.status_code = 400;
            response.message = "Invalid input";
            response.errors.push_back(e.what());
        } catch (const std::exception& e) {
            response.success = false;
            response.status_code = 500;
            response.message = "Internal server error";
            response.errors.push_back("Unexpected error: " + std::string(e.what()));
        }
        
        return response;
    }
};

// Main API Gateway
class APIGateway {
private:
    ConfigurationAPI config_api;
    ErrorReportingAPI error_api;
    PerformanceMetricsAPI metrics_api;
    APISecurityManager security_manager;
    
public:
    // Health check endpoint
    APIResponse healthCheck() {
        APIResponse response;
        response.success = true;
        response.status_code = 200;
        response.message = "API Gateway is healthy";
        response.data["status"] = "healthy";
        response.data["timestamp"] = "2024-01-01T00:00:00Z";
        response.data["version"] = "1.0.0";
        response.data["services"] = "config,errors,metrics";
        return response;
    }
    
    // Get all available endpoints
    APIResponse getEndpoints() {
        APIResponse response;
        response.success = true;
        response.status_code = 200;
        response.message = "Available API endpoints";
        
        response.data["endpoints"] = "GET /health, GET /endpoints, GET /config/{key}, PUT /config/{key}, "
                                     "POST /errors/report, GET /errors/summary, POST /metrics/record";
        
        return response;
    }
};

// Global API instance
static APIGateway api_gateway;

// C-style API functions for integration with existing C++ code
extern "C" {
    
    // Initialize the API system
    void api_init() {
        std::cout << "[API] NeonGlyph Production API initialized" << std::endl;
    }
    
    // Health check
    const char* api_health_check() {
        APIResponse response = api_gateway.healthCheck();
        static std::string result;
        result = "{"success": " + std::string(response.success ? "true" : "false") + 
                ", "status_code": " + std::to_string(response.status_code) + 
                ", "message": "\"" + response.message + "\"" + "}";
        return result.c_str();
    }
    
    // Report runtime error
    const char* api_report_error(const char* client_id, const char* error_code, 
                                const char* error_message, const char* error_type, const char* severity) {
        std::map<std::string, std::string> context;
        APIResponse response = api_gateway.error_api.reportError(
            std::string(client_id), std::string(error_code), std::string(error_message), 
            std::string(error_type), std::string(severity), context);
        
        static std::string result;
        result = "{"success": " + std::string(response.success ? "true" : "false") + 
                ", "status_code": " + std::to_string(response.status_code) + 
                ", "message": "\"" + response.message + "\"" + 
                ", "error_id": "\"" + response.data["error_id"] + "\"" + "}";
        return result.c_str();
    }
    
    // Record performance metric
    const char* api_record_metric(const char* client_id, const char* metric_name,
                                 double metric_value, const char* metric_unit, const char* category) {
        std::map<std::string, std::string> tags;
        APIResponse response = api_gateway.metrics_api.recordMetric(
            std::string(client_id), std::string(metric_name), metric_value,
            std::string(metric_unit), std::string(category), tags);
        
        static std::string result;
        result = "{"success": " + std::string(response.success ? "true" : "false") + 
                ", "status_code": " + std::to_string(response.status_code) + 
                ", "message": "\"" + response.message + "\"" + 
                ", "metric_id": "\"" + response.data["metric_id"] + "\"" + "}";
        return result.c_str();
    }
    
}