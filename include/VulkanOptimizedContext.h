#pragma once

#include "VulkanContext.h"
#include "VulkanPerformanceMetrics.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>

namespace NeonGlyph {

// Enhanced Vulkan context with performance optimization and debugging
class VulkanOptimizedContext : public VulkanContext {
public:
    VulkanOptimizedContext();
    ~VulkanOptimizedContext() override;
    
    // Enhanced initialization with performance monitoring
    Result Initialize(const Config& config) override;
    void Shutdown() override;
    
    // Performance-optimized resource management
    Result CreateOptimizedSwapchain();
    Result CreateOptimizedFramebuffers();
    Result CreateOptimizedCommandPools();
    Result CreateOptimizedSynchronizationObjects();
    
    // Advanced debugging and validation
    Result EnableDebugValidation();
    Result SetupDebugMessenger();
    Result EnablePerformanceWarnings();
    
    // Cross-hardware compatibility
    Result DetectAndHandleMultiGPU();
    Result SelectOptimalPhysicalDevice();
    Result ConfigureDeviceFeatures();
    
    // Pixel-perfect rendering optimizations
    Result ConfigurePixelPerfectSettings();
    Result SetupGammaCorrection();
    Result ConfigureColorDepth();
    Result SetupAntiAliasing();
    
    // Memory management optimization
    Result OptimizeMemoryAllocation();
    Result SetupMemoryBudgeting();
    Result ConfigureMemoryPools();
    
    // Command buffer optimization
    Result OptimizeCommandBufferRecording();
    Result SetupCommandBufferPools();
    Result ConfigureMultiThreading();
    
    // Synchronization optimization
    Result OptimizeFrameSynchronization();
    Result SetupTimelineSemaphores();
    Result ConfigurePresentModes();
    
    // Performance monitoring integration
    Result InitializePerformanceMonitoring();
    Result UpdatePerformanceMetrics();
    Result LogPerformanceReport();
    
    // Error handling and recovery
    Result HandleDeviceLost();
    Result RecoverFromError(VkResult error);
    Result ValidatePipelineState();
    
    // Quality assurance
    Result ValidatePixelAccuracy();
    Result CheckForVisualArtifacts();
    Result VerifyColorAccuracy();
    Result TestDepthStencilOperations();
    
    // Hardware-specific optimizations
    Result ApplyNvidiaOptimizations();
    Result ApplyAMDOptimizations();
    Result ApplyIntelOptimizations();
    Result ApplyMobileOptimizations();
    
    // Advanced features
    Result SetupVariableRateShading();
    Result ConfigureRayTracing();
    Result SetupMeshShaders();
    
    // Getters for performance data
    const VulkanPerformanceMetrics& GetPerformanceMetrics() const;
    std::vector<std::string> GetOptimizationSuggestions() const;
    bool IsPerformanceOptimal() const;
    bool ShouldAdjustQuality() const;
    
    // Real-time debugging
    void EnableRealTimeDebugging();
    void DisableRealTimeDebugging();
    void LogFrameStatistics();
    void DumpPipelineState();
    
private:
    // Performance monitoring
    std::unique_ptr<VulkanPerformanceMonitor> m_performanceMonitor;
    
    // Debug and validation
    VkDebugUtilsMessengerEXT m_debugMessenger;
    bool m_debugEnabled;
    bool m_performanceWarningsEnabled;
    
    // Hardware detection
    enum class GPUVendor {
        Unknown,
        Nvidia,
        AMD,
        Intel,
        Qualcomm,
        ARM,
        Apple
    };
    
    GPUVendor m_gpuVendor;
    std::string m_gpuName;
    uint32_t m_driverVersion;
    
    // Optimization state
    bool m_pixelPerfectEnabled;
    bool m_gammaCorrectionEnabled;
    bool m_antiAliasingEnabled;
    bool m_variableRateShadingEnabled;
    
    // Quality settings
    VkSampleCountFlagBits m_msaaSamples;
    VkFormat m_depthFormat;
    VkFormat m_stencilFormat;
    
    // Memory optimization
    struct MemoryPool {
        VkDeviceMemory memory;
        VkDeviceSize size;
        VkDeviceSize used;
        uint32_t memoryType;
        bool isHostVisible;
    };
    
    std::vector<MemoryPool> m_memoryPools;
    VkDeviceSize m_memoryBudget;
    VkDeviceSize m_memoryUsed;
    
    // Command buffer optimization
    struct CommandBufferPool {
        VkCommandPool pool;
        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t currentIndex;
        bool isMultiThreaded;
    };
    
    std::vector<CommandBufferPool> m_commandBufferPools;
    uint32_t m_currentCommandPool;
    
    // Synchronization optimization
    struct FrameSynchronization {
        VkSemaphore imageAvailable;
        VkSemaphore renderFinished;
        VkSemaphore timelineSemaphore;
        VkFence fence;
        uint64_t timelineValue;
        bool isTimelineEnabled;
    };
    
    std::vector<FrameSynchronization> m_frameSynchronization;
    uint64_t m_currentTimelineValue;
    
    // Present mode optimization
    VkPresentModeKHR m_optimalPresentMode;
    uint32_t m_optimalImageCount;
    bool m_vsyncEnabled;
    bool m_tearingPreventionEnabled;
    
    // Error tracking
    std::vector<VkResult> m_errorHistory;
    std::vector<std::string> m_validationErrors;
    uint32_t m_consecutiveErrors;
    
    // Performance thresholds
    static constexpr float OPTIMAL_FRAME_TIME_MS = 16.67f; // 60 FPS
    static constexpr float MAX_ACCEPTABLE_FRAME_TIME_MS = 33.33f; // 30 FPS
    static constexpr float STABLE_FRAME_VARIANCE_THRESHOLD = 1.0f;
    static constexpr uint32_t MAX_CONSECUTIVE_ERRORS = 5;
    
    // Private methods
    Result CreateDebugMessenger();
    Result SetupValidationLayers();
    Result DetectGPUVendor();
    Result SelectOptimalPresentMode();
    Result ConfigureSwapchainForTearingPrevention();
    Result SetupMemoryAllocationCallbacks();
    Result OptimizePipelineLayout();
    Result ConfigureShaderOptimization();
    Result SetupPipelineCache();
    Result ConfigureDescriptorSetOptimization();
    Result SetupRenderPassOptimization();
    Result OptimizeFramebufferAttachments();
    Result ConfigureSubpassDependencies();
    Result SetupLoadStoreOptimizations();
    Result TestHardwareCapabilities();
    Result ValidateCrossPlatformCompatibility();
    Result SetupErrorRecoveryMechanisms();
    Result ConfigureQualityAdaptation();
    Result SetupRealTimeMonitoring();
    
    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

} // namespace NeonGlyph