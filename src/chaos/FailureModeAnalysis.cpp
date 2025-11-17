#include "FailureModeAnalysis.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

namespace NeonGlyph {
namespace Chaos {

FailureModeAnalysis::FailureModeAnalysis() 
    : m_monitoringActive(false) {
    MapSystemComponents();
}

FailureModeAnalysis::~FailureModeAnalysis() {
    StopHealthMonitoring();
}

void FailureModeAnalysis::MapSystemComponents() {
    std::lock_guard<std::mutex> lock(m_componentMutex);
    
    // Map all NeonGlyph-specific components
    MapVulkanComponents();
    MapAudioComponents();
    MapAIComponents();
    MapASCIIComponents();
    MapWindowComponents();
    MapConfigurationComponents();
    
    BuildFailurePropagationGraph();
}

void FailureModeAnalysis::MapVulkanComponents() {
    SystemComponent vulkanContext;
    vulkanContext.name = "VulkanContext";
    vulkanContext.type = "Graphics";
    vulkanContext.dependencies = {"GraphicsDriver", "VulkanSDK"};
    vulkanContext.dependents = {"Renderer", "ComputePipelines", "ASCIIConverter"};
    vulkanContext.isCritical = true;
    vulkanContext.hasRedundancy = false;
    vulkanContext.maxAcceptableDowntime = std::chrono::milliseconds(100);
    m_components["VulkanContext"] = vulkanContext;

    SystemComponent renderer;
    renderer.name = "Renderer";
    renderer.type = "Graphics";
    renderer.dependencies = {"VulkanContext", "Window"};
    renderer.dependents = {"ASCIIConverter", "OutputManager"};
    renderer.isCritical = true;
    renderer.hasRedundancy = false;
    renderer.maxAcceptableDowntime = std::chrono::milliseconds(50);
    m_components["Renderer"] = renderer;

    SystemComponent computePipelines;
    computePipelines.name = "ComputePipelines";
    computePipelines.type = "Compute";
    computePipelines.dependencies = {"VulkanContext", "ASCIIConverter"};
    computePipelines.dependents = {"ASCIIConverter"};
    computePipelines.isCritical = true;
    computePipelines.hasRedundancy = false;
    computePipelines.maxAcceptableDowntime = std::chrono::milliseconds(100);
    m_components["ComputePipelines"] = computePipelines;
}

void FailureModeAnalysis::MapAudioComponents() {
    SystemComponent audioEngine;
    audioEngine.name = "AudioEngine";
    audioEngine.type = "Audio";
    audioEngine.dependencies = {"WindowsAudioSystem", "WASAPI"};
    audioEngine.dependents = {"MusicAnalyzer", "AIConductor"};
    audioEngine.isCritical = false;
    audioEngine.hasRedundancy = false;
    audioEngine.maxAcceptableDowntime = std::chrono::milliseconds(500);
    m_components["AudioEngine"] = audioEngine;

    SystemComponent musicAnalyzer;
    musicAnalyzer.name = "MusicAnalyzer";
    musicAnalyzer.type = "Audio";
    musicAnalyzer.dependencies = {"AudioEngine"};
    musicAnalyzer.dependents = {"AIConductor", "AIDirector"};
    musicAnalyzer.isCritical = false;
    musicAnalyzer.hasRedundancy = false;
    musicAnalyzer.maxAcceptableDowntime = std::chrono::milliseconds(1000);
    m_components["MusicAnalyzer"] = musicAnalyzer;
}

void FailureModeAnalysis::MapAIComponents() {
    SystemComponent aiConductor;
    aiConductor.name = "AIConductor";
    aiConductor.type = "AI";
    aiConductor.dependencies = {"AudioEngine", "MusicAnalyzer", "ONNXRuntime"};
    aiConductor.dependents = {"AIDirector", "StoryContext"};
    aiConductor.isCritical = false;
    aiConductor.hasRedundancy = false;
    aiConductor.maxAcceptableDowntime = std::chrono::milliseconds(2000);
    m_components["AIConductor"] = aiConductor;

    SystemComponent aiDirector;
    aiDirector.name = "AIDirector";
    aiDirector.type = "AI";
    aiDirector.dependencies = {"AIConductor", "MusicAnalyzer"};
    aiDirector.dependents = {"ASCIIConverter", "ScenarioManager"};
    aiDirector.isCritical = false;
    aiDirector.hasRedundancy = false;
    aiDirector.maxAcceptableDowntime = std::chrono::milliseconds(3000);
    m_components["AIDirector"] = aiDirector;
}

void FailureModeAnalysis::MapASCIIComponents() {
    SystemComponent asciiConverter;
    asciiConverter.name = "ASCIIConverter";
    asciiConverter.type = "Processing";
    asciiConverter.dependencies = {"Renderer", "ComputePipelines", "AIDirector"};
    asciiConverter.dependents = {"OutputManager"};
    asciiConverter.isCritical = true;
    asciiConverter.hasRedundancy = false;
    asciiConverter.maxAcceptableDowntime = std::chrono::milliseconds(100);
    m_components["ASCIIConverter"] = asciiConverter;

    SystemComponent outputManager;
    outputManager.name = "OutputManager";
    outputManager.type = "Output";
    outputManager.dependencies = {"ASCIIConverter", "Renderer"};
    outputManager.dependents = {};
    outputManager.isCritical = true;
    outputManager.hasRedundancy = false;
    outputManager.maxAcceptableDowntime = std::chrono::milliseconds(100);
    m_components["OutputManager"] = outputManager;
}

void FailureModeAnalysis::MapWindowComponents() {
    SystemComponent window;
    window.name = "Window";
    window.type = "UI";
    window.dependencies = {"GLFW", "GraphicsDriver"};
    window.dependents = {"Renderer", "InputHandler"};
    window.isCritical = false;
    window.hasRedundancy = true; // Headless mode provides redundancy
    window.maxAcceptableDowntime = std::chrono::milliseconds(500);
    m_components["Window"] = window;

    SystemComponent glfw;
    glfw.name = "GLFW";
    glfw.type = "Windowing";
    glfw.dependencies = {"GraphicsDriver"};
    glfw.dependents = {"Window"};
    glfw.isCritical = false;
    glfw.hasRedundancy = false;
    glfw.maxAcceptableDowntime = std::chrono::milliseconds(1000);
    m_components["GLFW"] = glfw;
}

void FailureModeAnalysis::MapConfigurationComponents() {
    SystemComponent configManager;
    configManager.name = "ConfigManager";
    configManager.type = "Configuration";
    configManager.dependencies = {"FileSystem"};
    configManager.dependents = {"AllComponents"};
    configManager.isCritical = true;
    configManager.hasRedundancy = false;
    configManager.maxAcceptableDowntime = std::chrono::milliseconds(0); // Must never fail
    m_components["ConfigManager"] = configManager;

    SystemComponent safetyManager;
    safetyManager.name = "SafetyManager";
    safetyManager.type = "Safety";
    safetyManager.dependencies = {"AllComponents"};
    safetyManager.dependents = {};
    safetyManager.isCritical = true;
    safetyManager.hasRedundancy = true;
    safetyManager.maxAcceptableDowntime = std::chrono::milliseconds(0); // Must never fail
    m_components["SafetyManager"] = safetyManager;
}

void FailureModeAnalysis::BuildFailurePropagationGraph() {
    for (const auto& [name, component] : m_components) {
        std::vector<std::string> affectedComponents;
        
        // Direct dependents
        for (const auto& dependent : component.dependents) {
            affectedComponents.push_back(dependent);
        }
        
        // Cascading impact (2nd level)
        for (const auto& dependent : component.dependents) {
            if (m_components.find(dependent) != m_components.end()) {
                for (const auto& secondLevel : m_components.at(dependent).dependents) {
                    if (std::find(affectedComponents.begin(), affectedComponents.end(), secondLevel) == affectedComponents.end()) {
                        affectedComponents.push_back(secondLevel);
                    }
                }
            }
        }
        
        m_failurePropagationGraph[name] = affectedComponents;
    }
}

void FailureModeAnalysis::IdentifyFailurePoints() {
    std::lock_guard<std::mutex> lock(m_scenarioMutex);
    
    // Register critical failure scenarios for each component
    for (const auto& [name, component] : m_components) {
        if (component.isCritical && !component.hasRedundancy) {
            // Single point of failure scenarios
            FailureScenario scenario;
            scenario.id = "CRITICAL_FAILURE_" + name;
            scenario.name = "Critical " + name + " Failure";
            scenario.type = FailureType::UNHANDLED_EXCEPTION;
            scenario.severity = FailureSeverity::CRITICAL;
            scenario.targetComponent = name;
            scenario.description = "Complete failure of critical component " + name;
            scenario.duration = std::chrono::seconds(30);
            scenario.requiresManualIntervention = true;
            
            m_scenarios.push_back(scenario);
        }
    }
}

std::vector<std::string> FailureModeAnalysis::GetSinglePointsOfFailure() const {
    std::vector<std::string> spofs;
    std::lock_guard<std::mutex> lock(m_componentMutex);
    
    for (const auto& [name, component] : m_components) {
        if (component.isCritical && !component.hasRedundancy) {
            spofs.push_back(name);
        }
    }
    
    return spofs;
}

FailureImpact FailureModeAnalysis::AssessFailureImpact(const std::string& component, FailureType type) const {
    std::lock_guard<std::mutex> lock(m_componentMutex);
    
    FailureImpact impact;
    impact.type = type;
    
    auto it = m_components.find(component);
    if (it == m_components.end()) {
        impact.severity = FailureSeverity::LOW;
        impact.description = "Unknown component";
        impact.isRecoverable = true;
        impact.causesCascadingFailure = false;
        impact.recoveryTime = std::chrono::milliseconds(1000);
        return impact;
    }
    
    const SystemComponent& comp = it->second;
    
    // Determine severity based on component criticality and failure type
    if (comp.isCritical) {
        impact.severity = FailureSeverity::CRITICAL;
        impact.recoveryTime = comp.maxAcceptableDowntime * 10;
    } else {
        impact.severity = FailureSeverity::MEDIUM;
        impact.recoveryTime = comp.maxAcceptableDowntime * 2;
    }
    
    // Get affected components
    auto propagationIt = m_failurePropagationGraph.find(component);
    if (propagationIt != m_failurePropagationGraph.end()) {
        impact.affectedComponents = propagationIt->second;
    }
    
    impact.description = "Failure in " + component + " of type " + std::to_string(static_cast<int>(type));
    impact.isRecoverable = !comp.isCritical || comp.hasRedundancy;
    impact.causesCascadingFailure = !impact.affectedComponents.empty();
    
    return impact;
}

std::map<std::string, double> FailureModeAnalysis::CalculateRiskScores() const {
    std::lock_guard<std::mutex> lock(m_componentMutex);
    std::map<std::string, double> riskScores;
    
    for (const auto& [name, component] : m_components) {
        riskScores[name] = CalculateComponentRiskScore(component);
    }
    
    return riskScores;
}

double FailureModeAnalysis::CalculateComponentRiskScore(const SystemComponent& component) const {
    double score = 0.0;
    
    // Criticality factor (0-40 points)
    if (component.isCritical) {
        score += 40.0;
    }
    
    // Dependency factor (0-30 points)
    score += component.dependents.size() * 3.0;
    
    // Recovery time factor (0-20 points)
    double recoveryTimeSeconds = component.maxAcceptableDowntime.count() / 1000.0;
    if (recoveryTimeSeconds < 0.1) {
        score += 20.0;
    } else if (recoveryTimeSeconds < 1.0) {
        score += 15.0;
    } else if (recoveryTimeSeconds < 5.0) {
        score += 10.0;
    } else {
        score += 5.0;
    }
    
    // Redundancy factor (0-10 points)
    if (!component.hasRedundancy) {
        score += 10.0;
    }
    
    return std::min(score, 100.0);
}

void FailureModeAnalysis::StartHealthMonitoring() {
    m_monitoringActive = true;
    
    // Start monitoring threads for each component
    for (const auto& [name, component] : m_components) {
        std::thread monitorThread([this, name]() {
            MonitorComponentHealth(name);
        });
        monitorThread.detach();
    }
}

void FailureModeAnalysis::StopHealthMonitoring() {
    m_monitoringActive = false;
}

void FailureModeAnalysis::MonitorComponentHealth(const std::string& component) {
    while (m_monitoringActive) {
        // Simulate health monitoring
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // In a real implementation, this would check actual component health
        // For now, we'll simulate random health issues
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        
        if (dis(gen) < 0.01) { // 1% chance of health issue
            std::cout << "Health monitoring detected issue in " << component << std::endl;
        }
    }
}

bool FailureModeAnalysis::IsSystemHealthy() const {
    // Simple health check - in reality this would be more sophisticated
    return m_monitoringActive;
}

std::map<std::string, std::string> FailureModeAnalysis::GetSystemHealthStatus() const {
    std::map<std::string, std::string> status;
    
    std::lock_guard<std::mutex> lock(m_componentMutex);
    for (const auto& [name, component] : m_components) {
        status[name] = "OPERATIONAL"; // Simplified for now
    }
    
    return status;
}

void FailureModeAnalysis::GenerateFailureModeReport(const std::string& filename) const {
    std::ofstream report(filename);
    if (!report.is_open()) {
        return;
    }
    
    report << "NEONGLYPH FAILURE MODE ANALYSIS REPORT\n";
    report << "=====================================\n\n";
    
    report << "SYSTEM COMPONENTS:\n";
    std::lock_guard<std::mutex> lock(m_componentMutex);
    for (const auto& [name, component] : m_components) {
        report << "- " << name << " (" << component.type << ")\n";
        report << "  Critical: " << (component.isCritical ? "YES" : "NO") << "\n";
        report << "  Redundancy: " << (component.hasRedundancy ? "YES" : "NO") << "\n";
        report << "  Dependencies: " << component.dependencies.size() << "\n";
        report << "  Dependents: " << component.dependents.size() << "\n\n";
    }
    
    report << "SINGLE POINTS OF FAILURE:\n";
    auto spofs = GetSinglePointsOfFailure();
    for (const auto& spof : spofs) {
        report << "- " << spof << "\n";
    }
    
    report.close();
}

std::string FailureModeAnalysis::GetFailureModeSummary() const {
    std::stringstream summary;
    
    summary << "Failure Mode Analysis Summary:\n";
    summary << "Total Components: " << m_components.size() << "\n";
    summary << "Single Points of Failure: " << GetSinglePointsOfFailure().size() << "\n";
    summary << "Registered Failure Scenarios: " << m_scenarios.size() << "\n";
    
    return summary.str();
}

} // namespace Chaos
} // namespace NeonGlyph