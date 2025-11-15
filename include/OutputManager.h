#pragma once

#include "NeonGlyph.h"
#include <vulkan/vulkan.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

namespace NeonGlyph {

// Spout 2.0 integration for real-time texture sharing
class SpoutSender {
public:
    SpoutSender();
    ~SpoutSender();

    Result Initialize(const std::string& senderName, uint32_t width, uint32_t height);
    void Shutdown();
    
    // Send texture via Spout
    Result SendTexture(VkImage vulkanImage, uint32_t width, uint32_t height);
    Result SendTexture(ID3D11Texture2D* d3d11Texture);
    Result SendRGBAData(const uint8_t* rgbaData, uint32_t width, uint32_t height, uint32_t rowPitch);
    
    // Sender management
    bool IsConnected() const { return m_isConnected; }
    std::string GetSenderName() const { return m_senderName; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    
    // Performance monitoring
    uint32_t GetFrameCount() const { return m_frameCount; }
    float GetFPS() const { return m_fps; }

private:
    std::string m_senderName;
    uint32_t m_width;
    uint32_t m_height;
    bool m_isConnected;
    uint32_t m_frameCount;
    float m_fps;
    
    // D3D11 resources for Spout
    ID3D11Device* m_d3d11Device;
    ID3D11DeviceContext* m_d3d11Context;
    ID3D11Texture2D* m_sharedTexture;
    HANDLE m_sharedHandle;
    
    // Vulkan to D3D11 interop
    VkDevice m_vulkanDevice;
    VkPhysicalDevice m_vulkanPhysicalDevice;
    VkQueue m_vulkanQueue;
    
    // Timing
    std::chrono::steady_clock::time_point m_lastFrameTime;
    
    // Private methods
    Result CreateD3D11Device();
    Result CreateSharedTexture();
    Result CopyVulkanToD3D11(VkImage vulkanImage, uint32_t width, uint32_t height);
    Result UpdatePerformanceMetrics();
};

// NDI (Network Device Interface) integration
class NDISender {
public:
    NDISender();
    ~NDISender();

    Result Initialize(const std::string& senderName, uint32_t width, uint32_t height);
    void Shutdown();
    
    // Send frame via NDI
    Result SendFrame(VkImage vulkanImage, uint32_t width, uint32_t height);
    Result SendFrame(const uint8_t* rgbaData, uint32_t width, uint32_t height);
    
    // Sender management
    bool IsConnected() const { return m_isConnected; }
    std::string GetSenderName() const { return m_senderName; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    
    // Network statistics
    uint32_t GetBitrate() const { return m_bitrate; }
    uint32_t GetDroppedFrames() const { return m_droppedFrames; }

private:
    std::string m_senderName;
    uint32_t m_width;
    uint32_t m_height;
    bool m_isConnected;
    uint32_t m_bitrate;
    uint32_t m_droppedFrames;
    uint32_t m_frameCount;
    
    // NDI resources (forward declarations)
    void* m_ndiSender; // NDIlib_send_instance_t
    void* m_ndiVideoFrame; // NDIlib_video_frame_v2_t
    
    // Vulkan to NDI conversion
    VkDevice m_vulkanDevice;
    VkQueue m_vulkanQueue;
    
    // Frame buffer for conversion
    std::vector<uint8_t> m_frameBuffer;
    
    // Private methods
    Result CreateNDISender();
    Result CreateNDIVideoFrame();
    Result ConvertVulkanToNDI(VkImage vulkanImage, uint32_t width, uint32_t height);
    Result UpdateNetworkStatistics();
};

// Output manager that handles both Spout and NDI
class OutputManager {
public:
    OutputManager();
    ~OutputManager();

    Result Initialize(const Config& config);
    void Shutdown();
    
    // Send frame to all enabled outputs
    Result SendFrame(VkImage vulkanImage, uint32_t width, uint32_t height);
    Result SendFrameCPU(const uint8_t* rgbaData, uint32_t width, uint32_t height);
    
    // Individual output control
    Result EnableSpout(const std::string& senderName);
    Result DisableSpout();
    Result EnableNDI(const std::string& senderName);
    Result DisableNDI();
    
    // Status monitoring
    bool IsSpoutEnabled() const { return m_spoutEnabled; }
    bool IsNDIEnabled() const { return m_ndiEnabled; }
    bool IsSpoutConnected() const { return m_spoutSender && m_spoutSender->IsConnected(); }
    bool IsNDIConnected() const { return m_ndiSender && m_ndiSender->IsConnected(); }
    
    // Performance monitoring
    void UpdatePerformanceMetrics();
    std::string GetStatusString() const;

private:
    Config m_config;
    
    // Output senders
    std::unique_ptr<SpoutSender> m_spoutSender;
    std::unique_ptr<NDISender> m_ndiSender;
    
    // Enable flags
    bool m_spoutEnabled;
    bool m_ndiEnabled;
    
    // Frame dimensions
    uint32_t m_width;
    uint32_t m_height;
    
    // Performance metrics
    uint32_t m_totalFramesSent;
    uint32_t m_spoutFramesSent;
    uint32_t m_ndiFramesSent;
    float m_averageSendTime;
    
    // Private methods
    Result InitializeSpout();
    Result InitializeNDI();
    Result CreateSenders();
    Result ConfigureOutputs();
};

} // namespace NeonGlyph