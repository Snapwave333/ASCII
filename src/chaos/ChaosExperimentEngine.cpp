#include "ChaosExperimentEngine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace NeonGlyph {
namespace Chaos {

ChaosExperimentEngine::ChaosExperimentEngine(std::shared_ptr<FailureModeAnalysis> failureAnalysis)
    : m_failureAnalysis(failureAnalysis),
      m_engineActive(true),
      m_automaticRecoveryEnabled(true),
      m_maxConcurrentFailures(3),
      m_currentFailureCount(0),
      m_randomGenerator(m_randomDevice()),
      m_engineStartTime(std::chrono::steady_clock::now()) {
    
    // Set default safety thresholds
    m_safetyThresholds["cpu_usage"] = 90.0;
    m_safetyThresholds["memory_usage"] = 85.0;
    m_safetyThresholds["disk_usage"] = 95.0;
    m_safetyThresholds["response_time"] = 5000.0; // 5 seconds
    m_safetyThresholds["error_rate"] = 10.0; // 10%
}

ChaosExperimentEngine::~ChaosExperimentEngine() {
    m_engineActive = false;
    
    // Wait for all experiment threads to complete
    for (auto& [id, thread] : m_experimentThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

std::string ChaosExperimentEngine::CreateExperiment(const ChaosExperiment& experiment) {
    std::string experimentId = "CHAOS_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    m_experiments[experimentId] = experiment;
    m_experimentStates[experimentId] = ExperimentState::IDLE;
    
    std::cout << "Created chaos experiment: " << experiment.name << " (ID: " << experimentId << ")" << std::endl;
    return experimentId;
}

void ChaosExperimentEngine::StartExperiment(const std::string& experimentId) {
    auto it = m_experiments.find(experimentId);
    if (it == m_experiments.end()) {
        std::cerr << "Experiment not found: " << experimentId << std::endl;
        return;
    }
    
    if (m_experimentStates[experimentId] == ExperimentState::RUNNING) {
        std::cout << "Experiment already running: " << experimentId << std::endl;
        return;
    }
    
    m_experimentStates[experimentId] = ExperimentState::RUNNING;
    
    // Start experiment in separate thread
    m_experimentThreads[experimentId] = std::thread([this, experimentId]() {
        ExecuteExperiment(experimentId);
    });
    
    std::cout << "Started chaos experiment: " << experimentId << std::endl;
}

void ChaosExperimentEngine::ExecuteExperiment(const std::string& experimentId) {
    auto& experiment = m_experiments[experimentId];
    ExperimentResult result;
    result.experimentId = experimentId;
    result.startTime = std::chrono::steady_clock::now();
    result.systemRecovered = true;
    result.maxDegradation = 0.0;
    
    // Collect baseline metrics
    result.systemMetricsBefore = CollectSystemMetrics();
    
    std::cout << "Executing chaos experiment: " << experiment.name << std::endl;
    std::cout << "Strategy: " << static_cast<int>(experiment.strategy) << std::endl;
    std::cout << "Duration: " << experiment.duration.count() << " seconds" << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + experiment.duration;
    
    try {
        // Execute based on injection strategy
        switch (experiment.strategy) {
            case InjectionStrategy::RANDOM:
                InjectRandomFailures(experiment);
                break;
            case InjectionStrategy::SEQUENTIAL:
                InjectSequentialFailures(experiment);
                break;
            case InjectionStrategy::TARGETED:
                InjectTargetedFailures(experiment);
                break;
            case InjectionStrategy::GRADUAL:
                InjectGradualFailures(experiment);
                break;
            case InjectionStrategy::BURST:
                InjectBurstFailures(experiment);
                break;
            case InjectionStrategy::DISTRIBUTED:
                InjectDistributedFailures(experiment);
                break;
        }
        
        // Monitor experiment progress
        while (std::chrono::steady_clock::now() < endTime && 
               m_experimentStates[experimentId] == ExperimentState::RUNNING) {
            
            MonitorExperimentProgress(experimentId);
            RecordExperimentMetrics(experimentId);
            
            // Check safety thresholds
            if (AreSafetyThresholdsExceeded()) {
                std::cout << "Safety thresholds exceeded, triggering recovery" << std::endl;
                TriggerAutomaticRecovery(experimentId);
                result.systemRecovered = false;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        result.finalState = ExperimentState::COMPLETED;
        
    } catch (const std::exception& e) {
        std::cerr << "Experiment failed with exception: " << e.what() << std::endl;
        result.finalState = ExperimentState::FAILED;
        result.systemRecovered = false;
    }
    
    // Collect final metrics
    result.systemMetricsAfter = CollectSystemMetrics();
    result.endTime = std::chrono::steady_clock::now();
    
    // Generate summary report
    std::stringstream summary;
    summary << "Chaos Experiment Report\n";
    summary << "======================\n";
    summary << "Experiment: " << experiment.name << "\n";
    summary << "Duration: " << std::chrono::duration_cast<std::chrono::seconds>(result.endTime - result.startTime).count() << "s\n";
    summary << "Final State: " << static_cast<int>(result.finalState) << "\n";
    summary << "System Recovered: " << (result.systemRecovered ? "YES" : "NO") << "\n";
    summary << "Max Degradation: " << result.maxDegradation << "%\n";
    result.summaryReport = summary.str();
    
    m_results[experimentId] = result;
    m_experimentStates[experimentId] = result.finalState;
    
    std::cout << "Experiment completed: " << experimentId << std::endl;
    std::cout << result.summaryReport << std::endl;
}

void ChaosExperimentEngine::InjectRandomFailures(const ChaosExperiment& experiment) {
    std::uniform_int_distribution<> scenarioDist(0, experiment.scenarios.size() - 1);
    std::uniform_int_distribution<> intervalDist(1, experiment.interval.count());
    
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + experiment.duration;
    
    while (std::chrono::steady_clock::now() < endTime) {
        // Select random scenario
        int scenarioIndex = scenarioDist(m_randomGenerator);
        const auto& scenario = experiment.scenarios[scenarioIndex];
        
        std::cout << "Injecting random failure: " << scenario.name << std::endl;
        InjectFailure(scenario);
        
        // Wait random interval
        int waitTime = intervalDist(m_randomGenerator);
        std::this_thread::sleep_for(std::chrono::seconds(waitTime));
    }
}

void ChaosExperimentEngine::InjectSequentialFailures(const ChaosExperiment& experiment) {
    for (size_t i = 0; i < experiment.scenarios.size(); ++i) {
        const auto& scenario = experiment.scenarios[i];
        std::cout << "Injecting sequential failure: " << scenario.name << std::endl;
        InjectFailure(scenario);
        
        std::this_thread::sleep_for(experiment.interval);
    }
}

void ChaosExperimentEngine::InjectTargetedFailures(const ChaosExperiment& experiment) {
    // Target specific components based on criticality
    for (const auto& scenario : experiment.scenarios) {
        if (m_failureAnalysis->AssessFailureImpact(scenario.targetComponent, scenario.type).severity >= FailureSeverity::HIGH) {
            std::cout << "Injecting targeted failure: " << scenario.name << std::endl;
            InjectFailure(scenario);
        }
    }
}

void ChaosExperimentEngine::InjectGradualFailures(const ChaosExperiment& experiment) {
    // Gradually increase failure intensity
    int steps = experiment.duration.count() / experiment.interval.count();
    double intensityIncrement = experiment.intensity / steps;
    double currentIntensity = 0.0;
    
    for (int i = 0; i < steps; ++i) {
        currentIntensity += intensityIncrement;
        
        // Inject failures with increasing intensity
        for (const auto& scenario : experiment.scenarios) {
            std::uniform_real_distribution<> dist(0.0, 1.0);
            if (dist(m_randomGenerator) < currentIntensity) {
                std::cout << "Injecting gradual failure (intensity: " << currentIntensity << "): " << scenario.name << std::endl;
                InjectFailure(scenario);
            }
        }
        
        std::this_thread::sleep_for(experiment.interval);
    }
}

void ChaosExperimentEngine::InjectBurstFailures(const ChaosExperiment& experiment) {
    // Inject multiple failures in quick succession
    std::cout << "Injecting burst failures..." << std::endl;
    
    for (int burst = 0; burst < 3; ++burst) { // 3 bursts
        for (const auto& scenario : experiment.scenarios) {
            InjectFailure(scenario);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rapid injection
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait between bursts
    }
}

void ChaosExperimentEngine::InjectDistributedFailures(const ChaosExperiment& experiment) {
    // Distribute failures across different components
    std::map<std::string, std::vector<FailureScenario>> componentScenarios;
    
    // Group scenarios by target component
    for (const auto& scenario : experiment.scenarios) {
        componentScenarios[scenario.targetComponent].push_back(scenario);
    }
    
    // Inject failures across components
    for (const auto& [component, scenarios] : componentScenarios) {
        std::cout << "Injecting distributed failures for component: " << component << std::endl;
        for (const auto& scenario : scenarios) {
            InjectFailure(scenario);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void ChaosExperimentEngine::InjectFailure(const FailureScenario& scenario) {
    std::cout << "Injecting failure: " << scenario.name << std::endl;
    
    try {
        // Execute the injection function
        if (scenario.injectionFunction) {
            scenario.injectionFunction();
        }
        
        m_currentFailureCount++;
        
        // Simulate different types of failures
        switch (scenario.type) {
            case FailureType::MEMORY_LEAK:
                InjectMemoryLeak();
                break;
            case FailureType::RESOURCE_EXHAUSTION:
                InjectResourceExhaustion();
                break;
            case FailureType::INFINITE_LOOP:
                // Simulate infinite loop (with timeout protection)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            case FailureType::DEADLOCK:
                InjectDeadlock();
                break;
            case FailureType::RACE_CONDITION:
                InjectRaceCondition();
                break;
            case FailureType::PERFORMANCE_DEGRADATION:
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate slowdown
                break;
            default:
                // Generic failure injection
                break;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to inject failure: " << e.what() << std::endl;
    }
}

void ChaosExperimentEngine::InjectMemoryLeak() {
    // Simulate memory leak by allocating memory without deallocation
    std::cout << "Injecting memory leak..." << std::endl;
    
    // Allocate some memory (will be cleaned up when engine stops)
    size_t leakSize = 1024 * 1024; // 1MB
    void* leakedMemory = std::malloc(leakSize);
    if (leakedMemory) {
        std::memset(leakedMemory, 0, leakSize);
        // Intentionally not freeing to simulate leak
        std::cout << "Memory leak injected: " << leakSize << " bytes" << std::endl;
    }
}

void ChaosExperimentEngine::InjectResourceExhaustion() {
    std::cout << "Injecting resource exhaustion..." << std::endl;
    
    // Simulate CPU-intensive operation
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100)) {
        // Busy loop to consume CPU
        volatile double x = 0.0;
        for (int i = 0; i < 1000000; ++i) {
            x += std::sqrt(i);
        }
    }
}

void ChaosExperimentEngine::InjectDeadlock() {
    std::cout << "Injecting deadlock..." << std::endl;
    
    // Simulate deadlock condition
    std::mutex mutex1, mutex2;
    
    std::thread thread1([&]() {
        std::lock_guard<std::mutex> lock1(mutex1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::lock_guard<std::mutex> lock2(mutex2);
    });
    
    std::thread thread2([&]() {
        std::lock_guard<std::mutex> lock2(mutex2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::lock_guard<std::mutex> lock1(mutex1);
    });
    
    // Let threads attempt to deadlock, then force termination
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (thread1.joinable()) thread1.detach();
    if (thread2.joinable()) thread2.detach();
}

void ChaosExperimentEngine::InjectRaceCondition() {
    std::cout << "Injecting race condition..." << std::endl;
    
    // Simulate race condition
    std::atomic<int> sharedValue(0);
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&sharedValue]() {
            int temp = sharedValue.load();
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            sharedValue.store(temp + 1);
        });
    }
    
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "Race condition result: " << sharedValue.load() << " (expected: 10)" << std::endl;
}

std::map<std::string, double> ChaosExperimentEngine::CollectSystemMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    std::map<std::string, double> metrics;
    
    // Simulate system metrics collection
    metrics["cpu_usage"] = 25.0 + (std::rand() % 50); // 25-75%
    metrics["memory_usage"] = 30.0 + (std::rand() % 40); // 30-70%
    metrics["disk_usage"] = 40.0 + (std::rand() % 30); // 40-70%
    metrics["response_time"] = 100.0 + (std::rand() % 900); // 100-1000ms
    metrics["error_rate"] = 0.1 + (std::rand() % 5); // 0.1-5%
    metrics["failure_count"] = static_cast<double>(m_currentFailureCount.load());
    
    return metrics;
}

bool ChaosExperimentEngine::AreSafetyThresholdsExceeded() const {
    auto metrics = CollectSystemMetrics();
    
    for (const auto& [threshold, limit] : m_safetyThresholds) {
        auto it = metrics.find(threshold);
        if (it != metrics.end() && it->second > limit) {
            std::cout << "Safety threshold exceeded: " << threshold << " = " << it->second << " (limit: " << limit << ")" << std::endl;
            return true;
        }
    }
    
    return false;
}

void ChaosExperimentEngine::TriggerAutomaticRecovery(const std::string& experimentId) {
    std::cout << "Triggering automatic recovery for experiment: " << experimentId << std::endl;
    
    // Reset failure count
    m_currentFailureCount = 0;
    
    // Perform system recovery
    if (m_automaticRecoveryEnabled) {
        // In a real implementation, this would trigger actual recovery mechanisms
        std::cout << "Automatic recovery activated" << std::endl;
    }
}

std::string ChaosExperimentEngine::CreateNetworkChaosExperiment() {
    ChaosExperiment experiment;
    experiment.name = "Network Chaos Experiment";
    experiment.description = "Simulates network failures and partitions";
    experiment.strategy = InjectionStrategy::RANDOM;
    experiment.duration = std::chrono::seconds(60);
    experiment.interval = std::chrono::seconds(5);
    experiment.intensity = 0.3;
    experiment.allowRecovery = true;
    experiment.monitorSystemHealth = true;
    
    // Create network failure scenarios
    FailureScenario networkPartition;
    networkPartition.id = "NETWORK_PARTITION";
    networkPartition.name = "Network Partition";
    networkPartition.type = FailureType::NETWORK_PARTITION;
    networkPartition.severity = FailureSeverity::HIGH;
    networkPartition.targetComponent = "AudioEngine";
    networkPartition.description = "Simulates network connectivity loss";
    networkPartition.duration = std::chrono::seconds(10);
    networkPartition.requiresManualIntervention = false;
    
    experiment.scenarios.push_back(networkPartition);
    
    return CreateExperiment(experiment);
}

std::string ChaosExperimentEngine::CreateMemoryPressureExperiment() {
    ChaosExperiment experiment;
    experiment.name = "Memory Pressure Experiment";
    experiment.description = "Tests system behavior under memory pressure";
    experiment.strategy = InjectionStrategy::GRADUAL;
    experiment.duration = std::chrono::seconds(120);
    experiment.interval = std::chrono::seconds(10);
    experiment.intensity = 0.5;
    experiment.allowRecovery = true;
    experiment.monitorSystemHealth = true;
    
    FailureScenario memoryLeak;
    memoryLeak.id = "MEMORY_LEAK";
    memoryLeak.name = "Memory Leak";
    memoryLeak.type = FailureType::MEMORY_LEAK;
    memoryLeak.severity = FailureSeverity::HIGH;
    memoryLeak.targetComponent = "VulkanContext";
    memoryLeak.description = "Simulates gradual memory leak";
    memoryLeak.duration = std::chrono::seconds(30);
    memoryLeak.requiresManualIntervention = false;
    
    experiment.scenarios.push_back(memoryLeak);
    
    return CreateExperiment(experiment);
}

std::string ChaosExperimentEngine::CreateComprehensiveSystemStressExperiment() {
    ChaosExperiment experiment;
    experiment.name = "Comprehensive System Stress";
    experiment.description = "Comprehensive stress test of all system components";
    experiment.strategy = InjectionStrategy::DISTRIBUTED;
    experiment.duration = std::chrono::seconds(300);
    experiment.interval = std::chrono::seconds(15);
    experiment.intensity = 0.7;
    experiment.allowRecovery = true;
    experiment.monitorSystemHealth = true;
    
    // Add multiple failure scenarios
    std::vector<FailureType> failureTypes = {
        FailureType::MEMORY_LEAK,
        FailureType::RESOURCE_EXHAUSTION,
        FailureType::PERFORMANCE_DEGRADATION,
        FailureType::INFINITE_LOOP,
        FailureType::RACE_CONDITION
    };
    
    for (size_t i = 0; i < failureTypes.size(); ++i) {
        FailureScenario scenario;
        scenario.id = "STRESS_SCENARIO_" + std::to_string(i);
        scenario.name = "Stress Scenario " + std::to_string(i);
        scenario.type = failureTypes[i];
        scenario.severity = FailureSeverity::MEDIUM;
        scenario.targetComponent = "SystemWide";
        scenario.description = "Comprehensive stress test scenario";
        scenario.duration = std::chrono::seconds(20);
        scenario.requiresManualIntervention = false;
        
        experiment.scenarios.push_back(scenario);
    }
    
    return CreateExperiment(experiment);
}

std::string ChaosExperimentEngine::GenerateChaosReport() const {
    std::stringstream report;
    
    report << "NEONGLYPH CHAOS ENGINEERING REPORT\n";
    report << "===================================\n\n";
    
    report << "Total Experiments Created: " << m_experiments.size() << "\n";
    report << "Completed Experiments: " << m_results.size() << "\n";
    report << "Running Experiments: " << GetRunningExperiments().size() << "\n\n";
    
    report << "SYSTEM STABILITY METRICS:\n";
    auto metrics = CollectSystemMetrics();
    for (const auto& [metric, value] : metrics) {
        report << "- " << metric << ": " << value << "\n";
    }
    
    report << "\nSAFETY THRESHOLDS:\n";
    for (const auto& [threshold, limit] : m_safetyThresholds) {
        report << "- " << threshold << ": " << limit << "\n";
    }
    
    return report.str();
}

std::vector<std::string> ChaosExperimentEngine::GetRunningExperiments() const {
    std::vector<std::string> running;
    for (const auto& [id, state] : m_experimentStates) {
        if (state == ExperimentState::RUNNING) {
            running.push_back(id);
        }
    }
    return running;
}

std::vector<std::string> ChaosExperimentEngine::GetCompletedExperiments() const {
    std::vector<std::string> completed;
    for (const auto& [id, state] : m_experimentStates) {
        if (state == ExperimentState::COMPLETED || state == ExperimentState::FAILED) {
            completed.push_back(id);
        }
    }
    return completed;
}

} // namespace Chaos
} // namespace NeonGlyph