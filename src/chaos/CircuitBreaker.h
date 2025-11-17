#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <functional>
#include <memory>
#include <map>

namespace NeonGlyph {
namespace Chaos {

enum class CircuitBreakerState {
    CLOSED,     // Normal operation
    OPEN,       // Failing fast
    HALF_OPEN   // Testing if service recovered
};

struct CircuitBreakerConfig {
    int failureThreshold = 5;                    // Number of failures before opening
    std::chrono::milliseconds timeout = std::chrono::milliseconds(60000); // 60 seconds
    std::chrono::milliseconds resetTimeout = std::chrono::milliseconds(30000); // 30 seconds
    int successThreshold = 3;                    // Successes needed to close from half-open
    double errorRateThreshold = 0.5;             // 50% error rate threshold
    int minimumRequests = 10;                      // Minimum requests before evaluating
    bool enabled = true;
};

struct CircuitBreakerMetrics {
    std::atomic<int64_t> totalRequests{0};
    std::atomic<int64_t> successfulRequests{0};
    std::atomic<int64_t> failedRequests{0};
    std::atomic<int64_t> rejectedRequests{0};
    std::atomic<int64_t> timeouts{0};
    std::chrono::steady_clock::time_point lastFailureTime;
    std::chrono::steady_clock::time_point lastSuccessTime;
    std::chrono::steady_clock::time_point stateChangeTime;
    double currentErrorRate{0.0};
    CircuitBreakerState currentState{CircuitBreakerState::CLOSED};
};

class CircuitBreaker {
public:
    using SuccessCallback = std::function<void()>;
    using FailureCallback = std::function<void(const std::string&)>;
    using StateChangeCallback = std::function<void(CircuitBreakerState, CircuitBreakerState)>;
    
    CircuitBreaker(const std::string& name, const CircuitBreakerConfig& config);
    ~CircuitBreaker();
    
    // Core circuit breaker functionality
    template<typename Func>
    auto Execute(Func&& func) -> decltype(func());
    
    bool AllowRequest() const;
    void RecordSuccess();
    void RecordFailure(const std::string& error = "");
    void RecordTimeout();
    
    // State management
    CircuitBreakerState GetState() const;
    void Reset();
    void ForceOpen();
    void ForceClose();
    
    // Metrics and monitoring
    CircuitBreakerMetrics GetMetrics() const;
    double GetErrorRate() const;
    bool IsHealthy() const;
    std::string GetName() const { return m_name; }
    
    // Configuration
    void UpdateConfig(const CircuitBreakerConfig& config);
    CircuitBreakerConfig GetConfig() const;
    
    // Callbacks
    void SetSuccessCallback(SuccessCallback callback);
    void SetFailureCallback(FailureCallback callback);
    void SetStateChangeCallback(StateChangeCallback callback);
    
    // Health checks
    bool ShouldAttemptReset() const;
    bool HasReachedFailureThreshold() const;
    bool HasMetSuccessThreshold() const;
    
    // Advanced features
    void Enable();
    void Disable();
    bool IsEnabled() const;
    
    // Statistics
    std::string GetStatistics() const;
    std::map<std::string, double> GetDetailedMetrics() const;

private:
    std::string m_name;
    CircuitBreakerConfig m_config;
    mutable std::mutex m_mutex;
    
    std::atomic<CircuitBreakerState> m_state;
    CircuitBreakerMetrics m_metrics;
    
    // Callbacks
    SuccessCallback m_successCallback;
    FailureCallback m_failureCallback;
    StateChangeCallback m_stateChangeCallback;
    
    // Internal methods
    void TransitionToState(CircuitBreakerState newState);
    void UpdateErrorRate();
    bool ShouldOpen() const;
    bool ShouldClose() const;
    bool ShouldTransitionToHalfOpen() const;
    void ResetMetrics();
    
    // Time utilities
    std::chrono::steady_clock::time_point GetCurrentTime() const;
    bool IsTimeoutExceeded() const;
    bool IsResetTimeoutExceeded() const;
};

// Circuit breaker registry for managing multiple breakers
class CircuitBreakerRegistry {
public:
    static CircuitBreakerRegistry& GetInstance();
    
    std::shared_ptr<CircuitBreaker> GetCircuitBreaker(const std::string& name);
    std::shared_ptr<CircuitBreaker> CreateCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config);
    void RemoveCircuitBreaker(const std::string& name);
    
    std::vector<std::string> GetCircuitBreakerNames() const;
    std::map<std::string, CircuitBreakerMetrics> GetAllMetrics() const;
    std::map<std::string, CircuitBreakerState> GetAllStates() const;
    
    void ResetAll();
    void EnableAll();
    void DisableAll();
    
    // Bulk operations
    std::map<std::string, bool> AreAllHealthy() const;
    std::vector<std::string> GetUnhealthyCircuitBreakers() const;
    double GetOverallHealthScore() const;
    
    // Configuration management
    void SetDefaultConfig(const CircuitBreakerConfig& config);
    CircuitBreakerConfig GetDefaultConfig() const;
    void UpdateAllConfigs(const CircuitBreakerConfig& config);

private:
    CircuitBreakerRegistry() = default;
    ~CircuitBreakerRegistry() = default;
    CircuitBreakerRegistry(const CircuitBreakerRegistry&) = delete;
    CircuitBreakerRegistry& operator=(const CircuitBreakerRegistry&) = delete;
    
    std::map<std::string, std::shared_ptr<CircuitBreaker>> m_circuitBreakers;
    mutable std::mutex m_registryMutex;
    CircuitBreakerConfig m_defaultConfig;
};

// Advanced circuit breaker with retry logic and fallback mechanisms
class ResilientCircuitBreaker : public CircuitBreaker {
public:
    using FallbackFunction = std::function<void()>;
    using RetryPolicy = std::function<bool(int attempt, const std::string& error)>;
    
    ResilientCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config);
    
    template<typename Func>
    auto ExecuteWithRetry(Func&& func, int maxRetries = 3) -> decltype(func());
    
    template<typename Func>
    auto ExecuteWithFallback(Func&& func, FallbackFunction fallback) -> decltype(func());
    
    void SetRetryPolicy(RetryPolicy policy);
    void SetFallbackFunction(FallbackFunction fallback);
    
    // Exponential backoff
    void EnableExponentialBackoff(bool enabled);
    void SetBackoffMultiplier(double multiplier);
    void SetMaxBackoffDuration(std::chrono::milliseconds maxDuration);

private:
    FallbackFunction m_fallbackFunction;
    RetryPolicy m_retryPolicy;
    bool m_exponentialBackoffEnabled;
    double m_backoffMultiplier;
    std::chrono::milliseconds m_maxBackoffDuration;
    
    std::chrono::milliseconds CalculateBackoffDuration(int attempt) const;
    bool ShouldRetry(int attempt, const std::string& error) const;
};

} // namespace Chaos
} // namespace NeonGlyph