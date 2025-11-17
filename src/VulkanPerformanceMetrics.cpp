#include "VulkanPerformanceMetrics.h"
#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace NeonGlyph {

std::unique_ptr<VulkanPerformanceMonitor> g_vulkanPerformanceMonitor = nullptr;

VulkanPerformanceMonitor::VulkanPerformanceMonitor() 
    : m_device(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE)
    , m_queue(VK_NULL_HANDLE)
    , m_queueFamilyIndex(0)
    , m_pipelineQueryPool(VK_NULL_HANDLE)
    , m_timestampQueryPool(VK_NULL_HANDLE)
    , m_statisticsQueryPool(VK_NULL_HANDLE)
    , m_isInitialized(false) {
    
    m_metrics.frameTimeHistory.fill(0.0f);
    m_metrics.gpuTimeHistory.fill(0.0f);
}

VulkanPerformanceMonitor::~VulkanPerformanceMonitor() {
    Shutdown();
}

Result VulkanPerformanceMonitor::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex) {
    if (m_isInitialized) {
        return Result::AlreadyInitialized;
    }
    
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_queue = queue;
    m_queueFamilyIndex = queueFamilyIndex;
    
    try {
        InitializeQueryPools();
        UpdateMemoryBudget();
        
        m_isInitialized = true;
        std::cout << "[VulkanPerformanceMonitor] Initialized successfully" << std::endl;
        return Result::Success;
        
    } catch (const std::exception& e) {
        std::cerr << "[VulkanPerformanceMonitor] Initialization failed: " << e.what() << std::endl;
        return Result::InitializationFailed;
    }
}

void VulkanPerformanceMonitor::Shutdown() {
    if (!m_isInitialized) {
        return;
    }
    
    CleanupQueryPools();
    
    // Log final performance report
    LogPerformanceReport();
    LogMemoryReport();
    LogPipelineStatistics();
    
    m_isInitialized = false;
    std::cout << "[VulkanPerformanceMonitor] Shutdown complete" << std::endl;
}

void VulkanPerformanceMonitor::InitializeQueryPools() {
    // Create timestamp query pool
    VkQueryPoolCreateInfo timestampPoolInfo{};
    timestampPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    timestampPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    timestampPoolInfo.queryCount = 128; // Enough for multiple frames
    
    if (vkCreateQueryPool(m_device, &timestampPoolInfo, nullptr, &m_timestampQueryPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create timestamp query pool");
    }
    
    // Create pipeline statistics query pool
    VkQueryPoolCreateInfo statisticsPoolInfo{};
    statisticsPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    statisticsPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    statisticsPoolInfo.queryCount = 32;
    statisticsPoolInfo.pipelineStatistics = 
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
    
    if (vkCreateQueryPool(m_device, &statisticsPoolInfo, nullptr, &m_statisticsQueryPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create statistics query pool");
    }
}

void VulkanPerformanceMonitor::CleanupQueryPools() {
    if (m_timestampQueryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(m_device, m_timestampQueryPool, nullptr);
        m_timestampQueryPool = VK_NULL_HANDLE;
    }
    
    if (m_statisticsQueryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(m_device, m_statisticsQueryPool, nullptr);
        m_statisticsQueryPool = VK_NULL_HANDLE;
    }
}

void VulkanPerformanceMonitor::BeginFrame() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    m_frameStartTime = std::chrono::high_resolution_clock::now();
    
    // Reset per-frame counters
    m_metrics.drawCalls = 0;
    m_metrics.verticesRendered = 0;
    m_metrics.trianglesRendered = 0;
    m_metrics.fragmentsProcessed = 0;
    m_metrics.pipelineSwitches = 0;
    m_metrics.descriptorSetBindings = 0;
    m_metrics.commandBufferSubmissions = 0;
}

void VulkanPerformanceMonitor::EndFrame() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    auto frameEndTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - m_frameStartTime);
    
    m_metrics.frameTimeMs = static_cast<float>(duration.count()) / 1000.0f;
    
    UpdateHistory();
    AnalyzePerformanceTrends();
}

void VulkanPerformanceMonitor::BeginRenderPass() {
    m_renderStartTime = std::chrono::high_resolution_clock::now();
}

void VulkanPerformanceMonitor::EndRenderPass() {
    auto renderEndTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(renderEndTime - m_renderStartTime);
    
    m_metrics.renderTimeMs = static_cast<float>(duration.count()) / 1000.0f;
}

void VulkanPerformanceMonitor::BeginGPUWork() {
    m_gpuWorkStartTime = std::chrono::high_resolution_clock::now();
}

void VulkanPerformanceMonitor::EndGPUWork() {
    auto gpuWorkEndTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(gpuWorkEndTime - m_gpuWorkStartTime);
    
    m_metrics.gpuTimeMs = static_cast<float>(duration.count()) / 1000.0f;
}

void VulkanPerformanceMonitor::BeginPipelineStatistics(VkCommandBuffer commandBuffer) {
    vkCmdResetQueryPool(commandBuffer, m_statisticsQueryPool, 0, 1);
    vkCmdBeginQuery(commandBuffer, m_statisticsQueryPool, 0, VK_QUERY_CONTROL_PRECISE_BIT);
}

void VulkanPerformanceMonitor::EndPipelineStatistics(VkCommandBuffer commandBuffer) {
    vkCmdEndQuery(commandBuffer, m_statisticsQueryPool, 0);
}

Result VulkanPerformanceMonitor::CollectPipelineStatistics() {
    if (!m_isInitialized) {
        return Result::NotInitialized;
    }
    
    uint64_t statistics[8];
    VkResult result = vkGetQueryPoolResults(
        m_device, 
        m_statisticsQueryPool, 
        0, 1, 
        sizeof(statistics), statistics, 
        sizeof(uint64_t), 
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
    );
    
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        
        m_pipelineStats.verticesProcessed = statistics[0];
        m_pipelineStats.primitivesProcessed = statistics[1];
        m_pipelineStats.vertexShaderInvocations = statistics[2];
        m_pipelineStats.fragmentShaderInvocations = statistics[3];
        m_pipelineStats.clippingInvocations = statistics[4];
        m_pipelineStats.clippingPrimitives = statistics[5];
        
        // Update derived metrics
        m_metrics.verticesRendered = static_cast<uint32_t>(m_pipelineStats.verticesProcessed);
        m_metrics.trianglesRendered = static_cast<uint32_t>(m_pipelineStats.primitivesProcessed / 3);
        m_metrics.fragmentsProcessed = static_cast<uint32_t>(m_pipelineStats.fragmentShaderInvocations);
    }
    
    return result == VK_SUCCESS ? Result::Success : Result::Error;
}

void VulkanPerformanceMonitor::TrackMemoryAllocation(VkDeviceMemory memory, VkDeviceSize size, uint32_t memoryType) {
    std::lock_guard<std::mutex> lock(m_memoryMutex);
    
    m_memoryAllocations[memory] = size;
    m_metrics.gpuMemoryAllocated += size;
    
    // Update memory budget
    UpdateMemoryBudget();
}

void VulkanPerformanceMonitor::TrackMemoryDeallocation(VkDeviceMemory memory) {
    std::lock_guard<std::mutex> lock(m_memoryMutex);
    
    auto it = m_memoryAllocations.find(memory);
    if (it != m_memoryAllocations.end()) {
        m_metrics.gpuMemoryAllocated -= it->second;
        m_memoryAllocations.erase(it);
        
        // Update memory budget
        UpdateMemoryBudget();
    }
}

void VulkanPerformanceMonitor::UpdateMemoryBudget() {
    // Get memory properties from Vulkan
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);
    
    // Update budget information
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        m_memoryBudget.heapBudget[i] = memoryProperties.memoryHeaps[i].size;
        m_memoryBudget.heapUsage[i] = 0; // This would need VK_EXT_memory_budget extension
    }
    
    m_memoryBudget.totalBudget = 0;
    m_memoryBudget.usage = m_metrics.gpuMemoryAllocated;
    
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
        m_memoryBudget.totalBudget += m_memoryBudget.heapBudget[i];
    }
}

void VulkanPerformanceMonitor::RecordFenceWait(float waitTimeMs) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.fenceWaits++;
    m_metrics.cpuWaitTimeMs += waitTimeMs;
}

void VulkanPerformanceMonitor::RecordSemaphoreWait(float waitTimeMs) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.semaphoreWaits++;
    m_metrics.gpuWaitTimeMs += waitTimeMs;
}

void VulkanPerformanceMonitor::RecordCPUWait(float waitTimeMs) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.cpuWaitTimeMs += waitTimeMs;
}

void VulkanPerformanceMonitor::RecordGPUWait(float waitTimeMs) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.gpuWaitTimeMs += waitTimeMs;
}

void VulkanPerformanceMonitor::RecordValidationError(const std::string& error) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.validationErrors++;
    std::cerr << "[Vulkan Validation Error] " << error << std::endl;
}

void VulkanPerformanceMonitor::RecordValidationWarning(const std::string& warning) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.validationWarnings++;
    std::cout << "[Vulkan Validation Warning] " << warning << std::endl;
}

void VulkanPerformanceMonitor::RecordDeviceLost() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.deviceLostEvents++;
    std::cerr << "[Vulkan] Device lost event recorded" << std::endl;
}

void VulkanPerformanceMonitor::RecordDroppedFrame() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.framesDropped++;
}

void VulkanPerformanceMonitor::RecordFrameTear() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.framesTorn++;
}

void VulkanPerformanceMonitor::RecordFrameVariance(float variance) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.averageFrameVariance = variance;
}

void VulkanPerformanceMonitor::RecordDeviceSwitch() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.deviceSwitches++;
}

void VulkanPerformanceMonitor::RecordDriverFallback() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.driverFallbacks++;
}

void VulkanPerformanceMonitor::UpdateCompatibilityScore(float score) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_metrics.compatibilityScore = score;
}

float VulkanPerformanceMonitor::GetAverageFrameTime() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    if (m_metrics.historyCount == 0) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    size_t count = std::min(m_metrics.historyCount, VulkanPerformanceMetrics::HISTORY_SIZE);
    
    for (size_t i = 0; i < count; ++i) {
        sum += m_metrics.frameTimeHistory[i];
    }
    
    return sum / static_cast<float>(count);
}

float VulkanPerformanceMonitor::GetFrameTimeVariance() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    size_t count = std::min(m_metrics.historyCount, VulkanPerformanceMetrics::HISTORY_SIZE);
    if (count < 2) {
        return 0.0f;
    }
    
    return CalculateVariance(m_metrics.frameTimeHistory, count);
}

float VulkanPerformanceMonitor::GetGPUUtilization() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    if (m_metrics.frameTimeMs == 0.0f) {
        return 0.0f;
    }
    
    return (m_metrics.gpuTimeMs / m_metrics.frameTimeMs) * 100.0f;
}

float VulkanPerformanceMonitor::GetMemoryPressure() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    if (m_memoryBudget.totalBudget == 0) {
        return 0.0f;
    }
    
    return static_cast<float>(m_metrics.gpuMemoryAllocated) / static_cast<float>(m_memoryBudget.totalBudget);
}

bool VulkanPerformanceMonitor::IsPerformanceStable() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    float variance = GetFrameTimeVariance();
    return variance < STABLE_FRAME_VARIANCE_THRESHOLD;
}

bool VulkanPerformanceMonitor::IsOptimalFramePacing() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    float avgFrameTime = GetAverageFrameTime();
    return std::abs(avgFrameTime - TARGET_FRAME_TIME_MS) < 2.0f;
}

void VulkanPerformanceMonitor::UpdateHistory() {
    m_metrics.frameTimeHistory[m_metrics.historyIndex] = m_metrics.frameTimeMs;
    m_metrics.gpuTimeHistory[m_metrics.historyIndex] = m_metrics.gpuTimeMs;
    
    m_metrics.historyIndex = (m_metrics.historyIndex + 1) % VulkanPerformanceMetrics::HISTORY_SIZE;
    if (m_metrics.historyCount < VulkanPerformanceMetrics::HISTORY_SIZE) {
        m_metrics.historyCount++;
    }
}

void VulkanPerformanceMonitor::AnalyzePerformanceTrends() {
    // Analyze frame time stability
    float variance = GetFrameTimeVariance();
    m_metrics.isPerformanceStable = variance < STABLE_FRAME_VARIANCE_THRESHOLD;
    
    // Analyze memory pressure
    float memoryPressure = GetMemoryPressure();
    m_metrics.isMemoryPressureLow = memoryPressure < HIGH_MEMORY_PRESSURE_THRESHOLD;
    
    // Check for thermal throttling indicators
    if (m_metrics.frameTimeMs > TARGET_FRAME_TIME_MS * 1.5f && variance > 5.0f) {
        m_metrics.isThermalThrottling = true;
    } else {
        m_metrics.isThermalThrottling = false;
    }
}

float VulkanPerformanceMonitor::CalculateVariance(const std::array<float, VulkanPerformanceMetrics::HISTORY_SIZE>& history, size_t count) const {
    if (count < 2) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += history[i];
    }
    
    float mean = sum / static_cast<float>(count);
    
    float varianceSum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float diff = history[i] - mean;
        varianceSum += diff * diff;
    }
    
    return varianceSum / static_cast<float>(count - 1);
}

void VulkanPerformanceMonitor::LogPerformanceReport() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    std::cout << "\n=== VULKAN PERFORMANCE REPORT ===" << std::endl;
    std::cout << "Frame Time: " << std::fixed << std::setprecision(2) << m_metrics.frameTimeMs << "ms" << std::endl;
    std::cout << "Render Time: " << m_metrics.renderTimeMs << "ms" << std::endl;
    std::cout << "GPU Time: " << m_metrics.gpuTimeMs << "ms" << std::endl;
    std::cout << "Present Time: " << m_metrics.presentTimeMs << "ms" << std::endl;
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << (1000.0f / m_metrics.frameTimeMs) << std::endl;
    std::cout << "GPU Utilization: " << std::fixed << std::setprecision(1) << GetGPUUtilization() << "%" << std::endl;
    std::cout << "Frame Variance: " << std::fixed << std::setprecision(3) << GetFrameTimeVariance() << "ms²" << std::endl;
    std::cout << "Performance Stable: " << (m_metrics.isPerformanceStable ? "Yes" : "No") << std::endl;
    std::cout << "Optimal Frame Pacing: " << (IsOptimalFramePacing() ? "Yes" : "No") << std::endl;
    std::cout << "================================" << std::endl;
}

void VulkanPerformanceMonitor::LogMemoryReport() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    std::cout << "\n=== VULKAN MEMORY REPORT ===" << std::endl;
    std::cout << "GPU Memory Allocated: " << (m_metrics.gpuMemoryAllocated / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "GPU Memory Used: " << (m_metrics.gpuMemoryUsed / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "Staging Memory Used: " << (m_metrics.stagingMemoryUsed / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "Memory Pressure: " << std::fixed << std::setprecision(1) << (GetMemoryPressure() * 100.0f) << "%" << std::endl;
    std::cout << "Memory Pressure Low: " << (m_metrics.isMemoryPressureLow ? "Yes" : "No") << std::endl;
    
    if (m_memoryBudget.totalBudget > 0) {
        std::cout << "Total Budget: " << (m_memoryBudget.totalBudget / (1024 * 1024)) << " MB" << std::endl;
    }
    std::cout << "==============================" << std::endl;
}

void VulkanPerformanceMonitor::LogPipelineStatistics() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    std::cout << "\n=== VULKAN PIPELINE STATISTICS ===" << std::endl;
    std::cout << "Vertices Processed: " << m_pipelineStats.verticesProcessed << std::endl;
    std::cout << "Primitives Processed: " << m_pipelineStats.primitivesProcessed << std::endl;
    std::cout << "Vertex Shader Invocations: " << m_pipelineStats.vertexShaderInvocations << std::endl;
    std::cout << "Fragment Shader Invocations: " << m_pipelineStats.fragmentShaderInvocations << std::endl;
    std::cout << "Clipping Invocations: " << m_pipelineStats.clippingInvocations << std::endl;
    std::cout << "Clipping Primitives: " << m_pipelineStats.clippingPrimitives << std::endl;
    std::cout << "Draw Calls: " << m_metrics.drawCalls << std::endl;
    std::cout << "Pipeline Switches: " << m_metrics.pipelineSwitches << std::endl;
    std::cout << "Descriptor Set Bindings: " << m_metrics.descriptorSetBindings << std::endl;
    std::cout << "Command Buffer Submissions: " << m_metrics.commandBufferSubmissions << std::endl;
    std::cout << "====================================" << std::endl;
}

std::vector<std::string> VulkanPerformanceMonitor::GetOptimizationSuggestions() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    std::vector<std::string> suggestions;
    
    // Frame time optimization
    if (m_metrics.frameTimeMs > TARGET_FRAME_TIME_MS * 1.2f) {
        suggestions.push_back("Frame time exceeds target - consider reducing rendering complexity");
    }
    
    // GPU utilization optimization
    float gpuUtilization = GetGPUUtilization();
    if (gpuUtilization < 50.0f) {
        suggestions.push_back("Low GPU utilization - consider parallelizing CPU work");
    } else if (gpuUtilization > 95.0f) {
        suggestions.push_back("High GPU utilization - consider reducing shader complexity");
    }
    
    // Memory optimization
    float memoryPressure = GetMemoryPressure();
    if (memoryPressure > HIGH_MEMORY_PRESSURE_THRESHOLD) {
        suggestions.push_back("High memory pressure - consider reducing texture resolution or using compression");
    }
    
    // Frame stability optimization
    if (!m_metrics.isPerformanceStable) {
        suggestions.push_back("Unstable frame times - consider using frame pacing or reducing load variance");
    }
    
    // Synchronization optimization
    if (m_metrics.cpuWaitTimeMs > 5.0f) {
        suggestions.push_back("High CPU wait time - consider optimizing synchronization points");
    }
    
    if (m_metrics.gpuWaitTimeMs > 5.0f) {
        suggestions.push_back("High GPU wait time - consider optimizing command buffer submission");
    }
    
    // Quality optimization
    if (m_metrics.framesDropped > 10) {
        suggestions.push_back("Frame drops detected - consider enabling VSync or triple buffering");
    }
    
    return suggestions;
}

bool VulkanPerformanceMonitor::ShouldReduceQuality() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    return (m_metrics.frameTimeMs > TARGET_FRAME_TIME_MS * 1.5f) ||
           (GetMemoryPressure() > CRITICAL_MEMORY_PRESSURE_THRESHOLD) ||
           (m_metrics.framesDropped > 5) ||
           (!m_metrics.isPerformanceStable && m_metrics.averageFrameVariance > 5.0f);
}

bool VulkanPerformanceMonitor::ShouldIncreaseQuality() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    return (m_metrics.frameTimeMs < TARGET_FRAME_TIME_MS * 0.8f) &&
           (GetMemoryPressure() < HIGH_MEMORY_PRESSURE_THRESHOLD) &&
           (m_metrics.framesDropped == 0) &&
           m_metrics.isPerformanceStable &&
           (GetGPUUtilization() < 80.0f);
}

} // namespace NeonGlyph