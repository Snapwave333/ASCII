#include "ResilienceMonitoringIntegration.h"
#include "FailureModeAnalysis.h"
#include "ChaosExperimentEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

using namespace NeonGlyph::Chaos;

class NeonGlyphChaosEngine {
public:
    NeonGlyphChaosEngine() {
        InitializeChaosEngineering();
    }
    
    ~NeonGlyphChaosEngine() {
        ShutdownChaosEngineering();
    }
    
    void RunDemonstration() {
        std::cout << "=== NeonGlyph Chaos Engineering Demonstration ===" << std::endl;
        
        // Start the monitoring dashboard
        std::cout << "Starting resilience monitoring dashboard on port 8080..." << std::endl;
        ResilienceMonitoringIntegration::GetInstance().StartDashboardWebServer(8080);
        
        std::cout << "Dashboard available at: http://localhost:8080" << std::endl;
        std::cout << "JSON API available at: http://localhost:8080/json" << std::endl;
        
        // Demonstrate various chaos engineering scenarios
        DemonstrateFailureModeAnalysis();
        DemonstrateCircuitBreakerPatterns();
        DemonstrateRedundancySystems();
        DemonstrateChaosExperiments();
        
        // Keep the system running for monitoring
        std::cout << "\nSystem running. Press Enter to continue with chaos experiments..." << std::endl;
        std::cin.get();
        
        RunContinuousChaos();
    }
    
private:
    void InitializeChaosEngineering() {
        // Initialize the resilience monitoring integration
        ResilienceMonitoringIntegration::GetInstance().Initialize();
        
        // Create monitored circuit breakers for key systems
        CreateMonitoredCircuitBreakers();
        
        // Create monitored redundancy systems
        CreateMonitoredRedundancySystems();
        
        // Register health checks
        RegisterHealthChecks();
        
        // Initialize failure mode analysis
        m_failureModeAnalysis = std::make_unique<FailureModeAnalysis>();
        m_failureModeAnalysis->AnalyzeSystem();
        
        // Initialize chaos experiment engine
        m_chaosEngine = std::make_unique<ChaosExperimentEngine>();
        
        std::cout << "Chaos engineering system initialized successfully." << std::endl;
    }
    
    void ShutdownChaosEngineering() {
        ResilienceMonitoringIntegration::GetInstance().Shutdown();
        std::cout << "Chaos engineering system shutdown complete." << std::endl;
    }
    
    void CreateMonitoredCircuitBreakers() {
        // Vulkan Context Circuit Breaker
        m_vulkanCircuitBreaker = std::make_unique<MonitoredCircuitBreaker>(
            "VulkanContext", 3, std::chrono::seconds(30), std::chrono::seconds(15)
        );
        
        // Audio Engine Circuit Breaker
        m_audioCircuitBreaker = std::make_unique<MonitoredCircuitBreaker>(
            "AudioEngine", 5, std::chrono::seconds(60), std::chrono::seconds(30)
        );
        
        // AI Engine Circuit Breaker
        m_aiCircuitBreaker = std::make_unique<MonitoredCircuitBreaker>(
            "AIEngine", 4, std::chrono::seconds(45), std::chrono::seconds(20)
        );
        
        // Network Circuit Breaker
        m_networkCircuitBreaker = std::make_unique<MonitoredCircuitBreaker>(
            "Network", 6, std::chrono::seconds(90), std::chrono::seconds(45)
        );
        
        std::cout << "Created monitored circuit breakers for all critical systems." << std::endl;
    }
    
    void CreateMonitoredRedundancySystems() {
        // Vulkan Graphics Redundancy
        m_vulkanRedundancy = std::make_unique<MonitoredRedundancyManager>("VulkanGraphics");
        m_vulkanRedundancy->GetManager()->ConfigureVulkanRedundancy();
        
        // Audio Engine Redundancy
        m_audioRedundancy = std::make_unique<MonitoredRedundancyManager>("AudioEngine");
        m_audioRedundancy->GetManager()->ConfigureAudioRedundancy();
        
        // AI Engine Redundancy
        m_aiRedundancy = std::make_unique<MonitoredRedundancyManager>("AIEngine");
        m_aiRedundancy->GetManager()->ConfigureAIRedundancy();
        
        // Headless Fallback Redundancy
        m_headlessRedundancy = std::make_unique<MonitoredRedundancyManager>("HeadlessFallback");
        m_headlessRedundancy->GetManager()->ConfigureHeadlessRedundancy();
        
        std::cout << "Created monitored redundancy systems for all critical subsystems." << std::endl;
    }
    
    void RegisterHealthChecks() {
        // Vulkan Context Health Check
        HealthMonitor::GetInstance().RegisterHealthCheck("VulkanContext", [this]() {
            return SimulateVulkanHealthCheck();
        });
        
        // Audio Engine Health Check
        HealthMonitor::GetInstance().RegisterHealthCheck("AudioEngine", [this]() {
            return SimulateAudioHealthCheck();
        });
        
        // AI Engine Health Check
        HealthMonitor::GetInstance().RegisterHealthCheck("AIEngine", [this]() {
            return SimulateAIHealthCheck();
        });
        
        // Memory Health Check
        HealthMonitor::GetInstance().RegisterHealthCheck("Memory", [this]() {
            return SimulateMemoryHealthCheck();
        });
        
        // Network Health Check
        HealthMonitor::GetInstance().RegisterHealthCheck("Network", [this]() {
            return SimulateNetworkHealthCheck();
        });
        
        std::cout << "Registered health checks for all system components." << std::endl;
    }
    
    void DemonstrateFailureModeAnalysis() {
        std::cout << "\n--- Demonstrating Failure Mode Analysis --" << std::endl;
        
        // Get failure scenarios for different components
        auto vulkanScenarios = m_failureModeAnalysis->GetFailureScenarios("VulkanContext");
        auto audioScenarios = m_failureModeAnalysis->GetFailureScenarios("AudioEngine");
        auto aiScenarios = m_failureModeAnalysis->GetFailureScenarios("AIEngine");
        
        std::cout << "Vulkan Context Failure Scenarios: " << vulkanScenarios.size() << std::endl;
        std::cout << "Audio Engine Failure Scenarios: " << audioScenarios.size() << std::endl;
        std::cout << "AI Engine Failure Scenarios: " << aiScenarios.size() << std::endl;
        
        // Simulate a failure scenario
        if (!vulkanScenarios.empty()) {
            auto& scenario = vulkanScenarios[0];
            std::cout << "Simulating failure scenario: " << scenario.description << std::endl;
            
            auto impact = m_failureModeAnalysis->AssessFailureImpact("VulkanContext", scenario.failureType);
            std::cout << "Failure Impact - Severity: " << static_cast<int>(impact.severity) 
                     << ", Affected Systems: " << impact.affectedSystems.size() << std::endl;
        }
    }
    
    void DemonstrateCircuitBreakerPatterns() {
        std::cout << "\n--- Demonstrating Circuit Breaker Patterns --" << std::endl;
        
        // Simulate successful operations
        std::cout << "Simulating successful operations..." << std::endl;
        for (int i = 0; i < 10; ++i) {
            try {
                m_vulkanCircuitBreaker->Execute([this]() {
                    SimulateVulkanOperation(true);
                });
                std::cout << "Vulkan operation " << i + 1 << " succeeded." << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Vulkan operation " << i + 1 << " failed: " << e.what() << std::endl;
            }
        }
        
        // Simulate failures to trigger circuit breaker
        std::cout << "\nSimulating failures to trigger circuit breaker..." << std::endl;
        for (int i = 0; i < 5; ++i) {
            try {
                m_vulkanCircuitBreaker->Execute([this]() {
                    SimulateVulkanOperation(false);
                });
            } catch (const std::exception& e) {
                std::cout << "Expected failure " << i + 1 << ": " << e.what() << std::endl;
            }
        }
        
        // Check circuit breaker state
        auto state = m_vulkanCircuitBreaker->GetState();
        std::cout << "Circuit breaker state: " << (state == CircuitBreakerState::OPEN ? "OPEN" : 
                                                  state == CircuitBreakerState::HALF_OPEN ? "HALF_OPEN" : "CLOSED") << std::endl;
    }
    
    void DemonstrateRedundancySystems() {
        std::cout << "\n--- Demonstrating Redundancy Systems --" << std::endl;
        
        // Simulate instance failures
        std::cout << "Simulating instance failures..." << std::endl;
        
        // Get current redundancy metrics
        auto vulkanMetrics = m_vulkanRedundancy->GetManager()->GetRedundancyMetrics("VulkanGraphics");
        std::cout << "Vulkan Graphics - Total: " << vulkanMetrics.totalInstances 
                 << ", Healthy: " << vulkanMetrics.healthyInstances 
                 << ", Failed: " << vulkanMetrics.failedInstances << std::endl;
        
        // Simulate failover
        std::cout << "Triggering failover for VulkanGraphics..." << std::endl;
        bool failoverResult = m_vulkanRedundancy->GetManager()->TriggerFailover("VulkanGraphics", "instance-1");
        std::cout << "Failover result: " << (failoverResult ? "SUCCESS" : "FAILED") << std::endl;
        
        // Check redundancy health after failover
        auto updatedMetrics = m_vulkanRedundancy->GetManager()->GetRedundancyMetrics("VulkanGraphics");
        std::cout << "After failover - Total: " << updatedMetrics.totalInstances 
                 << ", Healthy: " << updatedMetrics.healthyInstances 
                 << ", Failed: " << updatedMetrics.failedInstances << std::endl;
    }
    
    void DemonstrateChaosExperiments() {
        std::cout << "\n--- Demonstrating Chaos Experiments --" << std::endl;
        
        // Create a network chaos experiment
        std::cout << "Creating network chaos experiment..." << std::endl;
        std::string experimentId = m_chaosEngine->CreateNetworkChaosExperiment();
        std::cout << "Network chaos experiment created: " << experimentId << std::endl;
        
        // Create a memory pressure experiment
        std::cout << "Creating memory pressure experiment..." << std::endl;
        std::string memoryExperimentId = m_chaosEngine->CreateMemoryPressureExperiment();
        std::cout << "Memory pressure experiment created: " << memoryExperimentId << std::endl;
        
        // Run the experiments
        std::cout << "Running chaos experiments..." << std::endl;
        m_chaosEngine->StartExperiment(experimentId);
        m_chaosEngine->StartExperiment(memoryExperimentId);
        
        // Wait for experiments to complete
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Check experiment results
        auto networkResult = m_chaosEngine->GetExperimentResult(experimentId);
        auto memoryResult = m_chaosEngine->GetExperimentResult(memoryExperimentId);
        
        std::cout << "Network experiment result: " << networkResult.status << std::endl;
        std::cout << "Memory experiment result: " << memoryResult.status << std::endl;
    }
    
    void RunContinuousChaos() {
        std::cout << "\n--- Running Continuous Chaos Experiments --" << std::endl;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> experimentDist(0, 3);
        std::uniform_int_distribution<> intervalDist(5, 15);
        
        int experimentCount = 0;
        
        while (experimentCount < 20) { // Run 20 experiments
            int experimentType = experimentDist(gen);
            std::string experimentId;
            
            switch (experimentType) {
                case 0:
                    experimentId = m_chaosEngine->CreateNetworkChaosExperiment();
                    std::cout << "Created network chaos experiment: " << experimentId << std::endl;
                    break;
                case 1:
                    experimentId = m_chaosEngine->CreateMemoryPressureExperiment();
                    std::cout << "Created memory pressure experiment: " << experimentId << std::endl;
                    break;
                case 2:
                    experimentId = m_chaosEngine->CreateResourceExhaustionExperiment();
                    std::cout << "Created resource exhaustion experiment: " << experimentId << std::endl;
                    break;
                case 3:
                    experimentId = m_chaosEngine->CreateDeadlockExperiment();
                    std::cout << "Created deadlock experiment: " << experimentId << std::endl;
                    break;
            }
            
            // Start the experiment
            m_chaosEngine->StartExperiment(experimentId);
            
            // Update metrics
            ResilienceMonitoringIntegration::GetInstance().UpdateAllMetrics();
            
            // Wait before next experiment
            int interval = intervalDist(gen);
            std::cout << "Waiting " << interval << " seconds before next experiment..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            
            experimentCount++;
            
            // Show current system status
            ShowSystemStatus();
        }
        
        std::cout << "\nContinuous chaos experiments completed." << std::endl;
        std::cout << "Dashboard will remain available at http://localhost:8080" << std::endl;
        std::cout << "Press Enter to shutdown..." << std::endl;
        std::cin.get();
    }
    
    void ShowSystemStatus() {
        std::cout << "\n--- Current System Status --" << std::endl;
        
        // Overall health
        auto overallHealth = HealthMonitor::GetInstance().GetOverallHealth();
        double healthScore = HealthMonitor::GetInstance().GetHealthScore();
        std::cout << "Overall Health: ";
        switch (overallHealth) {
            case HealthStatus::HEALTHY:
                std::cout << "HEALTHY";
                break;
            case HealthStatus::DEGRADED:
                std::cout << "DEGRADED";
                break;
            case HealthStatus::UNHEALTHY:
                std::cout << "UNHEALTHY";
                break;
            case HealthStatus::CRITICAL:
                std::cout << "CRITICAL";
                break;
        }
        std::cout << " (Score: " << healthScore << "%)" << std::endl;
        
        // Circuit breaker health
        double cbHealth = CircuitBreakerMonitor::GetInstance().GetOverallCircuitBreakerHealth();
        std::cout << "Circuit Breaker Health: " << cbHealth << "%" << std::endl;
        
        // Open circuit breakers
        auto openBreakers = CircuitBreakerMonitor::GetInstance().GetOpenCircuitBreakers();
        if (!openBreakers.empty()) {
            std::cout << "Open Circuit Breakers: ";
            for (const auto& breaker : openBreakers) {
                std::cout << breaker << " ";
            }
            std::cout << std::endl;
        }
        
        // Redundancy systems with active failover
        auto activeFailovers = RedundancyMonitor::GetInstance().GetSystemsWithActiveFailover();
        if (!activeFailovers.empty()) {
            std::cout << "Active Failovers: ";
            for (const auto& system : activeFailovers) {
                std::cout << system << " ";
            }
            std::cout << std::endl;
        }
        
        // Active chaos experiments
        auto activeExperiments = ChaosExperimentMonitor::GetInstance().GetActiveExperiments();
        if (!activeExperiments.empty()) {
            std::cout << "Active Chaos Experiments: " << activeExperiments.size() << std::endl;
        }
        
        // Experiment success rate
        double successRate = ChaosExperimentMonitor::GetInstance().GetExperimentSuccessRate();
        std::cout << "Experiment Success Rate: " << successRate << "%" << std::endl;
    }
    
    // Simulation methods
    HealthStatus SimulateVulkanHealthCheck() {
        static int counter = 0;
        counter++;
        
        // Simulate occasional issues
        if (counter % 20 == 0) return HealthStatus::DEGRADED;
        if (counter % 50 == 0) return HealthStatus::UNHEALTHY;
        if (counter % 100 == 0) return HealthStatus::CRITICAL;
        
        return HealthStatus::HEALTHY;
    }
    
    HealthStatus SimulateAudioHealthCheck() {
        static int counter = 0;
        counter++;
        
        // Audio is generally stable
        if (counter % 30 == 0) return HealthStatus::DEGRADED;
        if (counter % 80 == 0) return HealthStatus::UNHEALTHY;
        
        return HealthStatus::HEALTHY;
    }
    
    HealthStatus SimulateAIHealthCheck() {
        static int counter = 0;
        counter++;
        
        // AI can be resource intensive
        if (counter % 15 == 0) return HealthStatus::DEGRADED;
        if (counter % 40 == 0) return HealthStatus::UNHEALTHY;
        if (counter % 90 == 0) return HealthStatus::CRITICAL;
        
        return HealthStatus::HEALTHY;
    }
    
    HealthStatus SimulateMemoryHealthCheck() {
        // Check memory usage
        auto memoryUsage = GetSimulatedMemoryUsage();
        
        if (memoryUsage > 95) return HealthStatus::CRITICAL;
        if (memoryUsage > 85) return HealthStatus::UNHEALTHY;
        if (memoryUsage > 75) return HealthStatus::DEGRADED;
        
        return HealthStatus::HEALTHY;
    }
    
    HealthStatus SimulateNetworkHealthCheck() {
        static int counter = 0;
        counter++;
        
        // Network can have intermittent issues
        if (counter % 25 == 0) return HealthStatus::DEGRADED;
        if (counter % 60 == 0) return HealthStatus::UNHEALTHY;
        
        return HealthStatus::HEALTHY;
    }
    
    void SimulateVulkanOperation(bool shouldSucceed) {
        if (!shouldSucceed) {
            throw std::runtime_error("Vulkan operation failed: GPU memory allocation error");
        }
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    double GetSimulatedMemoryUsage() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::normal_distribution<> dist(60.0, 15.0);
        
        double usage = dist(gen);
        return std::max(0.0, std::min(100.0, usage));
    }
    
    // Member variables
    std::unique_ptr<FailureModeAnalysis> m_failureModeAnalysis;
    std::unique_ptr<ChaosExperimentEngine> m_chaosEngine;
    std::unique_ptr<MonitoredCircuitBreaker> m_vulkanCircuitBreaker;
    std::unique_ptr<MonitoredCircuitBreaker> m_audioCircuitBreaker;
    std::unique_ptr<MonitoredCircuitBreaker> m_aiCircuitBreaker;
    std::unique_ptr<MonitoredCircuitBreaker> m_networkCircuitBreaker;
    std::unique_ptr<MonitoredRedundancyManager> m_vulkanRedundancy;
    std::unique_ptr<MonitoredRedundancyManager> m_audioRedundancy;
    std::unique_ptr<MonitoredRedundancyManager> m_aiRedundancy;
    std::unique_ptr<MonitoredRedundancyManager> m_headlessRedundancy;
};

// Main demonstration function
int main() {
    try {
        NeonGlyphChaosEngine chaosEngine;
        chaosEngine.RunDemonstration();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}