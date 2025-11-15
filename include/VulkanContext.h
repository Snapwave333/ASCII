#pragma once

#include "NeonGlyph.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <array>
#include <memory>

namespace NeonGlyph {

// Forward declarations
struct QueueFamilyIndices;
struct SwapchainSupportDetails;

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    Result Initialize(const Config& config);
    void Shutdown();
    
    // Surface management
    Result AttachSurface(VkSurfaceKHR surface);
    bool HasSurface() const { return m_surface != VK_NULL_HANDLE; }
    
    // Core Vulkan objects
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetComputeQueue() const { return m_computeQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    VkCommandPool GetCommandPool() const { return m_commandPool; }
    VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }
    
    // Swapchain
    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
    VkFormat GetSwapchainFormat() const { return m_swapchainFormat; }
    VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }
    const std::vector<VkImageView>& GetSwapchainImageViews() const { return m_swapchainImageViews; }
    Result UpdateFramePixels(const void* data, size_t size);
    
    // Compute pipeline
    VkPipeline GetComputePipeline() const { return m_computePipeline; }
    VkPipelineLayout GetComputePipelineLayout() const { return m_computePipelineLayout; }
    
    // Synchronization
    uint32_t GetCurrentFrameIndex() const { return m_currentFrame; }
    VkFence GetCurrentFence() const { return m_inFlightFences[m_currentFrame]; }
    VkSemaphore GetCurrentImageAvailableSemaphore() const { return m_imageAvailableSemaphores[m_currentFrame]; }
    VkSemaphore GetCurrentRenderFinishedSemaphore() const { return m_renderFinishedSemaphores[m_currentFrame]; }
    
    // Frame management
    Result BeginFrame();
    Result EndFrame();
    Result SubmitComputeWork(VkCommandBuffer commandBuffer);
    
    // Buffer management
    Result CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                       VkBuffer& buffer, VkDeviceMemory& memory);
    Result CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                      VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                      VkImage& image, VkDeviceMemory& memory);
    
    // Utility
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    
private:
    // Vulkan instance and devices
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_computeQueue;
    uint32_t m_graphicsQueueFamily;
    uint32_t m_computeQueueFamily;
    uint32_t m_presentQueueFamily;
    
    // Surface and swapchain
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    VkFormat m_swapchainFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    uint32_t m_currentImageIndex;
    VkBuffer m_stagingBuffer;
    VkDeviceMemory m_stagingMemory;
    VkDeviceSize m_stagingSize;
    bool m_hasPendingFrame;
    
    // Command pools and buffers
    VkCommandPool m_commandPool;
    VkCommandPool m_computeCommandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;
    
    // Synchronization
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    uint32_t m_currentFrame;
    VkQueue m_presentQueue;
    
    // Compute pipeline
    VkPipeline m_computePipeline;
    VkPipelineLayout m_computePipelineLayout;
    VkDescriptorSetLayout m_computeDescriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    
    // Memory management
    VkPhysicalDeviceMemoryProperties m_memoryProperties;
    
    // Configuration
    Config m_config;
    
public:
    // Individual initialization methods (for step-by-step initialization)
    Result CreateInstance();
    Result SelectPhysicalDevice();
    Result CreateLogicalDevice();
    Result CreateSurface();
    Result CreateSwapchain();
    Result CreateImageViews();
    Result CreateFrameResources();
    Result CreateCommandPools();
    Result CreateCommandBuffers();
    Result CreateSyncObjects();
    Result CreateComputePipeline();
    Result CreateDescriptorPool();
    
private:
    
    // Validation layers
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    
    // Device selection
    bool IsDeviceSuitable(VkPhysicalDevice device);
    struct QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    
    // Swapchain support
    struct SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    // Shader management
    Result CreateShaderModule(const std::vector<uint32_t>& code, VkShaderModule& shaderModule);
    
    // Helper structs (moved outside class for proper namespace scope)
    // Forward declarations are sufficient here
};

// Standalone struct definitions (moved outside class for proper namespace scope)
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> presentFamily;
    
    bool IsComplete() {
        return graphicsFamily.has_value() && computeFamily.has_value();
    }
    
    bool IsCompleteWithPresentation() {
        return graphicsFamily.has_value() && computeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

} // namespace NeonGlyph
