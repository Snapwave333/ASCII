#include "ASCIIConverter.h"
#include "VulkanContext.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <immintrin.h>
#if NEONGLYPH_HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H
#endif

namespace NeonGlyph {

// ASCII character density lookup table (pre-calculated)
const std::unordered_map<char, float32> DEFAULT_DENSITY_MAP = {
    {'@', 0.95f}, {'#', 0.85f}, {'%', 0.75f}, {'*', 0.65f}, {'+', 0.55f}, 
    {'=', 0.45f}, {':', 0.35f}, {'-', 0.25f}, {'.', 0.15f}, {' ', 0.05f}
};

ASCIIConverter::ASCIIConverter() 
    : m_vulkanContext(nullptr)
    , m_fontAtlas(VK_NULL_HANDLE)
    , m_fontAtlasMemory(VK_NULL_HANDLE)
    , m_fontAtlasView(VK_NULL_HANDLE)
    , m_fontAtlasSampler(VK_NULL_HANDLE)
    , m_conversionBuffer(VK_NULL_HANDLE)
    , m_conversionBufferMemory(VK_NULL_HANDLE)
    , m_outputBuffer(VK_NULL_HANDLE)
    , m_outputBufferMemory(VK_NULL_HANDLE)
    , m_conversionPipeline(VK_NULL_HANDLE)
    , m_conversionPipelineLayout(VK_NULL_HANDLE)
    , m_conversionDescriptorSetLayout(VK_NULL_HANDLE)
    , m_conversionDescriptorSet(VK_NULL_HANDLE)
    , m_threadCount(std::thread::hardware_concurrency())
    , m_useSIMD(true)
    , m_antialiasing(true)
    , m_subpixelRendering(false)
    , m_contrast(1.0f)
    , m_brightness(0.0f) {
    
    m_charset = "@%#*+=-:. ";
    BuildDensityLookupTable();
}

ASCIIConverter::~ASCIIConverter() {
    Shutdown();
}

Result ASCIIConverter::Initialize(VulkanContext* vulkanContext, const Config& config) {
    m_vulkanContext = vulkanContext;
    m_config = config;
    
    // Update configuration
    m_charset = config.ascii.charset;
    m_brightness = config.ascii.brightness;
    m_contrast = config.ascii.contrast;
    
    // Build density lookup table
    BuildDensityLookupTable();
    
    // Create GPU resources
    std::cout << "[ASCIIConverter] Creating GPU resources..." << std::endl;
    Result result = CreateGPUResources();
    if (result != Result::Success) {
        std::cerr << "[ASCIIConverter] Failed to create GPU resources" << std::endl;
        return result;
    }
    std::cout << "[ASCIIConverter] GPU resources created successfully" << std::endl;
    
    // Generate font atlas
    std::cout << "[ASCIIConverter] Generating font atlas..." << std::endl;
    result = GenerateFontAtlas(config.ascii.fontSize, "Consolas");
    if (result != Result::Success) {
        std::cerr << "[ASCIIConverter] Failed to generate font atlas" << std::endl;
        return result;
    }
    std::cout << "[ASCIIConverter] Font atlas generated successfully" << std::endl;
    
    // Create compute pipeline
    std::cout << "[ASCIIConverter] Creating compute pipeline..." << std::endl;
    result = CreateComputePipeline();
    if (result != Result::Success) {
        std::cerr << "[ASCIIConverter] Failed to create compute pipeline" << std::endl;
        m_computeAvailable = false;
    } else {
        m_computeAvailable = true;
    }
    std::cout << "[ASCIIConverter] Compute pipeline created successfully" << std::endl;
    
    return Result::Success;
}

void ASCIIConverter::Shutdown() {
    if (!m_vulkanContext) return;
    
    VkDevice device = m_vulkanContext->GetDevice();
    
    // Cleanup GPU resources
    if (m_fontAtlasSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_fontAtlasSampler, nullptr);
        m_fontAtlasSampler = VK_NULL_HANDLE;
    }
    
    if (m_fontAtlasView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_fontAtlasView, nullptr);
        m_fontAtlasView = VK_NULL_HANDLE;
    }
    
    if (m_fontAtlas != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_fontAtlas, nullptr);
        m_fontAtlas = VK_NULL_HANDLE;
    }
    
    if (m_fontAtlasMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_fontAtlasMemory, nullptr);
        m_fontAtlasMemory = VK_NULL_HANDLE;
    }
    
    if (m_conversionBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_conversionBuffer, nullptr);
        m_conversionBuffer = VK_NULL_HANDLE;
    }
    
    if (m_conversionBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_conversionBufferMemory, nullptr);
        m_conversionBufferMemory = VK_NULL_HANDLE;
    }
    
    if (m_outputBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_outputBuffer, nullptr);
        m_outputBuffer = VK_NULL_HANDLE;
    }
    
    if (m_outputBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_outputBufferMemory, nullptr);
        m_outputBufferMemory = VK_NULL_HANDLE;
    }
    
    if (m_conversionPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_conversionPipeline, nullptr);
        m_conversionPipeline = VK_NULL_HANDLE;
    }
    
    if (m_conversionPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_conversionPipelineLayout, nullptr);
        m_conversionPipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_conversionDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_conversionDescriptorSetLayout, nullptr);
        m_conversionDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

void ASCIIConverter::SetCharset(const std::string& charset) {
    m_charset = charset;
    BuildDensityLookupTable();
}

void ASCIIConverter::SetDensityMapping(const std::vector<float32>& densityMap) {
    m_densityMap = densityMap;
    BuildDensityLookupTable();
}

void ASCIIConverter::SetColorPalette(const ColorPalette& palette) {
    m_colorPalette = palette;
}

Result ASCIIConverter::ConvertFrame(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                                   ASCIIMapping* outputBuffer, uint32_t bufferSize) {
    if (!inputTexture || !outputBuffer || bufferSize == 0) {
        return Result::InvalidArgument;
    }
    
    uint32_t requiredSize = width * height;
    if (bufferSize < requiredSize) {
        return Result::InvalidArgument;
    }
    
    if (m_useSIMD) {
        return ConvertSIMD(inputTexture, width, height, outputBuffer, bufferSize);
    } else {
        return ConvertCPU(inputTexture, width, height, outputBuffer, bufferSize);
    }
}

Result ASCIIConverter::ConvertFrameGPU(VkImage inputImage, uint32_t width, uint32_t height,
                                      VkBuffer outputBuffer, uint32_t& outputSize) {
    return ConvertGPU(inputImage, width, height, outputBuffer, outputSize);
}

Result ASCIIConverter::GenerateFontAtlas(uint32_t fontSize, const std::string& fontName) {
    return CreateFontAtlas(fontSize, fontName);
}

float32 ASCIIConverter::CalculateCharacterDensity(char character) const {
    auto it = m_densityLookup.find(character);
    return it != m_densityLookup.end() ? it->second : 0.5f;
}

void ASCIIConverter::BuildDensityLookupTable() {
    m_densityLookup.clear();
    
    // Use default density map if none provided
    if (m_densityMap.empty()) {
        for (char c : m_charset) {
            auto it = DEFAULT_DENSITY_MAP.find(c);
            if (it != DEFAULT_DENSITY_MAP.end()) {
                m_densityLookup[c] = it->second;
            } else {
                // Estimate density based on character complexity
                m_densityLookup[c] = EstimateCharacterDensity(c);
            }
        }
    } else {
        // Use provided density mapping
        for (size_t i = 0; i < m_charset.size() && i < m_densityMap.size(); i++) {
            m_densityLookup[m_charset[i]] = m_densityMap[i];
        }
    }
    
    // Sort characters by density for binary search
    m_sortedCharacters.clear();
    m_sortedDensities.clear();
    
    std::vector<std::pair<char, float32>> sortedChars;
    for (const auto& pair : m_densityLookup) {
        sortedChars.push_back(pair);
    }
    
    std::sort(sortedChars.begin(), sortedChars.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    for (const auto& pair : sortedChars) {
        m_sortedCharacters.push_back(pair.first);
        m_sortedDensities.push_back(pair.second);
    }
}

float32 ASCIIConverter::EstimateCharacterDensity(char character) {
    // Simple heuristic based on ASCII value and common character densities
    if (character >= 'A' && character <= 'Z') {
        return 0.6f + (character - 'A') * 0.01f; // A=0.6, Z=0.85
    } else if (character >= 'a' && character <= 'z') {
        return 0.4f + (character - 'a') * 0.01f; // a=0.4, z=0.65
    } else if (character >= '0' && character <= '9') {
        return 0.5f + (character - '0') * 0.02f; // 0=0.5, 9=0.68
    } else {
        // Special characters - use a default value
        switch (character) {
            case '!': case '?': return 0.3f;
            case '.': case ',': return 0.1f;
            case '-': case '_': return 0.2f;
            case '+': case '=': return 0.4f;
            case '*': return 0.6f;
            case '#': return 0.8f;
            case '@': return 0.9f;
            case '$': return 0.7f;
            case '%': return 0.75f;
            case '^': return 0.3f;
            case '&': return 0.8f;
            default: return 0.5f;
        }
    }
}

Result ASCIIConverter::CreateFontAtlas(uint32_t fontSize, const std::string& fontName) {
    #if NEONGLYPH_HAVE_FREETYPE
    // Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        return Result::InitializationFailed;
    }
    
    // Load font face
    FT_Face face;
    std::string fontPath = "C:/Windows/Fonts/" + fontName + ".ttf";
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        // Try alternative font
        fontPath = "C:/Windows/Fonts/consola.ttf";
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
            FT_Done_FreeType(ft);
            return Result::FileNotFound;
        }
    }
    
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    std::vector<std::pair<char, FT_GlyphSlot>> slots;
    m_glyphMap.clear();
    m_kerningMap.clear();
    for (uint32_t i = 0; i < m_charset.size(); i++) {
        char c = m_charset[i];
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        slots.emplace_back(c, face->glyph);
    }
    uint32_t atlasWidth = std::max<uint32_t>(256u, fontSize * 32u);
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t shelfH = 0;
    std::vector<uint8_t> atlasData(atlasWidth * atlasWidth, 0);
    for (auto& p : slots) {
        char c = p.first;
        FT_GlyphSlot g = p.second;
        uint32_t gw = static_cast<uint32_t>(g->bitmap.width);
        uint32_t gh = static_cast<uint32_t>(g->bitmap.rows);
        uint32_t pad = 1;
        if (gw + pad > atlasWidth) continue;
        if (x + gw + pad > atlasWidth) { x = 0; y += shelfH + pad; shelfH = 0; }
        if (gh + pad > shelfH) shelfH = gh + pad;
        if ((y + gh + pad) * atlasWidth >= atlasData.size()) {
            uint32_t newH = y + gh + pad;
            atlasData.resize(static_cast<size_t>(atlasWidth) * newH);
        }
        for (uint32_t row = 0; row < gh; row++) {
            for (uint32_t col = 0; col < gw; col++) {
                uint32_t srcIndex = row * g->bitmap.pitch + col;
                uint32_t dstIndex = (y + row) * atlasWidth + (x + col);
                atlasData[dstIndex] = g->bitmap.buffer[srcIndex];
            }
        }
        GlyphInfo info;
        info.u = static_cast<uint16_t>(x);
        info.v = static_cast<uint16_t>(y);
        info.w = static_cast<uint16_t>(gw);
        info.h = static_cast<uint16_t>(gh);
        info.bx = static_cast<int16_t>(g->bitmap_left);
        info.by = static_cast<int16_t>(g->bitmap_top);
        info.adv = static_cast<uint16_t>(g->advance.x >> 6);
        m_glyphMap[c] = info;
        x += gw + pad;
    }
    uint32_t atlasHeight = static_cast<uint32_t>(atlasData.size() / atlasWidth);
    if (FT_HAS_KERNING(face)) {
        for (char a : m_charset) {
            for (char b : m_charset) {
                FT_Vector kern;
                if (FT_Get_Kerning(face, FT_Get_Char_Index(face, a), FT_Get_Char_Index(face, b), FT_KERNING_DEFAULT, &kern) == 0) {
                    int16_t k = static_cast<int16_t>(kern.x >> 6);
                    uint32_t key = (static_cast<uint32_t>(static_cast<uint8_t>(a)) << 16) | static_cast<uint32_t>(static_cast<uint8_t>(b));
                    if (k != 0) m_kerningMap[key] = k;
                }
            }
        }
    }
    Result result = CreateAtlasImage(atlasWidth, atlasHeight, atlasData);
    
    // Cleanup
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    
    return result;
    #else
    // Fallback implementation when FreeType is not available
    std::cout << "[ASCIIConverter] FreeType not available, creating basic font atlas..." << std::endl;
    
    // Create a basic font atlas without FreeType
    uint32_t charWidth = fontSize;
    uint32_t charHeight = fontSize;
    uint32_t atlasWidth = charWidth * 16; // 16x16 grid
    uint32_t atlasHeight = charHeight * 16;
    
    // Create atlas bitmap with basic character patterns
    std::vector<uint8_t> atlasData(atlasWidth * atlasHeight, 0);
    
    // Simple character rendering - just draw basic shapes for common characters
    for (uint32_t i = 0; i < m_charset.size() && i < 256; i++) {
        char c = m_charset[i % m_charset.size()];
        
        // Calculate position in atlas
        uint32_t x = (i % 16) * charWidth;
        uint32_t y = (i / 16) * charHeight;
        
        // Draw simple patterns for different character types
        uint8_t intensity = 0;
        if (c == '@' || c == '#' || c == '8' || c == 'M') {
            intensity = 255; // High density
        } else if (c == '%' || c == '*' || c == 'H' || c == 'W') {
            intensity = 200; // Medium-high density
        } else if (c == '+' || c == '=' || c == 'h' || c == 'w') {
            intensity = 150; // Medium density
        } else if (c == '-' || c == ':' || c == 'o' || c == 'a') {
            intensity = 100; // Medium-low density
        } else if (c == '.' || c == ' ' || c == '\'' || c == ',') {
            intensity = 50; // Low density
        } else {
            intensity = 128; // Default medium density
        }
        
        // Fill character cell with the calculated intensity
        for (uint32_t row = 1; row < charHeight - 1; row++) {
            for (uint32_t col = 1; col < charWidth - 1; col++) {
                if (row > 1 && row < charHeight - 2 && col > 1 && col < charWidth - 2) {
                    atlasData[(y + row) * atlasWidth + (x + col)] = intensity;
                }
            }
        }
    }
    
    std::cout << "[ASCIIConverter] Basic font atlas created with " << atlasWidth << "x" << atlasHeight << " dimensions" << std::endl;
    
    // Create Vulkan image from atlas data
    Result result = CreateAtlasImage(atlasWidth, atlasHeight, atlasData);
    
    std::cout << "[ASCIIConverter] Atlas image creation result: " << (result == Result::Success ? "SUCCESS" : "FAILED") << std::endl;
    
    return result;
    #endif
}

Result ASCIIConverter::CreateAtlasImage(uint32_t width, uint32_t height, const std::vector<uint8_t>& data) {
    VkDevice device = m_vulkanContext->GetDevice();
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    
    VkDeviceSize imageSize = width * height;
    
    Result result = m_vulkanContext->CreateBuffer(
        imageSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, 
        stagingMemory
    );
    
    if (result != Result::Success) {
        return result;
    }
    
    // Copy data to staging buffer
    void* mappedData;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mappedData);
    memcpy(mappedData, data.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingMemory);
    
    // Create font atlas image
    result = m_vulkanContext->CreateImage(
        width, height,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_fontAtlas,
        m_fontAtlasMemory
    );
    
    if (result != Result::Success) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return result;
    }
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_fontAtlas;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &m_fontAtlasView) != VK_SUCCESS) {
        vkDestroyImage(device, m_fontAtlas, nullptr);
        vkFreeMemory(device, m_fontAtlasMemory, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return Result::InitializationFailed;
    }
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_fontAtlasSampler) != VK_SUCCESS) {
        vkDestroyImageView(device, m_fontAtlasView, nullptr);
        vkDestroyImage(device, m_fontAtlas, nullptr);
        vkFreeMemory(device, m_fontAtlasMemory, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return Result::InitializationFailed;
    }
    
    // Cleanup staging resources
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
    
    return Result::Success;
}

Result ASCIIConverter::CreateGPUResources() {
    VkDevice device = m_vulkanContext->GetDevice();
    
    // Create conversion buffer for parameters
    VkDeviceSize bufferSize = sizeof(float32) * 256; // Enough for conversion parameters
    
    Result result = m_vulkanContext->CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_conversionBuffer,
        m_conversionBufferMemory
    );
    
    if (result != Result::Success) {
        return result;
    }
    
    // Create output buffer
    bufferSize = sizeof(uint32_t) * 80u * 24u;
    
    result = m_vulkanContext->CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_outputBuffer,
        m_outputBufferMemory
    );
    
    if (result != Result::Success) return result;
    VkDeviceSize rbSize = sizeof(uint32_t) * 80u * 24u;
    result = m_vulkanContext->CreateBuffer(
        rbSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_outputReadbackBuffer,
        m_outputReadbackMemory
    );
    return result;
}

Result ASCIIConverter::CreateComputePipeline() {
    m_computeManager = std::make_unique<ComputeShaderManager>(m_vulkanContext);
    Result r = m_computeManager->Initialize();
    if (r != Result::Success) return r;
    m_asciiPipeline = std::make_unique<ASCIIComputePipeline>(m_vulkanContext, m_computeManager.get());
    r = m_asciiPipeline->Initialize(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE);
    if (r != Result::Success) return r;
    m_asciiPipeline->SetCharset(m_charset);
    m_asciiPipeline->SetBrightness(m_brightness);
    m_asciiPipeline->SetContrast(m_contrast);
    return Result::Success;
}

Result ASCIIConverter::ConvertCPU(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                                 ASCIIMapping* outputBuffer, uint32_t bufferSize) {
    uint32_t charWidth = width / 80; // Assuming 80 character width
    uint32_t charHeight = height / 24; // Assuming 24 character height
    
    for (uint32_t y = 0; y < 24; y++) {
        for (uint32_t x = 0; x < 80; x++) {
            // Sample region
            uint32_t startX = x * charWidth;
            uint32_t startY = y * charHeight;
            uint32_t endX = std::min(startX + charWidth, width);
            uint32_t endY = std::min(startY + charHeight, height);
            
            // Calculate average luminance
            float32 totalLuminance = 0.0f;
            uint32_t sampleCount = 0;
            
            for (uint32_t py = startY; py < endY; py++) {
                for (uint32_t px = startX; px < endX; px++) {
                    uint32_t index = (py * width + px) * 4; // RGBA
                    if (index + 2 < width * height * 4) {
                        float32 r = inputTexture[index] / 255.0f;
                        float32 g = inputTexture[index + 1] / 255.0f;
                        float32 b = inputTexture[index + 2] / 255.0f;
                        
                        float32 luminance = 0.299f * r + 0.587f * g + 0.114f * b;
                        luminance = (luminance * m_contrast) + m_brightness;
                        luminance = std::clamp(luminance, 0.0f, 1.0f);
                        
                        totalLuminance += luminance;
                        sampleCount++;
                    }
                }
            }
            
            float32 avgLuminance = sampleCount > 0 ? totalLuminance / sampleCount : 0.0f;
            
            // Find best character
            char bestChar = FindBestCharacter(avgLuminance);
            
            // Store result
            uint32_t outputIndex = y * 80 + x;
            if (outputIndex < bufferSize) {
                outputBuffer[outputIndex] = ASCIIMapping(bestChar, avgLuminance, 0);
            }
        }
    }
    
    return Result::Success;
}

Result ASCIIConverter::ConvertSIMD(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                                  ASCIIMapping* outputBuffer, uint32_t bufferSize) {
    uint32_t charWidth = width / 80;
    uint32_t charHeight = height / 24;
    for (uint32_t y = 0; y < 24; y++) {
        for (uint32_t x = 0; x < 80; x++) {
            uint32_t startX = x * charWidth;
            uint32_t startY = y * charHeight;
            uint32_t endX = std::min(startX + charWidth, width);
            uint32_t endY = std::min(startY + charHeight, height);
            float total = 0.0f;
            uint32_t count = 0;
            for (uint32_t py = startY; py < endY; py++) {
                uint32_t idx = (py * width + startX) * 4;
                uint32_t cols = endX - startX;
                uint32_t simdCols = (cols / 8) * 8;
                __m256 sum = _mm256_setzero_ps();
                uint32_t i = 0;
                for (; i < simdCols; i += 8) {
                    __m256 r = _mm256_cvtepi32_ps(_mm256_set_epi32(
                        inputTexture[idx + (i+7)*4 + 0], inputTexture[idx + (i+6)*4 + 0], inputTexture[idx + (i+5)*4 + 0], inputTexture[idx + (i+4)*4 + 0],
                        inputTexture[idx + (i+3)*4 + 0], inputTexture[idx + (i+2)*4 + 0], inputTexture[idx + (i+1)*4 + 0], inputTexture[idx + i*4 + 0]));
                    __m256 g = _mm256_cvtepi32_ps(_mm256_set_epi32(
                        inputTexture[idx + (i+7)*4 + 1], inputTexture[idx + (i+6)*4 + 1], inputTexture[idx + (i+5)*4 + 1], inputTexture[idx + (i+4)*4 + 1],
                        inputTexture[idx + (i+3)*4 + 1], inputTexture[idx + (i+2)*4 + 1], inputTexture[idx + (i+1)*4 + 1], inputTexture[idx + i*4 + 1]));
                    __m256 b = _mm256_cvtepi32_ps(_mm256_set_epi32(
                        inputTexture[idx + (i+7)*4 + 2], inputTexture[idx + (i+6)*4 + 2], inputTexture[idx + (i+5)*4 + 2], inputTexture[idx + (i+4)*4 + 2],
                        inputTexture[idx + (i+3)*4 + 2], inputTexture[idx + (i+2)*4 + 2], inputTexture[idx + (i+1)*4 + 2], inputTexture[idx + i*4 + 2]));
                    __m256 rf = _mm256_mul_ps(r, _mm256_set1_ps(1.0f/255.0f));
                    __m256 gf = _mm256_mul_ps(g, _mm256_set1_ps(1.0f/255.0f));
                    __m256 bf = _mm256_mul_ps(b, _mm256_set1_ps(1.0f/255.0f));
                    __m256 lum = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(rf, _mm256_set1_ps(0.299f)), _mm256_mul_ps(gf, _mm256_set1_ps(0.587f))), _mm256_mul_ps(bf, _mm256_set1_ps(0.114f)));
                    lum = _mm256_add_ps(_mm256_mul_ps(lum, _mm256_set1_ps(m_contrast)), _mm256_set1_ps(m_brightness));
                    __m256 cl0 = _mm256_max_ps(lum, _mm256_set1_ps(0.0f));
                    __m256 cl1 = _mm256_min_ps(cl0, _mm256_set1_ps(1.0f));
                    sum = _mm256_add_ps(sum, cl1);
                }
                float tmp[8];
                _mm256_storeu_ps(tmp, sum);
                total += tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];
                count += simdCols;
                for (; i < cols; i++) {
                    uint32_t p = idx + i*4;
                    float r = inputTexture[p] * (1.0f/255.0f);
                    float g = inputTexture[p+1] * (1.0f/255.0f);
                    float b = inputTexture[p+2] * (1.0f/255.0f);
                    float lum = 0.299f*r + 0.587f*g + 0.114f*b;
                    lum = lum * m_contrast + m_brightness;
                    lum = std::clamp(lum, 0.0f, 1.0f);
                    total += lum;
                    count++;
                }
            }
            float avg = count ? (total / count) : 0.0f;
            char c = FindBestCharacter(avg);
            uint32_t oi = y * 80 + x;
            if (oi < bufferSize) {
                outputBuffer[oi] = ASCIIMapping(c, avg, 0);
            }
        }
    }
    return Result::Success;
}

Result ASCIIConverter::ConvertGPU(VkImage inputImage, uint32_t width, uint32_t height,
                                  VkBuffer outputBuffer, uint32_t& outputSize) {
    uint32_t fs = std::max<uint32_t>(8u, m_config.ascii.fontSize);
    uint32_t cols = std::max<uint32_t>(16u, width / fs);
    uint32_t rows = std::max<uint32_t>(8u, height / (fs + fs / 2));
    if (m_asciiPipeline) {
        m_asciiPipeline->SetAsciiDimensions(cols, rows);
    }
    if (!m_computeAvailable) {
        VkDevice device = m_vulkanContext->GetDevice();
        void* ptr = nullptr;
        VkDeviceSize size = sizeof(uint32_t) * 80u * 24u;
        if (vkMapMemory(device, m_outputReadbackMemory, 0, size, 0, &ptr) == VK_SUCCESS) {
            std::memset(ptr, 0, static_cast<size_t>(size));
            vkUnmapMemory(device, m_outputReadbackMemory);
            outputSize = 80u * 24u;
            return Result::Success;
        }
        return Result::Error;
    }
    VkCommandBuffer cmd;
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = m_vulkanContext->GetCommandPool();
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(m_vulkanContext->GetDevice(), &ai, &cmd) != VK_SUCCESS) return Result::InitializationFailed;
    }
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) return Result::Error;
    Result r = m_asciiPipeline->Convert(cmd, inputImage, width, height, m_outputBuffer);
    if (r != Result::Success) return r;
    VkBufferCopy bc{};
    bc.srcOffset = 0;
    bc.dstOffset = 0;
    bc.size = sizeof(uint32_t) * 80u * 24u;
    vkCmdCopyBuffer(cmd, m_outputBuffer, m_outputReadbackBuffer, 1, &bc);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) return Result::Error;
    r = m_vulkanContext->SubmitComputeWork(cmd);
    if (r != Result::Success) return r;
    void* data = nullptr;
    if (vkMapMemory(m_vulkanContext->GetDevice(), m_outputReadbackMemory, 0, bc.size, 0, &data) != VK_SUCCESS) return Result::Error;
    if (outputBuffer != VK_NULL_HANDLE) {
        VkDevice device = m_vulkanContext->GetDevice();
        VkBufferCopy bc2{};
        bc2.srcOffset = 0;
        bc2.dstOffset = 0;
        bc2.size = bc.size;
        VkCommandBuffer cmd2;
        VkCommandBufferAllocateInfo ai2{};
        ai2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai2.commandPool = m_vulkanContext->GetCommandPool();
        ai2.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai2.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device, &ai2, &cmd2) == VK_SUCCESS) {
            VkCommandBufferBeginInfo bi2{};
            bi2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bi2.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            if (vkBeginCommandBuffer(cmd2, &bi2) == VK_SUCCESS) {
                vkCmdCopyBuffer(cmd2, m_outputReadbackBuffer, outputBuffer, 1, &bc2);
                vkEndCommandBuffer(cmd2);
                m_vulkanContext->SubmitComputeWork(cmd2);
            }
        }
    }
    vkUnmapMemory(m_vulkanContext->GetDevice(), m_outputReadbackMemory);
    outputSize = 80u * 24u;
    return Result::Success;
}

char ASCIIConverter::FindBestCharacter(float32 density) {
    // Binary search for best character
    if (m_sortedDensities.empty()) return ' ';
    
    auto it = std::lower_bound(m_sortedDensities.begin(), m_sortedDensities.end(), density);
    size_t index = std::distance(m_sortedDensities.begin(), it);
    
    if (index >= m_sortedCharacters.size()) {
        index = m_sortedCharacters.size() - 1;
    }
    
    return m_sortedCharacters[index];
}

uint32 ASCIIConverter::FindBestColor(const uint8_t* pixel) {
    // Simple color quantization
    uint32_t r = pixel[0] >> 4; // 4 bits per channel
    uint32_t g = pixel[1] >> 4;
    uint32_t b = pixel[2] >> 4;
    
    return (r << 8) | (g << 4) | b;
}

float32 ASCIIConverter::CalculateLuminance(const uint8_t* pixel) {
    float32 r = pixel[0] / 255.0f;
    float32 g = pixel[1] / 255.0f;
    float32 b = pixel[2] / 255.0f;
    
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

void ASCIIConverter::SetThreadCount(uint32_t threadCount) {
    m_threadCount = std::max(1u, threadCount);
}

void ASCIIConverter::EnableSIMD(bool enable) {
    m_useSIMD = enable;
}

void ASCIIConverter::SetAntialiasing(bool enable) {
    m_antialiasing = enable;
}

void ASCIIConverter::SetSubpixelRendering(bool enable) {
    m_subpixelRendering = enable;
}

void ASCIIConverter::SetContrast(float32 contrast) {
    m_contrast = std::max(0.1f, contrast);
}

void ASCIIConverter::SetBrightness(float32 brightness) {
    m_brightness = std::clamp(brightness, -1.0f, 1.0f);
}

int16_t ASCIIConverter::GetKerning(char left, char right) const {
    uint32_t key = (static_cast<uint32_t>(static_cast<uint8_t>(left)) << 16) | static_cast<uint32_t>(static_cast<uint8_t>(right));
    auto it = m_kerningMap.find(key);
    return it != m_kerningMap.end() ? it->second : 0;
}
} // namespace NeonGlyph
