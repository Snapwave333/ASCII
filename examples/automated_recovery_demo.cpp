#include "AutomatedRecoverySystem.h"
#include "ResilienceMonitoringIntegration.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

using namespace NeonGlyph::Chaos;

class AutomatedRecoveryDemo {
public:
    AutomatedRecoveryDemo() {
        InitializeRecoverySystem();
    }
    
    ~AutomatedRecoveryDemo() {
        ShutdownRecoverySystem();
    }
    
    void RunDemonstration() {
        std::cout << "=== NeonGlyph Automated Recovery & Self-Healing Demonstration ===" << std::endl;
        
        // Start the monitoring dashboard
        std::cout << "Starting resilience monitoring dashboard on port 8081..." << std::endl;
        ResilienceMonitoringIntegration::GetInstance().StartDashboardWebServer(8081);
        
        std::cout << "Dashboard available at: http://localhost:8081" << std::endl;
        std::cout << "Recovery system monitoring available at: http://localhost:8081/recovery" << std::endl;
        
        // Demonstrate various recovery scenarios
        DemonstrateIncidentReporting();
        DemonstrateRecoveryPlanExecution();
        DemonstrateSelfHealingPolicies();
        DemonstrateAutomatedIncidentDetection();
        
        // Run continuous recovery monitoring
        std::cout << "\nSystem running with automated recovery enabled." << std::endl;
        std::cout << "Dashboard will show real-time recovery metrics." << std::endl;
        std::cout << "Press Enter to trigger manual incidents..." << std::endl;
        std::cin.get();
        
        RunManualIncidentSimulation();
    }
    
private:
    void InitializeRecoverySystem() {
        // Initialize the automated recovery system
        AutomatedRecoverySystem::GetInstance().Initialize();
        
        // Configure self-healing policies
        ConfigureSelfHealingPolicies();
        
        // Register custom recovery actions
        RegisterCustomRecoveryActions();
        
        // Start automated incident detection
        StartAutomatedIncidentDetection();
        
        std::cout << "Automated recovery system initialized successfully." << std::endl;
    }
    
    void ShutdownRecoverySystem() {
        StopAutomatedIncidentDetection();
        AutomatedRecoverySystem::GetInstance().Shutdown();
        std::cout << "Automated recovery system shutdown complete." << std::endl;
    }
    
    void ConfigureSelfHealingPolicies() {
        auto& selfHealingManager = SelfHealingManager::GetInstance();
        
        // Vulkan Health Policy
        SelfHealingManager::HealingPolicy vulkanPolicy;
        vulkanPolicy.name = "vulkan_health_policy";
        vulkanPolicy.component = "VulkanContext";
        vulkanPolicy.condition = "health_status != HEALTHY";
        vulkanPolicy.cooldown = std::chrono::minutes(5);
        vulkanPolicy.maxExecutionsPerHour = 6;
        vulkanPolicy.enabled = true;
        vulkanPolicy.actions = {
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("vulkan_context_recovery"),
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("vulkan_memory_recovery")
        };
        selfHealingManager.RegisterHealingPolicy(vulkanPolicy);
        
        // Audio Health Policy
        SelfHealingManager::HealingPolicy audioPolicy;
        audioPolicy.name = "audio_health_policy";
        audioPolicy.component = "AudioEngine";
        audioPolicy.condition = "health_status == UNHEALTHY OR health_status == CRITICAL";
        audioPolicy.cooldown = std::chrono::minutes(3);
        audioPolicy.maxExecutionsPerHour = 4;
        audioPolicy.enabled = true;
        audioPolicy.actions = {
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("audio_engine_recovery"),
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("audio_device_recovery")
        };
        selfHealingManager.RegisterHealingPolicy(audioPolicy);
        
        // AI Health Policy
        SelfHealingManager::HealingPolicy aiPolicy;
        aiPolicy.name = "ai_health_policy";
        aiPolicy.component = "AIEngine";
        aiPolicy.condition = "health_status == CRITICAL";
        aiPolicy.cooldown = std::chrono::minutes(10);
        aiPolicy.maxExecutionsPerHour = 2;
        aiPolicy.enabled = true;
        aiPolicy.actions = {
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("ai_engine_recovery"),
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("ai_model_recovery")
        };
        selfHealingManager.RegisterHealingPolicy(aiPolicy);
        
        // Memory Health Policy
        SelfHealingManager::HealingPolicy memoryPolicy;
        memoryPolicy.name = "memory_health_policy";
        memoryPolicy.component = "Memory";
        memoryPolicy.condition = "health_status != HEALTHY";
        memoryPolicy.cooldown = std::chrono::minutes(2);
        memoryPolicy.maxExecutionsPerHour = 12;
        memoryPolicy.enabled = true;
        memoryPolicy.actions = {
            RecoveryActionRegistry::GetInstance().GetRecoveryAction("memory_cleanup_recovery")
        };
        selfHealingManager.RegisterHealingPolicy(memoryPolicy);
        
        std::cout << "Configured " << selfHealingManager.GetActivePolicies().size() 
                  << " self-healing policies." << std::endl;
    }
    
    void RegisterCustomRecoveryActions() {
        auto& registry = RecoveryActionRegistry::GetInstance();
        
        // Custom recovery actions for demonstration
        RecoveryAction customVulkanAction;
        customVulkanAction.id = "custom_vulkan_optimization";
        customVulkanAction.name = "Vulkan Performance Optimization";
        customVulkanAction.type = RecoveryActionType::REALLOCATE_RESOURCES;
        customVulkanAction.priority = RecoveryPriority::MEDIUM;
        customVulkanAction.targetComponent = "VulkanContext";
        customVulkanAction.description = "Optimizes Vulkan resource allocation for better performance";
        customVulkanAction.timeout = std::chrono::milliseconds(15000);
        customVulkanAction.maxRetries = 1;
        customVulkanAction.execute = []() {
            std::cout << "Executing Vulkan performance optimization..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            return true;
        };
        customVulkanAction.verify = []() {
            // Verify optimization succeeded
            return true;
        };
        registry.RegisterRecoveryAction(customVulkanAction);
        
        RecoveryAction customAudioAction;
        customAudioAction.id = "custom_audio_optimization";
        customAudioAction.name = "Audio Latency Optimization";
        customAudioAction.type = RecoveryActionType::REALLOCATE_RESOURCES;
        customAudioAction.priority = RecoveryPriority::LOW;
        customAudioAction.targetComponent = "AudioEngine";
        customAudioAction.description = "Optimizes audio buffer allocation to reduce latency";
        customAudioAction.timeout = std::chrono::milliseconds(8000);
        customAudioAction.maxRetries = 2;
        customAudioAction.execute = []() {
            std::cout << "Executing audio latency optimization..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return true;
        };
        registry.RegisterRecoveryAction(customAudioAction);
        
        std::cout << "Registered custom recovery actions." << std::endl;
    }
    
    void StartAutomatedIncidentDetection() {
        // Start a background thread for automated incident detection
        m_incidentDetectionThread = std::thread([this]() {
            IncidentDetectionLoop();
        });
        
        std::cout << "Started automated incident detection." << std::endl;
    }
    
    void StopAutomatedIncidentDetection() {
        m_stopDetection = true;
        if (m_incidentDetectionThread.joinable()) {
            m_incidentDetectionThread.join();
        }
        std::cout << "Stopped automated incident detection." << std::endl;
    }
    
    void IncidentDetectionLoop() {
        while (!m_stopDetection) {
            // Auto-detect incidents based on system health
            AutomatedRecoverySystem::GetInstance().AutoDetectAndRecover();
            
            // Wait before next detection cycle
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    
    void DemonstrateIncidentReporting() {
        std::cout << "\n--- Demonstrating Incident Reporting --" << std::endl;
        
        // Report various types of incidents
        std::cout << "Reporting Vulkan context incident..." << std::endl;
        AutomatedRecoverySystem::GetInstance().ReportIncident(
            "VulkanContext",
            "gpu_memory_exhaustion",
            RecoveryPriority::CRITICAL,
            {
                {"error_code", "VK_ERROR_OUT_OF_DEVICE_MEMORY"},
                {"memory_usage_mb", "8192"},
                {"available_memory_mb", "256"}
            }
        );
        
        std::cout << "Reporting audio engine incident..." << std::endl;
        AutomatedRecoverySystem::GetInstance().ReportIncident(
            "AudioEngine",
            "audio_buffer_underrun",
            RecoveryPriority::HIGH,
            {
                {"buffer_size", "1024"},
                {"underrun_count", "5"},
                {"latency_ms", "50"}
            }
        );
        
        std::cout << "Reporting AI engine incident..." << std::endl;
        AutomatedRecoverySystem::GetInstance().ReportIncident(
            "AIEngine",
            "model_inference_timeout",
            RecoveryPriority::MEDIUM,
            {
                {"model_name", "text_generation_model"},
                {"timeout_ms", "30000"},
                {"input_size", "1024"}
            }
        );
        
        // Show active incidents
        auto activeIncidents = AutomatedRecoverySystem::GetInstance().GetActiveIncidents();
        std::cout << "Active incidents: " << activeIncidents.size() << std::endl;
        
        for (const auto& incident : activeIncidents) {
            std::cout << "  - " << incident.component << ": " << incident.failureType 
                     << " (Priority: " << static_cast<int>(incident.priority) << ")" << std::endl;
        }
    }
    
    void DemonstrateRecoveryPlanExecution() {
        std::cout << "\n--- Demonstrating Recovery Plan Execution --" << std::endl;
        
        auto& orchestrator = RecoveryOrchestrator::GetInstance();
        
        // Create and execute recovery plans for different components
        std::cout << "Creating Vulkan recovery plan..." << std::endl;
        auto vulkanPlan = orchestrator.CreateRecoveryPlanForComponent(
            "VulkanContext",
            "gpu_memory_exhaustion",
            RecoveryPriority::CRITICAL
        );
        
        std::cout << "Vulkan recovery plan contains " << vulkanPlan.actions.size() << " actions." << std::endl;
        
        // Execute the recovery plan
        std::cout << "Executing Vulkan recovery plan..." << std::endl;
        bool vulkanSuccess = orchestrator.ExecuteRecoveryPlan(vulkanPlan);
        std::cout << "Vulkan recovery plan execution: " << (vulkanSuccess ? "SUCCESS" : "FAILED") << std::endl;
        
        // Create and execute audio recovery plan
        std::cout << "\nCreating Audio recovery plan..." << std::endl;
        auto audioPlan = orchestrator.CreateRecoveryPlanForComponent(
            "AudioEngine",
            "audio_buffer_underrun",
            RecoveryPriority::HIGH
        );
        
        std::cout << "Audio recovery plan contains " << audioPlan.actions.size() << " actions." << std::endl;
        
        std::cout << "Executing Audio recovery plan..." << std::endl;
        bool audioSuccess = orchestrator.ExecuteRecoveryPlan(audioPlan);
        std::cout << "Audio recovery plan execution: " << (audioSuccess ? "SUCCESS" : "FAILED") << std::endl;
    }
    
    void DemonstrateSelfHealingPolicies() {
        std::cout << "\n--- Demonstrating Self-Healing Policies --" << std::endl;
        
        auto& selfHealingManager = SelfHealingManager::GetInstance();
        auto activePolicies = selfHealingManager.GetActivePolicies();
        
        std::cout << "Active self-healing policies: " << activePolicies.size() << std::endl;
        for (const auto& policy : activePolicies) {
            std::cout << "  - " << policy.name << " (Component: " << policy.component 
                     << ", Cooldown: " << policy.cooldown.count() / 1000 << "s)" << std::endl;
        }
        
        // Manually trigger policy evaluation
        std::cout << "Evaluating healing policies..." << std::endl;
        selfHealingManager.EvaluateHealingPolicies();
        
        // Simulate component health changes to trigger policies
        std::cout << "Simulating component health issues to trigger self-healing..." << std::endl;
        SimulateComponentHealthIssues();
    }
    
    void DemonstrateAutomatedIncidentDetection() {
        std::cout << "\n--- Demonstrating Automated Incident Detection --" << std::endl;
        
        // The automated incident detection is already running in the background
        // Let's simulate some health check failures to trigger automatic detection
        
        std::cout << "Automated incident detection is running..." << std::endl;
        std::cout << "The system will automatically detect and recover from incidents." << std::endl;
        
        // Show current recovery statistics
        ShowRecoveryStatistics();
    }
    
    void RunManualIncidentSimulation() {
        std::cout << "\n--- Manual Incident Simulation --" << std::endl;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> componentDist(0, 3);
        std::uniform_int_distribution<> failureDist(0, 2);
        std::uniform_int_distribution<> priorityDist(1, 4);
        
        std::vector<std::string> components = {"VulkanContext", "AudioEngine", "AIEngine", "Memory"};
        std::vector<std::string> failures = {"performance_degradation", "resource_exhaustion", "timeout_error"};
        
        int incidentCount = 0;
        while (incidentCount < 10) {
            std::string component = components[componentDist(gen)];
            std::string failure = failures[failureDist(gen)];
            RecoveryPriority priority = static_cast<RecoveryPriority>(priorityDist(gen));
            
            std::cout << "\nSimulating incident: " << component << " - " << failure << std::endl;
            
            AutomatedRecoverySystem::GetInstance().ReportIncident(component, failure, priority, {
                {"simulated", "true"},
                {"incident_number", std::to_string(incidentCount + 1)}
            });
            
            // Show current status
            ShowRecoveryStatistics();
            
            // Wait before next incident
            std::this_thread::sleep_for(std::chrono::seconds(3));
            incidentCount++;
        }
        
        std::cout << "\nManual incident simulation completed." << std::endl;
        std::cout << "Final recovery statistics:" << std::endl;
        ShowRecoveryStatistics();
    }
    
    void SimulateComponentHealthIssues() {
        // Simulate health issues that would trigger self-healing policies
        
        // Simulate Vulkan context issues
        HealthMonitor::GetInstance().RegisterHealthCheck("VulkanContext_Simulated", []() {
            static int counter = 0;
            counter++;
            return (counter % 10 == 0) ? HealthStatus::DEGRADED : HealthStatus::HEALTHY;
        });
        
        // Simulate audio engine issues
        HealthMonitor::GetInstance().RegisterHealthCheck("AudioEngine_Simulated", []() {
            static int counter = 0;
            counter++;
            return (counter % 15 == 0) ? HealthStatus::UNHEALTHY : HealthStatus::HEALTHY;
        });
        
        // Trigger policy evaluation
        SelfHealingManager::GetInstance().EvaluateHealingPolicies();
    }
    
    void ShowRecoveryStatistics() {
        auto& recoverySystem = AutomatedRecoverySystem::GetInstance();
        
        std::cout << "\n--- Recovery System Statistics --" << std::endl;
        
        // Overall statistics
        std::cout << "Total recoveries: " << recoverySystem.GetTotalRecoveries() << std::endl;
        std::cout << "Recovery success rate: " << std::fixed << std::setprecision(1) 
                  << recoverySystem.GetRecoverySuccessRate() << "%" << std::endl;
        std::cout << "Average recovery time: " << std::fixed << std::setprecision(0) 
                  << recoverySystem.GetAverageRecoveryTime() << " ms" << std::endl;
        
        // Active incidents
        auto activeIncidents = recoverySystem.GetActiveIncidents();
        std::cout << "Active incidents: " << activeIncidents.size() << std::endl;
        for (const auto& incident : activeIncidents) {
            std::cout << "  - " << incident.component << ": " << incident.failureType 
                     << " (" << incident.resolutionStatus << ")" << std::endl;
        }
        
        // Active recovery plans
        auto& orchestrator = RecoveryOrchestrator::GetInstance();
        auto activePlans = orchestrator.GetActiveRecoveryPlans();
        std::cout << "Active recovery plans: " << activePlans.size() << std::endl;
        
        // Executing recovery actions
        auto& executor = RecoveryExecutor::GetInstance();
        auto executingActions = executor.GetExecutingActions();
        std::cout << "Executing recovery actions: " << executingActions.size() << std::endl;
        
        // Self-healing status
        auto& selfHealingManager = SelfHealingManager::GetInstance();
        std::cout << "Self-healing active: " << (selfHealingManager.IsSelfHealingActive() ? "YES" : "NO") << std::endl;
        
        // Auto-recovery status
        std::cout << "Auto-recovery enabled: " << (recoverySystem.IsAutoRecoveryEnabled() ? "YES" : "NO") << std::endl;
    }
    
    // Member variables
    std::thread m_incidentDetectionThread;
    std::atomic<bool> m_stopDetection{false};
};

// Main demonstration function
int main() {
    try {
        AutomatedRecoveryDemo recoveryDemo;
        recoveryDemo.RunDemonstration();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}