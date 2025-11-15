#include "OutputManager.h"
#include <iostream>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#endif

namespace NeonGlyph {

// Spout 2.0 implementation
SpoutSender::SpoutSender() 
    : m_d3d11Device(nullptr)
    , m_d3d11Context(nullptr)
    , m_sharedTexture(nullptr)
    , m_sharedHandle(nullptr)
    , m_isConnected(false)
    , m_frameCount(0)
    , m_fps(0.0f)
    , m_width(1920)
    , m_height(1080) {
}

SpoutSender::~SpoutSender() {
    Shutdown();
}

Result SpoutSender::Initialize(const std::string& senderName, uint32_t width, uint32_t height) {
    m_senderName = senderName;
    m_width = width;
    m_height = height;
    
    Result result = CreateD3D11Device();
    if (result != Result::Success) {
        return result;
    }
    
    result = CreateSharedTexture();
    if (result != Result::Success) {
        return result;
    }
    
    m_isConnected = true;
    m_lastFrameTime = std::chrono::steady_clock::now();
    
    std::cout << "Spout sender initialized: " << senderName 
              << " (" << width << "x" << height << ")" << std::endl;
    
    return Result::Success;
}

void SpoutSender::Shutdown() {
    if (m_sharedTexture) {
        m_sharedTexture->Release();
        m_sharedTexture = nullptr;
    }
    
    if (m_d3d11Context) {
        m_d3d11Context->Release();
        m_d3d11Context = nullptr;
    }
    
    if (m_d3d11Device) {
        m_d3d11Device->Release();
        m_d3d11Device = nullptr;
    }
    
    m_isConnected = false;
    
    std::cout << "Spout sender shutdown: " << m_senderName << std::endl;
}

Result SpoutSender::SendTexture(VkImage vulkanImage, uint32_t width, uint32_t height) {
    if (!m_isConnected) {
        return Result::Error;
    }
    
    // Convert Vulkan image to D3D11 texture
    Result result = CopyVulkanToD3D11(vulkanImage, width, height);
    if (result != Result::Success) {
        return result;
    }
    
    // Update performance metrics
    UpdatePerformanceMetrics();
    
    return Result::Success;
}

Result SpoutSender::SendTexture(ID3D11Texture2D* d3d11Texture) {
    if (!m_isConnected || !d3d11Texture) {
        return Result::Error;
    }
    
    // Copy texture to shared texture
    m_d3d11Context->CopyResource(m_sharedTexture, d3d11Texture);
    
    // Update performance metrics
    UpdatePerformanceMetrics();
    
    return Result::Success;
}

Result SpoutSender::SendRGBAData(const uint8_t* rgbaData, uint32_t width, uint32_t height, uint32_t rowPitch) {
    if (!m_isConnected || !rgbaData) {
        return Result::Error;
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = m_d3d11Context->Map(m_sharedTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) {
        return Result::Error;
    }
    uint8_t* dst = static_cast<uint8_t*>(mapped.pData);
    uint32_t dstPitch = mapped.RowPitch;
    const uint8_t* src = rgbaData;
    for (uint32_t y = 0; y < height; y++) {
        std::memcpy(dst + y * dstPitch, src + y * rowPitch, std::min(dstPitch, rowPitch));
    }
    m_d3d11Context->Unmap(m_sharedTexture, 0);
    UpdatePerformanceMetrics();
    return Result::Success;
}

Result SpoutSender::CreateD3D11Device() {
    // Create D3D11 device for Spout interop
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // Default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,                    // No software rasterizer
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_d3d11Device,
        &featureLevel,
        &m_d3d11Context
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create D3D11 device for Spout: " << hr << std::endl;
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result SpoutSender::CreateSharedTexture() {
    // Create shared texture for Spout
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = m_width;
    desc.Height = m_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    
    HRESULT hr = m_d3d11Device->CreateTexture2D(&desc, nullptr, &m_sharedTexture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create shared texture for Spout: " << hr << std::endl;
        return Result::InitializationFailed;
    }
    
    // Get shared handle
    IDXGIResource* dxgiResource = nullptr;
    hr = m_sharedTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);
    if (FAILED(hr)) {
        std::cerr << "Failed to query DXGI resource: " << hr << std::endl;
        return Result::InitializationFailed;
    }
    
    hr = dxgiResource->GetSharedHandle(&m_sharedHandle);
    dxgiResource->Release();
    
    if (FAILED(hr)) {
        std::cerr << "Failed to get shared handle: " << hr << std::endl;
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result SpoutSender::CopyVulkanToD3D11(VkImage vulkanImage, uint32_t width, uint32_t height) {
    // This is a placeholder implementation
    // In a real implementation, you would:
    // 1. Create a Vulkan buffer from the image
    // 2. Map the buffer to CPU memory
    // 3. Create a D3D11 staging texture
    // 4. Copy the data to the staging texture
    // 5. Copy from staging to shared texture
    
    // For now, just update the shared texture with a test pattern
    if (width != m_width || height != m_height) {
        // Recreate shared texture with new dimensions
        m_width = width;
        m_height = height;
        
        if (m_sharedTexture) {
            m_sharedTexture->Release();
            m_sharedTexture = nullptr;
        }
        
        Result result = CreateSharedTexture();
        if (result != Result::Success) {
            return result;
        }
    }
    
    // Fill with test pattern (ASCII gradient)
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = m_d3d11Context->Map(m_sharedTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        uint8_t* data = static_cast<uint8_t*>(mappedResource.pData);
        
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                uint32_t index = y * mappedResource.RowPitch + x * 4;
                uint8_t value = static_cast<uint8_t>((x + y) * 255 / (width + height));
                
                data[index + 0] = value;     // B
                data[index + 1] = value;     // G
                data[index + 2] = value;     // R
                data[index + 3] = 255;       // A
            }
        }
        
        m_d3d11Context->Unmap(m_sharedTexture, 0);
    }
    
    return Result::Success;
}

Result SpoutSender::UpdatePerformanceMetrics() {
    m_frameCount++;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFrameTime);
    
    if (elapsed.count() >= 1000) { // Update every second
        m_fps = m_frameCount * 1000.0f / elapsed.count();
        m_frameCount = 0;
        m_lastFrameTime = now;
    }
    
    return Result::Success;
}

// NDI implementation (simplified)
NDISender::NDISender() 
    : m_ndiSender(nullptr)
    , m_ndiVideoFrame(nullptr)
    , m_isConnected(false)
    , m_bitrate(0)
    , m_droppedFrames(0)
    , m_frameCount(0)
    , m_width(1920)
    , m_height(1080) {
}

NDISender::~NDISender() {
    Shutdown();
}

Result NDISender::Initialize(const std::string& senderName, uint32_t width, uint32_t height) {
    m_senderName = senderName;
    m_width = width;
    m_height = height;
    
    Result result = CreateNDISender();
    if (result != Result::Success) {
        return result;
    }
    
    result = CreateNDIVideoFrame();
    if (result != Result::Success) {
        return result;
    }
    
    m_isConnected = true;
    
    std::cout << "NDI sender initialized: " << senderName 
              << " (" << width << "x" << height << ")" << std::endl;
    
    return Result::Success;
}

void NDISender::Shutdown() {
    // Cleanup NDI resources
    if (m_ndiVideoFrame) {
        // Free video frame memory
        m_ndiVideoFrame = nullptr;
    }
    
    if (m_ndiSender) {
        // Destroy NDI sender
        m_ndiSender = nullptr;
    }
    
    m_isConnected = false;
    
    std::cout << "NDI sender shutdown: " << m_senderName << std::endl;
}

Result NDISender::SendFrame(VkImage vulkanImage, uint32_t width, uint32_t height) {
    if (!m_isConnected) {
        return Result::Error;
    }
    
    // Convert Vulkan image to NDI format
    Result result = ConvertVulkanToNDI(vulkanImage, width, height);
    if (result != Result::Success) {
        return result;
    }
    
    // Send frame via NDI
    if (m_ndiSender && m_ndiVideoFrame) {
        // Send frame (placeholder implementation)
        m_frameCount++;
    }
    
    return Result::Success;
}

Result NDISender::SendFrame(const uint8_t* rgbaData, uint32_t width, uint32_t height) {
    if (!m_isConnected || !rgbaData) {
        return Result::Error;
    }
    
    // Copy data to frame buffer
    size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    if (m_frameBuffer.size() != dataSize) {
        m_frameBuffer.resize(dataSize);
    }
    
    std::memcpy(m_frameBuffer.data(), rgbaData, dataSize);
    
    // Send frame via NDI
    if (m_ndiSender && m_ndiVideoFrame) {
        // Send frame (placeholder implementation)
        m_frameCount++;
    }
    
    return Result::Success;
}

Result NDISender::CreateNDISender() {
    // Create NDI sender (placeholder implementation)
    // In a real implementation, you would use the NDI SDK
    
    std::cout << "NDI sender created: " << m_senderName << std::endl;
    return Result::Success;
}

Result NDISender::CreateNDIVideoFrame() {
    // Create NDI video frame (placeholder implementation)
    m_frameBuffer.resize(m_width * m_height * 4);
    
    std::cout << "NDI video frame created: " << m_width << "x" << m_height << std::endl;
    return Result::Success;
}

Result NDISender::ConvertVulkanToNDI(VkImage vulkanImage, uint32_t width, uint32_t height) {
    // Convert Vulkan image to NDI format (placeholder implementation)
    // In a real implementation, you would:
    // 1. Create a Vulkan buffer from the image
    // 2. Map the buffer to CPU memory
    // 3. Convert the format if necessary
    // 4. Copy to NDI frame buffer
    
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        m_frameBuffer.resize(width * height * 4);
    }
    
    // Fill with test pattern
    for (uint32_t i = 0; i < width * height * 4; i += 4) {
        m_frameBuffer[i + 0] = 128; // B
        m_frameBuffer[i + 1] = 128; // G
        m_frameBuffer[i + 2] = 128; // R
        m_frameBuffer[i + 3] = 255; // A
    }
    
    return Result::Success;
}

Result NDISender::UpdateNetworkStatistics() {
    // Update network statistics (placeholder implementation)
    m_bitrate = 10000000; // 10 Mbps placeholder
    m_droppedFrames = 0;
    
    return Result::Success;
}

// Output Manager Implementation
OutputManager::OutputManager() 
    : m_spoutEnabled(false)
    , m_ndiEnabled(false)
    , m_width(1920)
    , m_height(1080)
    , m_totalFramesSent(0)
    , m_spoutFramesSent(0)
    , m_ndiFramesSent(0)
    , m_averageSendTime(0.0f) {
}

OutputManager::~OutputManager() {
    Shutdown();
}

Result OutputManager::Initialize(const Config& config) {
    m_config = config;
    
    Result result = ConfigureOutputs();
    if (result != Result::Success) {
        return result;
    }
    
    return Result::Success;
}

void OutputManager::Shutdown() {
    DisableSpout();
    DisableNDI();
}

Result OutputManager::SendFrame(VkImage vulkanImage, uint32_t width, uint32_t height) {
    auto start = std::chrono::high_resolution_clock::now();
    
    Result result = Result::Success;
    
    // Send via Spout
    if (m_spoutEnabled && m_spoutSender) {
        Result spoutResult = m_spoutSender->SendTexture(vulkanImage, width, height);
        if (spoutResult == Result::Success) {
            m_spoutFramesSent++;
        }
        result = spoutResult;
    }
    
    // Send via NDI
    if (m_ndiEnabled && m_ndiSender) {
        Result ndiResult = m_ndiSender->SendFrame(vulkanImage, width, height);
        if (ndiResult == Result::Success) {
            m_ndiFramesSent++;
        }
        result = ndiResult;
    }
    
    // Update performance metrics
    auto end = std::chrono::high_resolution_clock::now();
    float sendTime = std::chrono::duration<float, std::milli>(end - start).count();
    
    m_averageSendTime = (m_averageSendTime * m_totalFramesSent + sendTime) / (m_totalFramesSent + 1);
    m_totalFramesSent++;
    
    return result;
}

Result OutputManager::SendFrameCPU(const uint8_t* rgbaData, uint32_t width, uint32_t height) {
    Result result = Result::Success;
    if (m_spoutEnabled && m_spoutSender) {
        Result sp = m_spoutSender->SendRGBAData(rgbaData, width, height, width * 4u);
        if (sp == Result::Success) m_spoutFramesSent++;
        result = sp;
    }
    if (m_ndiEnabled && m_ndiSender) {
        Result nd = m_ndiSender->SendFrame(rgbaData, width, height);
        if (nd == Result::Success) m_ndiFramesSent++;
        result = nd;
    }
    m_totalFramesSent++;
    return result;
}

Result OutputManager::EnableSpout(const std::string& senderName) {
    if (m_spoutEnabled) {
        return Result::Success;
    }
    
    m_spoutSender = std::make_unique<SpoutSender>();
    Result result = m_spoutSender->Initialize(senderName, m_width, m_height);
    if (result != Result::Success) {
        m_spoutSender.reset();
        return result;
    }
    
    m_spoutEnabled = true;
    std::cout << "Spout output enabled: " << senderName << std::endl;
    
    return Result::Success;
}

Result OutputManager::DisableSpout() {
    if (!m_spoutEnabled) {
        return Result::Success;
    }
    
    if (m_spoutSender) {
        m_spoutSender->Shutdown();
        m_spoutSender.reset();
    }
    
    m_spoutEnabled = false;
    std::cout << "Spout output disabled" << std::endl;
    
    return Result::Success;
}

Result OutputManager::EnableNDI(const std::string& senderName) {
    if (m_ndiEnabled) {
        return Result::Success;
    }
    
    m_ndiSender = std::make_unique<NDISender>();
    Result result = m_ndiSender->Initialize(senderName, m_width, m_height);
    if (result != Result::Success) {
        m_ndiSender.reset();
        return result;
    }
    
    m_ndiEnabled = true;
    std::cout << "NDI output enabled: " << senderName << std::endl;
    
    return Result::Success;
}

Result OutputManager::DisableNDI() {
    if (!m_ndiEnabled) {
        return Result::Success;
    }
    
    if (m_ndiSender) {
        m_ndiSender->Shutdown();
        m_ndiSender.reset();
    }
    
    m_ndiEnabled = false;
    std::cout << "NDI output disabled" << std::endl;
    
    return Result::Success;
}

Result OutputManager::ConfigureOutputs() {
    // Configure based on config
    if (m_config.output.spoutEnabled) {
        Result result = EnableSpout(m_config.output.spoutName);
        if (result != Result::Success) {
            std::cerr << "Failed to enable Spout output" << std::endl;
        }
    }
    
    if (m_config.output.ndiEnabled) {
        Result result = EnableNDI(m_config.output.ndiName);
        if (result != Result::Success) {
            std::cerr << "Failed to enable NDI output" << std::endl;
        }
    }
    
    return Result::Success;
}

void OutputManager::UpdatePerformanceMetrics() {
    // Update performance metrics from individual senders
    if (m_spoutSender) {
        // Update Spout metrics
    }
    
    if (m_ndiSender) {
        // Update NDI metrics
    }
}

std::string OutputManager::GetStatusString() const {
    std::stringstream status;
    
    status << "Output Status:\n";
    status << "  Spout: " << (m_spoutEnabled ? "Enabled" : "Disabled");
    if (m_spoutEnabled && m_spoutSender) {
        status << " (Connected: " << (m_spoutSender->IsConnected() ? "Yes" : "No") << ")";
    }
    status << "\n";
    
    status << "  NDI: " << (m_ndiEnabled ? "Enabled" : "Disabled");
    if (m_ndiEnabled && m_ndiSender) {
        status << " (Connected: " << (m_ndiSender->IsConnected() ? "Yes" : "No") << ")";
    }
    status << "\n";
    
    status << "  Total frames sent: " << m_totalFramesSent << "\n";
    status << "  Spout frames: " << m_spoutFramesSent << "\n";
    status << "  NDI frames: " << m_ndiFramesSent << "\n";
    status << "  Average send time: " << m_averageSendTime << "ms\n";
    
    return status.str();
}

} // namespace NeonGlyph
