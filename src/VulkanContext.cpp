#include "VulkanContext.h"

// Windows headers must come before Vulkan platform headers
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include "Logger.h"

namespace NeonGlyph {

VulkanContext::VulkanContext() 
    : m_instance(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE)
    , m_device(VK_NULL_HANDLE)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_computeQueue(VK_NULL_HANDLE)
    , m_graphicsQueueFamily(0)
    , m_computeQueueFamily(0)
    , m_presentQueueFamily(0)
    , m_surface(VK_NULL_HANDLE)
    , m_swapchain(VK_NULL_HANDLE)
    , m_swapchainFormat(VK_FORMAT_UNDEFINED)
    , m_swapchainExtent{0, 0}
    , m_currentImageIndex(0)
    , m_stagingBuffer(VK_NULL_HANDLE)
    , m_stagingMemory(VK_NULL_HANDLE)
    , m_stagingSize(0)
    , m_hasPendingFrame(false)
    , m_commandPool(VK_NULL_HANDLE)
    , m_computeCommandPool(VK_NULL_HANDLE)
    , m_computePipeline(VK_NULL_HANDLE)
    , m_computePipelineLayout(VK_NULL_HANDLE)
    , m_computeDescriptorSetLayout(VK_NULL_HANDLE)
    , m_descriptorPool(VK_NULL_HANDLE)
    , m_currentFrame(0) {
}

VulkanContext::~VulkanContext() {
    Shutdown();
}

Result VulkanContext::Initialize(const Config& config) {
    m_config = config;
    
    Result result = CreateInstance();
    if (result != Result::Success) return result;
    
    result = SelectPhysicalDevice();
    if (result != Result::Success) return result;
    
    result = CreateLogicalDevice();
    if (result != Result::Success) return result;
    
    result = CreateCommandPools();
    if (result != Result::Success) return result;
    
    result = CreateCommandBuffers();
    if (result != Result::Success) return result;
    
    result = CreateSyncObjects();
    if (result != Result::Success) return result;
    
    result = CreateComputePipeline();
    if (result != Result::Success) return result;
    
    result = CreateDescriptorPool();
    if (result != Result::Success) return result;
    if (m_surface != VK_NULL_HANDLE) {
        result = CreateSwapchain();
        if (result != Result::Success) return result;
        result = CreateImageViews();
        if (result != Result::Success) return result;
        result = CreateFrameResources();
        if (result != Result::Success) return result;
    }
    
    return Result::Success;
}

void VulkanContext::Shutdown() {
    if (m_device == VK_NULL_HANDLE) return;
    
    vkDeviceWaitIdle(m_device);
    
    // Cleanup in reverse order
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }
    
    if (m_computePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
    }
    
    if (m_computePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
    }
    
    // Cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }
        if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        }
        if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        }
    }
    
    // Cleanup command buffers and pools
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }
    
    if (m_computeCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
    }
    
    // Cleanup swapchain
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }
    
    // Cleanup surface
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
    
    // Cleanup device
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }
    
    // Cleanup instance
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

Result VulkanContext::CreateInstance() {
    std::cout << "[VulkanContext] Creating Vulkan instance with validation..." << std::endl;
    
    // CRITICAL SAFETY CHECK: Ensure Vulkan loader is available and functional
    // This prevents crashes in vulkan-1.dll during multi-GPU initialization
    std::cout << "[VulkanContext] Performing pre-initialization safety checks..." << std::endl;
    
    // Check if Vulkan functions are available
    if (vkCreateInstance == nullptr) {
        std::cerr << "[VulkanContext] CRITICAL ERROR: vkCreateInstance function not available" << std::endl;
        std::cerr << "[VulkanContext] This indicates a corrupted Vulkan installation or missing drivers" << std::endl;
        return Result::InitializationFailed;
    }
    
    // Test basic Vulkan functionality with minimal instance creation
    // This helps identify multi-GPU system issues before full initialization
    std::cout << "[VulkanContext] Testing basic Vulkan loader functionality..." << std::endl;
    
    uint32_t layerCount = 0;
    VkResult testResult = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (testResult != VK_SUCCESS && testResult != VK_INCOMPLETE) {
        std::cerr << "[VulkanContext] CRITICAL ERROR: Vulkan loader test failed. Error: " << testResult << std::endl;
        std::cerr << "[VulkanContext] This typically occurs on multi-GPU systems with driver conflicts" << std::endl;
        std::cerr << "[VulkanContext] Applying automatic multi-GPU safety fix..." << std::endl;
        
        // Apply automatic multi-GPU safety fix
        #ifdef _WIN32
        // Force NVIDIA GPU usage on Windows multi-GPU systems
        _putenv("VK_ICD_FILENAMES=C:\\Windows\\System32\\DriverStore\\FileRepository\\nvami.inf_amd64_f6ed7dd5d89ca48a\\nv-vk64.json");
        _putenv("VK_INSTANCE_LAYERS=");
        _putenv("VK_LOADER_LAYERS_DISABLE=1");
        _putenv("DISABLE_VULKAN_VALIDATION=1");
        std::cout << "[VulkanContext] Applied automatic multi-GPU safety fix" << std::endl;
        #endif
        
        // Retry the test after applying the fix
        testResult = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (testResult != VK_SUCCESS && testResult != VK_INCOMPLETE) {
            std::cerr << "[VulkanContext] CRITICAL ERROR: Safety fix failed. Error: " << testResult << std::endl;
            std::cerr << "[VulkanContext] Manual intervention required - set VK_ICD_FILENAMES environment variable" << std::endl;
            return Result::InitializationFailed;
        }
    }
    
    std::cout << "[VulkanContext] Vulkan loader test successful, found " << layerCount << " layer(s)" << std::endl;
    
    // Application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "NeonGlyph";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NeonGlyph Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    std::cout << "[VulkanContext] Vulkan API Version: 1.2" << std::endl;
    
    // Instance create info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // Extensions
    std::vector<const char*> extensions = GetRequiredExtensions();
    std::cout << "[VulkanContext] Required extensions count: " << extensions.size() << std::endl;
    for (size_t i = 0; i < extensions.size(); i++) {
        std::cout << "[VulkanContext] Extension[" << i << "]: " << extensions[i] << std::endl;
    }
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    // Validation layers (for debug builds and enhanced error reporting)
    std::vector<const char*> validationLayers;
    bool enableValidation = true; // Enable validation for detailed error reporting
    
    if (enableValidation && CheckValidationLayerSupport()) {
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        std::cout << "[VulkanContext] Validation layers enabled" << std::endl;
    } else {
        std::cout << "[VulkanContext] Validation layers disabled or not available" << std::endl;
    }
    
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    
    std::cout << "[VulkanContext] Creating Vulkan instance..." << std::endl;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    
    if (result != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Failed to create Vulkan instance. Error code: " << result << std::endl;
        
        // Provide detailed error information
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                std::cerr << "[VulkanContext] Error: Out of host memory" << std::endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                std::cerr << "[VulkanContext] Error: Out of device memory" << std::endl;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                std::cerr << "[VulkanContext] Error: Initialization failed - this is the multi-GPU crash" << std::endl;
                std::cerr << "[VulkanContext] SOLUTION: Set VK_ICD_FILENAMES to force single GPU usage" << std::endl;
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                std::cerr << "[VulkanContext] Error: Layer not present" << std::endl;
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                std::cerr << "[VulkanContext] Error: Extension not present" << std::endl;
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                std::cerr << "[VulkanContext] Error: Incompatible driver" << std::endl;
                std::cerr << "[VulkanContext] This often occurs with mixed GPU vendors (NVIDIA + Intel)" << std::endl;
                break;
            default:
                std::cerr << "[VulkanContext] Error: Unknown error code" << std::endl;
                break;
        }
        
        return Result::InitializationFailed;
    }
    
    std::cout << "[VulkanContext] Vulkan instance created successfully" << std::endl;
    return Result::Success;
}

Result VulkanContext::SelectPhysicalDevice() {
    std::cout << "[VulkanContext] Enumerating physical devices..." << std::endl;
    
    uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    if (result != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Failed to enumerate physical devices. Error: " << result << std::endl;
        return Result::InitializationFailed;
    }
    
    if (deviceCount == 0) {
        std::cerr << "[VulkanContext] No Vulkan-compatible physical devices found!" << std::endl;
        return Result::InitializationFailed;
    }
    
    std::cout << "[VulkanContext] Found " << deviceCount << " physical device(s)" << std::endl;
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    if (result != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Failed to get physical device handles. Error: " << result << std::endl;
        return Result::InitializationFailed;
    }
    
    // Log device information
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        
        std::cout << "[VulkanContext] Device[" << i << "]: " << deviceProperties.deviceName << std::endl;
        std::cout << "[VulkanContext]   Type: " << deviceProperties.deviceType << std::endl;
        std::cout << "[VulkanContext]   API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "." 
                  << VK_VERSION_MINOR(deviceProperties.apiVersion) << "." 
                  << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
        std::cout << "[VulkanContext]   Driver Version: " << deviceProperties.driverVersion << std::endl;
        std::cout << "[VulkanContext]   Vendor ID: 0x" << std::hex << deviceProperties.vendorID << std::dec << std::endl;
        std::cout << "[VulkanContext]   Device ID: 0x" << std::hex << deviceProperties.deviceID << std::dec << std::endl;
    }
    
    // Select first suitable device
    for (const auto& device : devices) {
        std::cout << "[VulkanContext] Checking device suitability..." << std::endl;
        if (IsDeviceSuitable(device)) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            std::cout << "[VulkanContext] Selected device: " << deviceProperties.deviceName << std::endl;
            m_physicalDevice = device;
            break;
        } else {
            std::cout << "[VulkanContext] Device not suitable for requirements" << std::endl;
        }
    }
    
    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Failed to find a suitable GPU!" << std::endl;
        std::cerr << "[VulkanContext] Requirements: Graphics queue family, Compute queue family, Presentation support" << std::endl;
        return Result::InitializationFailed;
    }
    
    // Get memory properties
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
    
    std::cout << "[VulkanContext] Physical device selected and memory properties obtained" << std::endl;
    return Result::Success;
}

Result VulkanContext::CreateLogicalDevice() {
    std::cout << "[VulkanContext] Creating logical device..." << std::endl;
    
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    
    // Debug output for queue families
    std::cout << "[VulkanContext] Queue families found:" << std::endl;
    std::cout << "[VulkanContext]   Graphics family: " << (indices.graphicsFamily.has_value() ? std::to_string(indices.graphicsFamily.value()) : "none") << std::endl;
    std::cout << "[VulkanContext]   Compute family: " << (indices.computeFamily.has_value() ? std::to_string(indices.computeFamily.value()) : "none") << std::endl;
    std::cout << "[VulkanContext]   Present family: " << (indices.presentFamily.has_value() ? std::to_string(indices.presentFamily.value()) : "none") << std::endl;
    std::cout << "[VulkanContext]   Surface status: " << (m_surface != VK_NULL_HANDLE ? "attached" : "not attached") << std::endl;
    
    // Validate that required queue families are available
    if (!indices.graphicsFamily.has_value()) {
        std::cerr << "[VulkanContext] Error: No graphics queue family found" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (!indices.computeFamily.has_value()) {
        std::cerr << "[VulkanContext] Error: No compute queue family found" << std::endl;
        return Result::InitializationFailed;
    }
    
    std::cout << "[VulkanContext] Graphics queue family: " << indices.graphicsFamily.value() << std::endl;
    std::cout << "[VulkanContext] Compute queue family: " << indices.computeFamily.value() << std::endl;
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.computeFamily.value()
    };
    if (indices.presentFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.presentFamily.value());
    }
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
    
    // Device create info
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    // Extensions
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME
    };
    
    std::cout << "[VulkanContext] Required device extensions:" << std::endl;
    for (size_t i = 0; i < deviceExtensions.size(); i++) {
        std::cout << "[VulkanContext]   Extension[" << i << "]: " << deviceExtensions[i] << std::endl;
    }
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    std::cout << "[VulkanContext] Creating logical device..." << std::endl;
    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    
    if (result != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Failed to create logical device. Error: " << result << std::endl;
        
        // Provide detailed error information
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                std::cerr << "[VulkanContext] Error: Out of host memory" << std::endl;
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                std::cerr << "[VulkanContext] Error: Out of device memory" << std::endl;
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                std::cerr << "[VulkanContext] Error: Device initialization failed" << std::endl;
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                std::cerr << "[VulkanContext] Error: Required device extension not present" << std::endl;
                break;
            case VK_ERROR_FEATURE_NOT_PRESENT:
                std::cerr << "[VulkanContext] Error: Required device feature not present" << std::endl;
                break;
            case VK_ERROR_TOO_MANY_OBJECTS:
                std::cerr << "[VulkanContext] Error: Too many device objects" << std::endl;
                break;
            case VK_ERROR_DEVICE_LOST:
                std::cerr << "[VulkanContext] Error: Device lost" << std::endl;
                break;
            default:
                std::cerr << "[VulkanContext] Error: Unknown device creation error" << std::endl;
                break;
        }
        
        return Result::InitializationFailed;
    }
    
    std::cout << "[VulkanContext] Logical device created successfully" << std::endl;
    
    // Get queues
    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.computeFamily.value(), 0, &m_computeQueue);
    m_graphicsQueueFamily = indices.graphicsFamily.value();
    m_computeQueueFamily = indices.computeFamily.value();
    
    std::cout << "[VulkanContext] Graphics and compute queues obtained" << std::endl;
    if (indices.presentFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
        m_presentQueueFamily = indices.presentFamily.value();
        std::cout << "[VulkanContext] Present queue obtained" << std::endl;
    } else {
        m_presentQueue = m_graphicsQueue;
        m_presentQueueFamily = m_graphicsQueueFamily;
        std::cout << "[VulkanContext] Present queue not distinct, using graphics queue" << std::endl;
    }
    
    return Result::Success;
}

Result VulkanContext::CreateCommandPools() {
    // Validate Vulkan objects before use
    if (m_device == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Device not initialized in CreateCommandPools" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Physical device not initialized in CreateCommandPools" << std::endl;
        return Result::InitializationFailed;
    }
    
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);
    
    // Validate queue families
    if (!queueFamilyIndices.graphicsFamily.has_value()) {
        std::cerr << "[VulkanContext] Error: No graphics queue family found in CreateCommandPools" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (!queueFamilyIndices.computeFamily.has_value()) {
        std::cerr << "[VulkanContext] Error: No compute queue family found in CreateCommandPools" << std::endl;
        return Result::InitializationFailed;
    }
    
    // Graphics command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to create graphics command pool" << std::endl;
        return Result::InitializationFailed;
    }
    
    // Compute command pool
    poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_computeCommandPool) != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to create compute command pool" << std::endl;
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result VulkanContext::CreateCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    // Graphics command buffers
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    
    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    // Compute command buffers
    allocInfo.commandPool = m_computeCommandPool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_computeCommandBuffers.size());
    
    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result VulkanContext::CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            return Result::InitializationFailed;
        }
    }
    
    return Result::Success;
}

Result VulkanContext::CreateComputePipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding layoutBindings[] = {
        // Input texture
        {
            0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        },
        // Conversion parameters
        {
            1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        },
        // Character set
        {
            2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        },
        // Output buffer
        {
            3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        }
    };
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 4;
    layoutInfo.pBindings = layoutBindings;
    
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_computeDescriptorSetLayout) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
    
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    const char* src = R"(#version 450
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
void main() {}
)";
    auto compileToSpirv = [&](const std::string& source, std::vector<uint32_t>& spirv) -> Result {
        spirv.clear();
        std::filesystem::path exe;
        const char* sdk = std::getenv("VULKAN_SDK");
        if (sdk) {
            std::filesystem::path bin = std::filesystem::path(sdk) / "Bin";
            exe = bin / "glslangValidator.exe";
            if (!std::filesystem::exists(exe)) exe = bin / "glslangValidator";
        }
        if (exe.empty() || !std::filesystem::exists(exe)) {
            std::vector<std::filesystem::path> candidates = {
                std::filesystem::path("C:/VulkanSDK/1.4.328.1/Bin/glslangValidator.exe"),
                std::filesystem::path("C:/VulkanSDK/1.4.312.1/Bin/glslangValidator.exe"),
                std::filesystem::path("C:/VulkanSDK/Bin/glslangValidator.exe")
            };
            for (auto& c : candidates) { if (std::filesystem::exists(c)) { exe = c; break; } }
        }
        if (exe.empty() || !std::filesystem::exists(exe)) {
            NeonGlyph::Logger::LogLine("ShaderCompile glslangValidator not found");
            return Result::ValidationFailed;
        }
        std::filesystem::path tmpDir = std::filesystem::temp_directory_path();
        std::filesystem::path srcPath = tmpDir / "ng_empty.comp";
        std::filesystem::path spvPath = tmpDir / "ng_empty.spv";
        {
            std::ofstream out(srcPath.string(), std::ios::out | std::ios::binary);
            out.write(source.data(), static_cast<std::streamsize>(source.size()));
        }
        std::string cmd = "\"" + exe.string() + "\" -V \"" + srcPath.string() + "\" -o \"" + spvPath.string() + "\"";
        NeonGlyph::Logger::LogLine("ShaderCompile start compute");
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            NeonGlyph::Logger::LogLine(std::string("ShaderCompile failed rc=") + std::to_string(rc));
            return Result::ValidationFailed;
        }
        std::ifstream binIn(spvPath.string(), std::ios::binary);
        if (!binIn.is_open()) {
            return Result::FileNotFound;
        }
        binIn.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(binIn.tellg());
        binIn.seekg(0, std::ios::beg);
        if (size % 4 != 0) {
            return Result::ValidationFailed;
        }
        spirv.resize(size / 4);
        binIn.read(reinterpret_cast<char*>(spirv.data()), size);
        NeonGlyph::Logger::LogLine(std::string("ShaderCompile success sizeBytes=") + std::to_string(size));
        return Result::Success;
    };
    std::vector<uint32_t> code;
    Result cr = compileToSpirv(src, code);
    if (cr != Result::Success) {
        NeonGlyph::Logger::LogLine("PipelineCreate abort due to shader compile failure");
        return cr;
    }
    VkShaderModule module;
    cr = CreateShaderModule(code, module);
    if (cr != Result::Success) {
        NeonGlyph::Logger::LogLine("ShaderModule creation failed");
        return cr;
    }
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = module;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = m_computePipelineLayout;
    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, module, nullptr);
        NeonGlyph::Logger::LogLine("PipelineCreate failed compute");
        return Result::InitializationFailed;
    }
    vkDestroyShaderModule(m_device, module, nullptr);
    NeonGlyph::Logger::LogLine("PipelineCreate success compute");
    return Result::Success;
}

Result VulkanContext::CreateDescriptorPool() {
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 100;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 100;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 100;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = 100;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 200;
    
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result VulkanContext::BeginFrame() {
    // Validate Vulkan objects before use
    if (m_device == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Device not initialized in BeginFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_currentFrame >= MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Invalid frame index in BeginFrame" << std::endl;
        return Result::InvalidArgument;
    }
    
    // Validate vector sizes
    if (m_inFlightFences.size() != MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Fences vector not properly sized in BeginFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_commandBuffers.size() != MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Command buffers vector not properly sized in BeginFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_inFlightFences[m_currentFrame] == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Fence not initialized in BeginFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_commandBuffers[m_currentFrame] == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Command buffer not initialized in BeginFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    // Wait for previous frame
    VkResult waitResult = vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to wait for fence in BeginFrame" << std::endl;
        return Result::Error;
    }
    
    // Reset fence
    VkResult resetResult = vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    if (resetResult != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to reset fence in BeginFrame" << std::endl;
        return Result::Error;
    }
    
    if (m_swapchain != VK_NULL_HANDLE) {
        VkResult acquire = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_currentImageIndex);
        if (acquire != VK_SUCCESS) {
            std::cerr << "[VulkanContext] Error: Failed to acquire swapchain image" << std::endl;
            return Result::Error;
        }
    }
    VkResult cmdResetResult = vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    if (cmdResetResult != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to reset command buffer in BeginFrame" << std::endl;
        return Result::Error;
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to begin command buffer" << std::endl;
        return Result::Error;
    }
    if (m_swapchain != VK_NULL_HANDLE) {
        VkImage image = m_swapchainImages[m_currentImageIndex];
        VkImageMemoryBarrier barrier1{};
        barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier1.srcAccessMask = 0;
        barrier1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.image = image;
        barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier1.subresourceRange.baseMipLevel = 0;
        barrier1.subresourceRange.levelCount = 1;
        barrier1.subresourceRange.baseArrayLayer = 0;
        barrier1.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(m_commandBuffers[m_currentFrame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier1);
        VkClearColorValue clear{};
        uint32_t rgba = m_config.render.startupBgColor;
        float r = ((rgba >> 16) & 0xFFu) / 255.0f;
        float g = ((rgba >> 8) & 0xFFu) / 255.0f;
        float b = (rgba & 0xFFu) / 255.0f;
        float a = ((rgba >> 24) & 0xFFu) / 255.0f;
        clear.float32[0] = r; clear.float32[1] = g; clear.float32[2] = b; clear.float32[3] = a;
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        vkCmdClearColorImage(m_commandBuffers[m_currentFrame], image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
        if (m_hasPendingFrame && m_stagingBuffer != VK_NULL_HANDLE) {
            VkBufferImageCopy copy{};
            copy.bufferOffset = 0;
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel = 0;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageOffset = {0, 0, 0};
            copy.imageExtent = { m_swapchainExtent.width, m_swapchainExtent.height, 1 };
            vkCmdCopyBufferToImage(m_commandBuffers[m_currentFrame], m_stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        }
        VkImageMemoryBarrier barrier2{};
        barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier2.dstAccessMask = 0;
        barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.image = image;
        barrier2.subresourceRange = range;
        vkCmdPipelineBarrier(m_commandBuffers[m_currentFrame], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
    }
    if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to end command buffer" << std::endl;
        return Result::Error;
    }
    
    return Result::Success;
}

Result VulkanContext::EndFrame() {
    // Validate Vulkan objects before use
    if (m_device == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Device not initialized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_graphicsQueue == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Graphics queue not initialized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_currentFrame >= MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Invalid frame index in EndFrame" << std::endl;
        return Result::InvalidArgument;
    }
    
    // Validate vector sizes
    if (m_commandBuffers.size() != MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Command buffers vector not properly sized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_inFlightFences.size() != MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Fences vector not properly sized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_renderFinishedSemaphores.size() != MAX_FRAMES_IN_FLIGHT) {
        std::cerr << "[VulkanContext] Error: Render finished semaphores vector not properly sized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_commandBuffers[m_currentFrame] == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Command buffer not initialized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_inFlightFences[m_currentFrame] == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Fence not initialized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_renderFinishedSemaphores[m_currentFrame] == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Render finished semaphore not initialized in EndFrame" << std::endl;
        return Result::InitializationFailed;
    }
    
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = m_swapchain != VK_NULL_HANDLE ? 1u : 0u;
    submitInfo.pWaitSemaphores = m_swapchain != VK_NULL_HANDLE ? &m_imageAvailableSemaphores[m_currentFrame] : nullptr;
    submitInfo.pWaitDstStageMask = m_swapchain != VK_NULL_HANDLE ? waitStages : nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    submitInfo.signalSemaphoreCount = m_swapchain != VK_NULL_HANDLE ? 1u : 0u;
    submitInfo.pSignalSemaphores = m_swapchain != VK_NULL_HANDLE ? &m_renderFinishedSemaphores[m_currentFrame] : nullptr;
    VkResult submitResult = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    if (submitResult != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to submit command buffer in EndFrame" << std::endl;
        return Result::Error;
    }
    if (m_swapchain != VK_NULL_HANDLE) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_currentImageIndex;
        presentInfo.pResults = nullptr;
        VkResult pres = vkQueuePresentKHR(m_presentQueue, &presentInfo);
        if (pres != VK_SUCCESS) {
            std::cerr << "[VulkanContext] Error: Failed to present swapchain image" << std::endl;
            return Result::Error;
        }
        m_hasPendingFrame = false;
    }
    // Advance frame
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
    return Result::Success;
}

Result VulkanContext::SubmitComputeWork(VkCommandBuffer commandBuffer) {
    // Validate Vulkan objects before use
    if (m_device == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Device not initialized in SubmitComputeWork" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (m_computeQueue == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Compute queue not initialized in SubmitComputeWork" << std::endl;
        return Result::InitializationFailed;
    }
    
    if (commandBuffer == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Invalid command buffer in SubmitComputeWork" << std::endl;
        return Result::InvalidArgument;
    }
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    VkResult submitResult = vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (submitResult != VK_SUCCESS) {
        std::cerr << "[VulkanContext] Error: Failed to submit compute work" << std::endl;
        return Result::Error;
    }
    
    return Result::Success;
}

Result VulkanContext::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                                  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        return Result::OutOfMemory;
    }
    
    vkBindBufferMemory(m_device, buffer, memory, 0);
    
    return Result::Success;
}

Result VulkanContext::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                   VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                                   VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        return Result::OutOfMemory;
    }
    
    vkBindImageMemory(m_device, image, memory, 0);
    
    return Result::Success;
}

uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    std::cerr << "[VulkanContext] Error: Failed to find suitable memory type for typeFilter=" << typeFilter 
              << " properties=" << properties << std::endl;
    return 0; // Return default memory type, caller should handle this case
}

VkFormat VulkanContext::FindSupportedFormat(const std::vector<VkFormat>& candidates, 
                                           VkImageTiling tiling, VkFormatFeatureFlags features) {
    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Physical device not initialized in FindSupportedFormat" << std::endl;
        return VK_FORMAT_UNDEFINED;
    }
    
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    std::cerr << "[VulkanContext] Error: Failed to find supported format" << std::endl;
    return VK_FORMAT_UNDEFINED; // Return undefined format, caller should handle this case
}

std::vector<const char*> VulkanContext::GetRequiredExtensions() {
    std::vector<const char*> extensions;
    
    #ifdef NEONGLYPH_DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif
    
    // Platform-specific extensions
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    
    return extensions;
}

bool VulkanContext::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = FindQueueFamilies(device);
    
    bool extensionsSupported = false;
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    std::set<std::string> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    extensionsSupported = requiredExtensions.empty();
    
    return indices.IsComplete() && extensionsSupported;
}

QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }
        
        // Check if this queue family supports presentation to our surface
        if (m_surface != VK_NULL_HANDLE) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }
        
        if (indices.IsComplete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

Result VulkanContext::AttachSurface(VkSurfaceKHR surface) {
    if (m_surface != VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Warning: Surface already attached, destroying previous surface" << std::endl;
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
    
    m_surface = surface;
    
    if (m_surface == VK_NULL_HANDLE) {
        std::cerr << "[VulkanContext] Error: Attempted to attach null surface" << std::endl;
        return Result::InvalidArgument;
    }
    
    std::cout << "[VulkanContext] Surface attached successfully" << std::endl;
    return Result::Success;
}

Result VulkanContext::CreateSwapchain() {
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    SwapchainSupportDetails support = QuerySwapchainSupport(m_physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.presentModes);
    VkExtent2D extent = ChooseSwapExtent(support.capabilities);
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) imageCount = support.capabilities.maxImageCount;
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) return Result::InitializationFailed;
    m_swapchainFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
    NeonGlyph::Logger::LogLine(std::string("Swapchain created format=") + std::to_string(static_cast<int>(m_swapchainFormat)) + std::string(" extent=") + std::to_string(m_swapchainExtent.width) + std::string("x") + std::to_string(m_swapchainExtent.height));
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr);
    m_swapchainImages.resize(count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, m_swapchainImages.data());
    return Result::Success;
}

Result VulkanContext::CreateImageViews() {
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_swapchainFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) return Result::InitializationFailed;
    }
    return Result::Success;
}

Result VulkanContext::CreateFrameResources() {
    if (m_swapchainExtent.width == 0 || m_swapchainExtent.height == 0) return Result::InitializationFailed;
    VkDeviceSize size = static_cast<VkDeviceSize>(m_swapchainExtent.width) * m_swapchainExtent.height * 4;
    m_stagingSize = size;
    VkBufferCreateInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = size;
    bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(m_device, &bi, nullptr, &m_stagingBuffer) != VK_SUCCESS) return Result::InitializationFailed;
    VkMemoryRequirements mr{};
    vkGetBufferMemoryRequirements(m_device, m_stagingBuffer, &mr);
    VkMemoryAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = mr.size;
    ai.memoryTypeIndex = FindMemoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(m_device, &ai, nullptr, &m_stagingMemory) != VK_SUCCESS) return Result::OutOfMemory;
    vkBindBufferMemory(m_device, m_stagingBuffer, m_stagingMemory, 0);
    return Result::Success;
}

Result VulkanContext::UpdateFramePixels(const void* data, size_t size) {
    if (m_stagingBuffer == VK_NULL_HANDLE || m_stagingMemory == VK_NULL_HANDLE) return Result::InitializationFailed;
    if (size > static_cast<size_t>(m_stagingSize)) return Result::InvalidArgument;
    void* dst = nullptr;
    if (vkMapMemory(m_device, m_stagingMemory, 0, m_stagingSize, 0, &dst) != VK_SUCCESS) return Result::Error;
    std::memcpy(dst, data, size);
    vkUnmapMemory(m_device, m_stagingMemory);
    m_hasPendingFrame = true;
    return Result::Success;
}

SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkPhysicalDevice device) {
    SwapchainSupportDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    return details;
}

VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& f : availableFormats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
    }
    return availableFormats.empty() ? VkSurfaceFormatKHR{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } : availableFormats[0];
}

VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& m : availablePresentModes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) return capabilities.currentExtent;
    VkExtent2D extent{};
    extent.width = 1920;
    extent.height = 1080;
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}

} // namespace NeonGlyph