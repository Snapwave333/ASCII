#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <map>
#include <queue>

namespace NeonGlyph {
namespace Chaos {

enum class RecoveryActionType {
    RESTART_SERVICE,
    RESET_CONNECTION,
    CLEAR_CACHE,
    REALLOCATE_RESOURCES,
    ROLLBACK_CONFIGURATION,
    SWITCH_TO_BACKUP,
    ESCALATE_TO_HUMAN,
    NONE
};

enum class RecoveryPriority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

struct RecoveryAction {
    std::string id;
    std::string name;
    RecoveryActionType type;
    RecoveryPriority priority;
    std::string targetComponent;
    std::string description;
    std::function<bool()> execute;
    std::function<bool()> verify;
    std::chrono::milliseconds timeout;
    int maxRetries;
    std::map<std::string, std::string> parameters;
};

struct RecoveryContext {
    std::string incidentId;
    std::string component;
    std::string failureType;
    std::chrono::steady_clock::time_point detectedAt;
    std::chrono::steady_clock::time_point resolvedAt;
    RecoveryPriority priority;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> attemptedActions;
    std::string resolutionStatus;
};

class RecoveryActionRegistry {
public:
    static RecoveryActionRegistry& GetInstance();
    
    void RegisterRecoveryAction(const RecoveryAction& action);
    void UnregisterRecoveryAction(const std::string& actionId);
    
    std::vector<RecoveryAction> GetRecoveryActions(const std::string& component, 
                                                   RecoveryPriority minPriority = RecoveryPriority::LOW) const;
    
    std::vector<RecoveryAction> GetRecoveryActionsForFailureType(const std::string& failureType) const;
    
    RecoveryAction GetRecoveryAction(const std::string& actionId) const;
    bool HasRecoveryAction(const std::string& actionId) const;
    
private:
    RecoveryActionRegistry() = default;
    mutable std::mutex m_actionsMutex;
    std::map<std::string, RecoveryAction> m_recoveryActions;
};

class RecoveryExecutor {
public:
    static RecoveryExecutor& GetInstance();
    
    struct ExecutionResult {
        bool success;
        std::string message;
        std::chrono::milliseconds executionTime;
        int retryCount;
        std::string errorDetails;
    };
    
    ExecutionResult ExecuteRecoveryAction(const RecoveryAction& action, 
                                          const RecoveryContext& context);
    
    ExecutionResult ExecuteRecoveryActionWithRetry(const RecoveryAction& action, 
                                                   const RecoveryContext& context);
    
    void CancelExecution(const std::string& actionId);
    void CancelAllExecutions();
    
    bool IsActionExecuting(const std::string& actionId) const;
    std::vector<std::string> GetExecutingActions() const;
    
private:
    RecoveryExecutor() = default;
    
    ExecutionResult ExecuteActionInternal(const RecoveryAction& action, 
                                         const RecoveryContext& context);
    bool VerifyActionResult(const RecoveryAction& action);
    
    mutable std::mutex m_executionsMutex;
    std::map<std::string, std::atomic<bool>> m_activeExecutions;
};

class RecoveryOrchestrator {
public:
    static RecoveryOrchestrator& GetInstance();
    
    struct RecoveryPlan {
        std::string planId;
        std::string incidentId;
        std::string component;
        RecoveryPriority priority;
        std::vector<RecoveryAction> actions;
        std::chrono::steady_clock::time_point createdAt;
        std::chrono::steady_clock::time_point executedAt;
        std::string status; // "PENDING", "EXECUTING", "COMPLETED", "FAILED"
        std::string result;
    };
    
    RecoveryPlan CreateRecoveryPlan(const RecoveryContext& context);
    RecoveryPlan CreateRecoveryPlanForComponent(const std::string& component, 
                                               const std::string& failureType,
                                               RecoveryPriority priority);
    
    bool ExecuteRecoveryPlan(const RecoveryPlan& plan);
    bool ExecuteRecoveryPlanAsync(const RecoveryPlan& plan);
    
    RecoveryPlan GetRecoveryPlan(const std::string& planId) const;
    std::vector<RecoveryPlan> GetRecoveryPlansForIncident(const std::string& incidentId) const;
    std::vector<RecoveryPlan> GetActiveRecoveryPlans() const;
    
    void CancelRecoveryPlan(const std::string& planId);
    void CancelAllRecoveryPlans();
    
private:
    RecoveryOrchestrator() = default;
    
    std::vector<RecoveryAction> SelectRecoveryActions(const RecoveryContext& context);
    RecoveryPriority DetermineRecoveryPriority(const RecoveryContext& context);
    bool ValidateRecoveryPlan(const RecoveryPlan& plan);
    
    mutable std::mutex m_plansMutex;
    std::map<std::string, RecoveryPlan> m_recoveryPlans;
    std::queue<std::string> m_pendingPlans;
    std::atomic<bool> m_processing{false};
    std::thread m_processingThread;
    
    void ProcessingLoop();
    void ProcessRecoveryPlan(const RecoveryPlan& plan);
};

class SelfHealingManager {
public:
    static SelfHealingManager& GetInstance();
    
    struct HealingPolicy {
        std::string name;
        std::string component;
        std::string condition; // Expression that triggers healing
        std::vector<RecoveryAction> actions;
        std::chrono::milliseconds cooldown;
        int maxExecutionsPerHour;
        bool enabled;
    };
    
    void RegisterHealingPolicy(const HealingPolicy& policy);
    void UnregisterHealingPolicy(const std::string& policyName);
    void EnableHealingPolicy(const std::string& policyName);
    void DisableHealingPolicy(const std::string& policyName);
    
    void StartSelfHealing();
    void StopSelfHealing();
    bool IsSelfHealingActive() const;
    
    void EvaluateHealingPolicies();
    std::vector<HealingPolicy> GetApplicablePolicies(const std::string& component, 
                                                    const std::string& condition) const;
    
    std::vector<HealingPolicy> GetActivePolicies() const;
    std::vector<HealingPolicy> GetPolicyExecutionHistory(const std::string& policyName, 
                                                         size_t limit = 100) const;
    
    void RecordPolicyExecution(const std::string& policyName, bool success, 
                              const std::string& details = "");
    
private:
    SelfHealingManager() = default;
    
    bool EvaluateCondition(const std::string& condition, const std::string& component);
    bool ShouldExecutePolicy(const HealingPolicy& policy, const std::string& component);
    void ExecuteHealingPolicy(const HealingPolicy& policy, const std::string& component);
    
    mutable std::mutex m_policiesMutex;
    std::map<std::string, HealingPolicy> m_healingPolicies;
    std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> m_policyExecutions;
    std::map<std::string, std::chrono::steady_clock::time_point> m_lastPolicyExecution;
    
    std::atomic<bool> m_selfHealingActive{false};
    std::thread m_healingThread;
    std::chrono::milliseconds m_evaluationInterval{5000};
    
    void HealingLoop();
};

class AutomatedRecoverySystem {
public:
    static AutomatedRecoverySystem& GetInstance();
    
    void Initialize();
    void Shutdown();
    
    // Incident detection and handling
    void ReportIncident(const std::string& component, const std::string& failureType, 
                       RecoveryPriority priority = RecoveryPriority::MEDIUM,
                       const std::map<std::string, std::string>& metadata = {});
    
    void AutoDetectAndRecover();
    
    // Recovery execution
    bool TriggerRecovery(const std::string& component, const std::string& failureType);
    bool TriggerRecoveryWithContext(const RecoveryContext& context);
    
    // Status and monitoring
    std::vector<RecoveryContext> GetActiveIncidents() const;
    std::vector<RecoveryContext> GetIncidentHistory(size_t limit = 100) const;
    RecoveryContext GetIncident(const std::string& incidentId) const;
    
    double GetRecoverySuccessRate() const;
    double GetAverageRecoveryTime() const;
    size_t GetTotalRecoveries() const;
    
    // Configuration
    void SetAutoRecoveryEnabled(bool enabled);
    bool IsAutoRecoveryEnabled() const;
    
    void SetRecoveryTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds GetRecoveryTimeout() const;
    
private:
    AutomatedRecoverySystem() = default;
    
    std::string GenerateIncidentId();
    void ProcessIncident(const RecoveryContext& context);
    void UpdateIncidentStatus(const std::string& incidentId, const std::string& status, 
                             const std::string& resolutionDetails = "");
    
    mutable std::mutex m_incidentsMutex;
    std::map<std::string, RecoveryContext> m_activeIncidents;
    std::vector<RecoveryContext> m_incidentHistory;
    std::atomic<bool> m_autoRecoveryEnabled{true};
    std::chrono::milliseconds m_recoveryTimeout{300000}; // 5 minutes
    
    static constexpr size_t MAX_HISTORY_SIZE = 1000;
};

// NeonGlyph-specific recovery actions
class NeonGlyphRecoveryActions {
public:
    // Vulkan Graphics Recovery Actions
    static RecoveryAction CreateVulkanContextRecovery();
    static RecoveryAction CreateVulkanDeviceRecovery();
    static RecoveryAction CreateVulkanSwapchainRecovery();
    static RecoveryAction CreateVulkanMemoryRecovery();
    
    // Audio Engine Recovery Actions
    static RecoveryAction CreateAudioEngineRecovery();
    static RecoveryAction CreateAudioDeviceRecovery();
    static RecoveryAction CreateAudioBufferRecovery();
    static RecoveryAction CreateAudioStreamRecovery();
    
    // AI Engine Recovery Actions
    static RecoveryAction CreateAIEngineRecovery();
    static RecoveryAction CreateAIModelRecovery();
    static RecoveryAction CreateAIInferenceRecovery();
    static RecoveryAction CreateAIMemoryRecovery();
    
    // ASCII Engine Recovery Actions
    static RecoveryAction CreateASCIIRendererRecovery();
    static RecoveryAction CreateASCIIBufferRecovery();
    static RecoveryAction CreateASCIIFontRecovery();
    static RecoveryAction CreateASCIIAnimationRecovery();
    
    // System-wide Recovery Actions
    static RecoveryAction CreateMemoryCleanupRecovery();
    static RecoveryAction CreateConfigurationReloadRecovery();
    static RecoveryAction CreateServiceRestartRecovery();
    static RecoveryAction CreateHeadlessModeRecovery();
    
    // Helper methods
    static std::vector<RecoveryAction> GetComponentRecoveryActions(const std::string& component);
    static RecoveryAction CreateEscalationAction(const std::string& component, 
                                              const std::string& failureDetails);
};

} // namespace Chaos
} // namespace NeonGlyph