#include "CircuitBreaker.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <exception>
#include <thread>
#include <chrono>

namespace NeonGlyph {
namespace Chaos {

// CircuitBreaker Implementation
CircuitBreaker::CircuitBreaker(const std::string& name, const CircuitBreakerConfig& config)
    : m_name(name),
      m_config(config),
      m_state(CircuitBreakerState::CLOSED) {
    ResetMetrics();
    m_metrics.stateChangeTime = GetCurrentTime();
}

CircuitBreaker::~CircuitBreaker() {
    // Cleanup
}

template<typename Func>
auto CircuitBreaker::Execute(Func&& func) -> decltype(func()) {
    if (!m_config.enabled) {
        return func();
    }
    
    // Check if we should allow the request
    if (!AllowRequest()) {
        m_metrics.rejectedRequests++;
        throw std::runtime_error("Circuit breaker is OPEN - request rejected");
    }
    
    try {
        // Execute the function
        auto result = func();
        
        // Record success
        RecordSuccess();
        
        // Call success callback if set
        if (m_successCallback) {
            m_successCallback();
        }
        
        return result;
        
    } catch (const std::exception& e) {
        // Record failure
        RecordFailure(e.what());
        
        // Call failure callback if set
        if (m_failureCallback) {
            m_failureCallback(e.what());
        }
        
        throw;
    }
}

// Explicit template instantiation for common types
template int CircuitBreaker::Execute(std::function<int()>);
template std::string CircuitBreaker::Execute(std::function<std::string()>);
template void CircuitBreaker::Execute(std::function<void()>);

bool CircuitBreaker::AllowRequest() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    CircuitBreakerState currentState = m_state.load();
    
    switch (currentState) {
        case CircuitBreakerState::CLOSED:
            return true;
            
        case CircuitBreakerState::OPEN:
            // Check if we should transition to half-open
            if (ShouldTransitionToHalfOpen()) {
                // Transition to half-open state
                const_cast<CircuitBreaker*>(this)->TransitionToState(CircuitBreakerState::HALF_OPEN);
                return true;
            }
            return false;
            
        case CircuitBreakerState::HALF_OPEN:
            // Allow limited requests to test if service recovered
            return true;
            
        default:
            return false;
    }
}

void CircuitBreaker::RecordSuccess() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_metrics.totalRequests++;
    m_metrics.successfulRequests++;
    m_metrics.lastSuccessTime = GetCurrentTime();
    
    UpdateErrorRate();
    
    CircuitBreakerState currentState = m_state.load();
    
    switch (currentState) {
        case CircuitBreakerState::CLOSED:
            // Normal operation, nothing special to do
            break;
            
        case CircuitBreakerState::HALF_OPEN:
            // Check if we should close the circuit
            if (ShouldClose()) {
                TransitionToState(CircuitBreakerState::CLOSED);
            }
            break;
            
        case CircuitBreakerState::OPEN:
            // This shouldn't happen, but handle gracefully
            break;
    }
}

void CircuitBreaker::RecordFailure(const std::string& error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_metrics.totalRequests++;
    m_metrics.failedRequests++;
    m_metrics.lastFailureTime = GetCurrentTime();
    
    UpdateErrorRate();
    
    CircuitBreakerState currentState = m_state.load();
    
    switch (currentState) {
        case CircuitBreakerState::CLOSED:
            // Check if we should open the circuit
            if (ShouldOpen()) {
                TransitionToState(CircuitBreakerState::OPEN);
            }
            break;
            
        case CircuitBreakerState::HALF_OPEN:
            // Service is still failing, go back to open
            TransitionToState(CircuitBreakerState::OPEN);
            break;
            
        case CircuitBreakerState::OPEN:
            // Already open, nothing to do
            break;
    }
}

void CircuitBreaker::RecordTimeout() {
    RecordFailure("Request timeout");
    m_metrics.timeouts++;
}

CircuitBreakerState CircuitBreaker::GetState() const {
    return m_state.load();
}

void CircuitBreaker::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    TransitionToState(CircuitBreakerState::CLOSED);
    ResetMetrics();
}

void CircuitBreaker::ForceOpen() {
    std::lock_guard<std::mutex> lock(m_mutex);
    TransitionToState(CircuitBreakerState::OPEN);
}

void CircuitBreaker::ForceClose() {
    std::lock_guard<std::mutex> lock(m_mutex);
    TransitionToState(CircuitBreakerState::CLOSED);
}

CircuitBreakerMetrics CircuitBreaker::GetMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics;
}

double CircuitBreaker::GetErrorRate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics.currentErrorRate;
}

bool CircuitBreaker::IsHealthy() const {
    return GetState() == CircuitBreakerState::CLOSED;
}

void CircuitBreaker::UpdateConfig(const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

CircuitBreakerConfig CircuitBreaker::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void CircuitBreaker::SetSuccessCallback(SuccessCallback callback) {
    m_successCallback = callback;
}

void CircuitBreaker::SetFailureCallback(FailureCallback callback) {
    m_failureCallback = callback;
}

void CircuitBreaker::SetStateChangeCallback(StateChangeCallback callback) {
    m_stateChangeCallback = callback;
}

bool CircuitBreaker::ShouldAttemptReset() const {
    return GetState() == CircuitBreakerState::OPEN && IsResetTimeoutExceeded();
}

bool CircuitBreaker::HasReachedFailureThreshold() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics.failedRequests >= m_config.failureThreshold;
}

bool CircuitBreaker::HasMetSuccessThreshold() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics.successfulRequests >= m_config.successThreshold;
}

void CircuitBreaker::Enable() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.enabled = true;
}

void CircuitBreaker::Disable() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.enabled = false;
}

bool CircuitBreaker::IsEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.enabled;
}

std::string CircuitBreaker::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::stringstream stats;
    stats << "Circuit Breaker: " << m_name << "\n";
    stats << "State: " << static_cast<int>(m_state.load()) << "\n";
    stats << "Total Requests: " << m_metrics.totalRequests.load() << "\n";
    stats << "Successful: " << m_metrics.successfulRequests.load() << "\n";
    stats << "Failed: " << m_metrics.failedRequests.load() << "\n";
    stats << "Rejected: " << m_metrics.rejectedRequests.load() << "\n";
    stats << "Timeouts: " << m_metrics.timeouts.load() << "\n";
    stats << "Error Rate: " << (m_metrics.currentErrorRate * 100) << "%\n";
    
    return stats.str();
}

std::map<std::string, double> CircuitBreaker::GetDetailedMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::map<std::string, double> metrics;
    metrics["total_requests"] = static_cast<double>(m_metrics.totalRequests.load());
    metrics["successful_requests"] = static_cast<double>(m_metrics.successfulRequests.load());
    metrics["failed_requests"] = static_cast<double>(m_metrics.failedRequests.load());
    metrics["rejected_requests"] = static_cast<double>(m_metrics.rejectedRequests.load());
    metrics["timeouts"] = static_cast<double>(m_metrics.timeouts.load());
    metrics["error_rate"] = m_metrics.currentErrorRate;
    metrics["state"] = static_cast<double>(m_state.load());
    
    return metrics;
}

// Private methods
void CircuitBreaker::TransitionToState(CircuitBreakerState newState) {
    CircuitBreakerState oldState = m_state.load();
    
    if (oldState != newState) {
        m_state.store(newState);
        m_metrics.stateChangeTime = GetCurrentTime();
        
        // Call state change callback
        if (m_stateChangeCallback) {
            m_stateChangeCallback(oldState, newState);
        }
        
        std::cout << "Circuit breaker '" << m_name << "' transitioned from " 
                  << static_cast<int>(oldState) << " to " << static_cast<int>(newState) << std::endl;
    }
}

void CircuitBreaker::UpdateErrorRate() {
    int64_t total = m_metrics.totalRequests.load();
    int64_t failed = m_metrics.failedRequests.load();
    
    if (total > 0) {
        m_metrics.currentErrorRate = static_cast<double>(failed) / static_cast<double>(total);
    } else {
        m_metrics.currentErrorRate = 0.0;
    }
}

bool CircuitBreaker::ShouldOpen() const {
    // Check failure threshold
    if (m_metrics.failedRequests >= m_config.failureThreshold) {
        return true;
    }
    
    // Check error rate threshold (minimum requests must be met)
    if (m_metrics.totalRequests >= m_config.minimumRequests && 
        m_metrics.currentErrorRate >= m_config.errorRateThreshold) {
        return true;
    }
    
    return false;
}

bool CircuitBreaker::ShouldClose() const {
    return m_metrics.successfulRequests >= m_config.successThreshold;
}

bool CircuitBreaker::ShouldTransitionToHalfOpen() const {
    return IsResetTimeoutExceeded();
}

void CircuitBreaker::ResetMetrics() {
    m_metrics.totalRequests = 0;
    m_metrics.successfulRequests = 0;
    m_metrics.failedRequests = 0;
    m_metrics.rejectedRequests = 0;
    m_metrics.timeouts = 0;
    m_metrics.currentErrorRate = 0.0;
}

std::chrono::steady_clock::time_point CircuitBreaker::GetCurrentTime() const {
    return std::chrono::steady_clock::now();
}

bool CircuitBreaker::IsTimeoutExceeded() const {
    auto now = GetCurrentTime();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_metrics.lastFailureTime);
    return elapsed >= m_config.timeout;
}

bool CircuitBreaker::IsResetTimeoutExceeded() const {
    auto now = GetCurrentTime();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_metrics.stateChangeTime);
    return elapsed >= m_config.resetTimeout;
}

// CircuitBreakerRegistry Implementation
CircuitBreakerRegistry& CircuitBreakerRegistry::GetInstance() {
    static CircuitBreakerRegistry instance;
    return instance;
}

std::shared_ptr<CircuitBreaker> CircuitBreakerRegistry::GetCircuitBreaker(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    auto it = m_circuitBreakers.find(name);
    if (it != m_circuitBreakers.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::shared_ptr<CircuitBreaker> CircuitBreakerRegistry::CreateCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    auto circuitBreaker = std::make_shared<CircuitBreaker>(name, config);
    m_circuitBreakers[name] = circuitBreaker;
    
    return circuitBreaker;
}

void CircuitBreakerRegistry::RemoveCircuitBreaker(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    m_circuitBreakers.erase(name);
}

std::vector<std::string> CircuitBreakerRegistry::GetCircuitBreakerNames() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    std::vector<std::string> names;
    for (const auto& [name, breaker] : m_circuitBreakers) {
        names.push_back(name);
    }
    
    return names;
}

std::map<std::string, CircuitBreakerMetrics> CircuitBreakerRegistry::GetAllMetrics() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    std::map<std::string, CircuitBreakerMetrics> allMetrics;
    for (const auto& [name, breaker] : m_circuitBreakers) {
        allMetrics[name] = breaker->GetMetrics();
    }
    
    return allMetrics;
}

std::map<std::string, CircuitBreakerState> CircuitBreakerRegistry::GetAllStates() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    std::map<std::string, CircuitBreakerState> allStates;
    for (const auto& [name, breaker] : m_circuitBreakers) {
        allStates[name] = breaker->GetState();
    }
    
    return allStates;
}

void CircuitBreakerRegistry::ResetAll() {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    for (auto& [name, breaker] : m_circuitBreakers) {
        breaker->Reset();
    }
}

void CircuitBreakerRegistry::EnableAll() {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    for (auto& [name, breaker] : m_circuitBreakers) {
        breaker->Enable();
    }
}

void CircuitBreakerRegistry::DisableAll() {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    for (auto& [name, breaker] : m_circuitBreakers) {
        breaker->Disable();
    }
}

std::map<std::string, bool> CircuitBreakerRegistry::AreAllHealthy() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    std::map<std::string, bool> healthStatus;
    for (const auto& [name, breaker] : m_circuitBreakers) {
        healthStatus[name] = breaker->IsHealthy();
    }
    
    return healthStatus;
}

std::vector<std::string> CircuitBreakerRegistry::GetUnhealthyCircuitBreakers() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    std::vector<std::string> unhealthy;
    for (const auto& [name, breaker] : m_circuitBreakers) {
        if (!breaker->IsHealthy()) {
            unhealthy.push_back(name);
        }
    }
    
    return unhealthy;
}

double CircuitBreakerRegistry::GetOverallHealthScore() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    if (m_circuitBreakers.empty()) {
        return 100.0;
    }
    
    int healthyCount = 0;
    for (const auto& [name, breaker] : m_circuitBreakers) {
        if (breaker->IsHealthy()) {
            healthyCount++;
        }
    }
    
    return (static_cast<double>(healthyCount) / m_circuitBreakers.size()) * 100.0;
}

void CircuitBreakerRegistry::SetDefaultConfig(const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    m_defaultConfig = config;
}

CircuitBreakerConfig CircuitBreakerRegistry::GetDefaultConfig() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    return m_defaultConfig;
}

void CircuitBreakerRegistry::UpdateAllConfigs(const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    for (auto& [name, breaker] : m_circuitBreakers) {
        breaker->UpdateConfig(config);
    }
}

// ResilientCircuitBreaker Implementation
ResilientCircuitBreaker::ResilientCircuitBreaker(const std::string& name, const CircuitBreakerConfig& config)
    : CircuitBreaker(name, config),
      m_exponentialBackoffEnabled(false),
      m_backoffMultiplier(2.0),
      m_maxBackoffDuration(std::chrono::milliseconds(60000)) {
}

template<typename Func>
auto ResilientCircuitBreaker::ExecuteWithRetry(Func&& func, int maxRetries) -> decltype(func()) {
    int attempt = 0;
    std::string lastError;
    
    while (attempt <= maxRetries) {
        try {
            return Execute(func);
        } catch (const std::exception& e) {
            lastError = e.what();
            
            if (!ShouldRetry(attempt, lastError)) {
                throw;
            }
            
            // Calculate backoff duration
            auto backoffDuration = CalculateBackoffDuration(attempt);
            std::this_thread::sleep_for(backoffDuration);
            
            attempt++;
        }
    }
    
    throw std::runtime_error("Max retries exceeded: " + lastError);
}

template<typename Func>
auto ResilientCircuitBreaker::ExecuteWithFallback(Func&& func, FallbackFunction fallback) -> decltype(func()) {
    try {
        return Execute(func);
    } catch (const std::exception& e) {
        if (fallback) {
            std::cout << "Circuit breaker fallback activated for: " << GetName() << std::endl;
            fallback();
            return decltype(func()){}; // Return default value
        }
        throw;
    }
}

void ResilientCircuitBreaker::SetRetryPolicy(RetryPolicy policy) {
    m_retryPolicy = policy;
}

void ResilientCircuitBreaker::SetFallbackFunction(FallbackFunction fallback) {
    m_fallbackFunction = fallback;
}

void ResilientCircuitBreaker::EnableExponentialBackoff(bool enabled) {
    m_exponentialBackoffEnabled = enabled;
}

void ResilientCircuitBreaker::SetBackoffMultiplier(double multiplier) {
    m_backoffMultiplier = multiplier;
}

void ResilientCircuitBreaker::SetMaxBackoffDuration(std::chrono::milliseconds maxDuration) {
    m_maxBackoffDuration = maxDuration;
}

std::chrono::milliseconds ResilientCircuitBreaker::CalculateBackoffDuration(int attempt) const {
    if (!m_exponentialBackoffEnabled) {
        return std::chrono::milliseconds(1000); // 1 second default
    }
    
    auto baseDuration = std::chrono::milliseconds(100); // 100ms base
    auto calculatedDuration = std::chrono::milliseconds(
        static_cast<long long>(baseDuration.count() * std::pow(m_backoffMultiplier, attempt))
    );
    
    return std::min(calculatedDuration, m_maxBackoffDuration);
}

bool ResilientCircuitBreaker::ShouldRetry(int attempt, const std::string& error) const {
    if (m_retryPolicy) {
        return m_retryPolicy(attempt, error);
    }
    
    // Default retry policy: retry on network errors, timeouts, etc.
    return attempt < 3 && (error.find("timeout") != std::string::npos || 
                          error.find("network") != std::string::npos ||
                          error.find("connection") != std::string::npos);
}

// Explicit template instantiation for ResilientCircuitBreaker
template int ResilientCircuitBreaker::ExecuteWithRetry(std::function<int()>, int);
template std::string ResilientCircuitBreaker::ExecuteWithRetry(std::function<std::string()>, int);
template void ResilientCircuitBreaker::ExecuteWithRetry(std::function<void()>, int);

template int ResilientCircuitBreaker::ExecuteWithFallback(std::function<int()>, FallbackFunction);
template std::string ResilientCircuitBreaker::ExecuteWithFallback(std::function<std::string()>, FallbackFunction);
template void ResilientCircuitBreaker::ExecuteWithFallback(std::function<void()>, FallbackFunction);

} // namespace Chaos
} // namespace NeonGlyph