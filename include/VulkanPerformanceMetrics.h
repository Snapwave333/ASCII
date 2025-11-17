#pragma once

#include "NeonGlyph.h"
#include <vulkan/vulkan.h>
#include <chrono>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace NeonGlyph {

struct VulkanPerformanceMetrics {
    // Frame timing metrics
    float frameTimeMs = 0.0f;
    float renderTimeMs = 0.0f;
    float presentTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    
    // Performance counters
    uint32_t drawCalls = 0;
    uint32_t verticesRendered = 0;
    uint32_t trianglesRendered = 0;
    uint32_t fragmentsProcessed = 0;
    
    // Memory usage
    uint64_t gpuMemoryUsed = 0;
    uint64_t gpuMemoryAllocated = 0;
    uint64_t stagingMemoryUsed = 0;
    
    // Pipeline statistics
    uint32_t pipelineSwitches = 0;
    uint32_t descriptorSetBindings = 0;
    uint32_t commandBufferSubmissions = 0;
    
    // Synchronization metrics
    float cpuWaitTimeMs = 0.0f;
    float gpuWaitTimeMs = 0.0f;
    uint32_t fenceWaits = 0;
    uint32_t semaphoreWaits = 0;
    
    // Error and validation metrics
    uint32_t validationErrors = 0;
    uint32_t validationWarnings = 0;
    uint32_t deviceLostEvents = 0;
    
    // Hardware utilization
    float gpuUtilization = 0.0f;
    float memoryBandwidthUsage = 0.0f;
    float textureCacheHitRate = 0.0f;
    
    // Quality metrics
    uint32_t framesDropped = 0;
    uint32_t framesTorn = 0;
    float averageFrameVariance = 0.0f;
    
    // Timing history for trend analysis
    static constexpr size_t HISTORY_SIZE = 1440; // 24 seconds at 60fps
    std::array<float, HISTORY_SIZE> frameTimeHistory;
    std::array<float, HISTORY_SIZE> gpuTimeHistory;
    size_t historyIndex = 0;
    size_t historyCount = 0;
    
    // Real-time performance indicators
    bool isPerformanceStable = true;
    bool isMemoryPressureLow = true;
    bool isThermalThrottling = false;
    
    // Cross-hardware compatibility metrics
    uint32_t deviceSwitches = 0;
    uint32_t driverFallbacks = 0;
    float compatibilityScore = 1.0f;
};

struct VulkanPipelineStatistics {
    uint64_t verticesProcessed;
    uint64_t primitivesProcessed;
    uint64_t vertexShaderInvocations;
    uint64_t fragmentShaderInvocations;
    uint64_t computeShaderInvocations;
    uint64_t tessellationControlShaderInvocations;
    uint64_t tessellationEvaluationShaderInvocations;
    uint64_t geometryShaderInvocations;
    uint64_t clippingInvocations;
    uint64_t clippingPrimitives;
};

struct VulkanMemoryBudget {
    uint64_t totalBudget;
    uint64_t usage;
    uint64_t allocationSize;
    uint64_t availableForReservation;
    uint64_t heapUsage[VK_MAX_MEMORY_HEAPS];
    uint64_t heapBudget[VK_MAX_MEMORY_HEAPS];
};

class VulkanPerformanceMonitor {
public:
    VulkanPerformanceMonitor();
    ~VulkanPerformanceMonitor();
    
    // Initialization and cleanup
    Result Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, uint32_t queueFamilyIndex);
    void Shutdown();
    
    // Frame timing
    void BeginFrame();
    void EndFrame();
    void BeginRenderPass();
    void EndRenderPass();
    void BeginGPUWork();
    void EndGPUWork();
    
    // Performance queries
    void BeginPipelineStatistics(VkCommandBuffer commandBuffer);
    void EndPipelineStatistics(VkCommandBuffer commandBuffer);
    Result CollectPipelineStatistics();
    
    // Memory tracking
    void TrackMemoryAllocation(VkDeviceMemory memory, VkDeviceSize size, uint32_t memoryType);
    void TrackMemoryDeallocation(VkDeviceMemory memory);
    void UpdateMemoryBudget();
    
    // Synchronization tracking
    void RecordFenceWait(float waitTimeMs);
    void RecordSemaphoreWait(float waitTimeMs);
    void RecordCPUWait(float waitTimeMs);
    void RecordGPUWait(float waitTimeMs);
    
    // Error tracking
    void RecordValidationError(const std::string& error);
    void RecordValidationWarning(const std::string& warning);
    void RecordDeviceLost();
    
    // Quality metrics
    void RecordDroppedFrame();
    void RecordFrameTear();
    void RecordFrameVariance(float variance);
    
    // Hardware compatibility
    void RecordDeviceSwitch();
    void RecordDriverFallback();
    void UpdateCompatibilityScore(float score);
    
    // Getters
    const VulkanPerformanceMetrics& GetMetrics() const { return m_metrics; }
    const VulkanPipelineStatistics& GetPipelineStats() const { return m_pipelineStats; }
    const VulkanMemoryBudget& GetMemoryBudget() const { return m_memoryBudget; }
    
    // Performance analysis
    float GetAverageFrameTime() const;
    float GetFrameTimeVariance() const;
    float GetGPUUtilization() const;
    float GetMemoryPressure() const;
    bool IsPerformanceStable() const;
    bool IsOptimalFramePacing() const;
    
    // Reporting
    void LogPerformanceReport();
    void LogMemoryReport();
    void LogPipelineStatistics();
    void GeneratePerformanceProfile();
    
    // Real-time optimization suggestions
    std::vector<std::string> GetOptimizationSuggestions();
    bool ShouldReduceQuality() const;
    bool ShouldIncreaseQuality() const;
    
private:
    // Vulkan objects
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkQueue m_queue;
    uint32_t m_queueFamilyIndex;
    
    // Performance metrics
    VulkanPerformanceMetrics m_metrics;
    VulkanPipelineStatistics m_pipelineStats;
    VulkanMemoryBudget m_memoryBudget;
    
    // Query pools
    VkQueryPool m_pipelineQueryPool;
    VkQueryPool m_timestampQueryPool;
    VkQueryPool m_statisticsQueryPool;
    
    // Timing
    std::chrono::high_resolution_clock::time_point m_frameStartTime;
    std::chrono::high_resolution_clock::time_point m_renderStartTime;
    std::chrono::high_resolution_clock::time_point m_gpuWorkStartTime;
    
    // Memory tracking
    mutable std::mutex m_memoryMutex;
    std::unordered_map<VkDeviceMemory, VkDeviceSize> m_memoryAllocations;
    
    // Thread safety
    mutable std::mutex m_metricsMutex;
    std::atomic<bool> m_isInitialized;
    
    // Performance thresholds
    static constexpr float TARGET_FRAME_TIME_MS = 16.67f; // 60 FPS
    static constexpr float STABLE_FRAME_VARIANCE_THRESHOLD = 1.0f;
    static constexpr float HIGH_MEMORY_PRESSURE_THRESHOLD = 0.85f;
    static constexpr float CRITICAL_MEMORY_PRESSURE_THRESHOLD = 0.95f;
    
    // Private methods
    void InitializeQueryPools();
    void CleanupQueryPools();
    void UpdateHistory();
    void AnalyzePerformanceTrends();
    float CalculateVariance(const std::array<float, VulkanPerformanceMetrics::HISTORY_SIZE>& history, size_t count) const;
};

// Global performance monitor instance
extern std::unique_ptr<VulkanPerformanceMonitor> g_vulkanPerformanceMonitor;

} // namespace NeonGlyph