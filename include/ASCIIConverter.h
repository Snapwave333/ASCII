#pragma once

#include "NeonGlyph.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>
#include <string>
#include "ComputePipelines.h"

namespace NeonGlyph {

class ASCIIConverter {
public:
    ASCIIConverter();
    ~ASCIIConverter();

    Result Initialize(VulkanContext* vulkanContext, const Config& config);
    void Shutdown();
    
    // Conversion settings
    void SetCharset(const std::string& charset);
    void SetDensityMapping(const std::vector<float32>& densityMap);
    void SetColorPalette(const ColorPalette& palette);
    
    // Real-time conversion
    Result ConvertFrame(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                       ASCIIMapping* outputBuffer, uint32_t bufferSize);
    
    // GPU-accelerated conversion
    Result ConvertFrameGPU(VkImage inputImage, uint32_t width, uint32_t height,
                          VkBuffer outputBuffer, uint32_t& outputSize);
    
    // Font atlas management
    Result GenerateFontAtlas(uint32_t fontSize, const std::string& fontName);
    VkImageView GetFontAtlasView() const { return m_fontAtlasView; }
    VkSampler GetFontAtlasSampler() const { return m_fontAtlasSampler; }
    struct GlyphInfo { uint16_t u; uint16_t v; uint16_t w; uint16_t h; int16_t bx; int16_t by; uint16_t adv; };
    const std::unordered_map<char, GlyphInfo>& GetGlyphMap() const { return m_glyphMap; }
    int16_t GetKerning(char left, char right) const;
    
    // Character density analysis
    float32 CalculateCharacterDensity(char character) const;
    void BuildDensityLookupTable();
    
    // Performance optimization
    void SetThreadCount(uint32_t threadCount);
    void EnableSIMD(bool enable);
    
    // Quality settings
    void SetAntialiasing(bool enable);
    void SetSubpixelRendering(bool enable);
    void SetContrast(float32 contrast);
    void SetBrightness(float32 brightness);

private:
    // Vulkan context
    VulkanContext* m_vulkanContext;
    
    // Conversion parameters
    std::string m_charset;
    std::vector<float32> m_densityMap;
    ColorPalette m_colorPalette;
    
    // Font atlas
    VkImage m_fontAtlas;
    VkDeviceMemory m_fontAtlasMemory;
    VkImageView m_fontAtlasView;
    VkSampler m_fontAtlasSampler;
    std::unordered_map<char, GlyphInfo> m_glyphMap;
    std::unordered_map<uint32_t, int16_t> m_kerningMap;
    
    // GPU resources
    VkBuffer m_conversionBuffer;
    VkDeviceMemory m_conversionBufferMemory;
    VkBuffer m_outputBuffer;
    VkDeviceMemory m_outputBufferMemory;
    VkBuffer m_outputReadbackBuffer;
    VkDeviceMemory m_outputReadbackMemory;
    
    // Compute pipeline
    VkPipeline m_conversionPipeline;
    VkPipelineLayout m_conversionPipelineLayout;
    VkDescriptorSetLayout m_conversionDescriptorSetLayout;
    VkDescriptorSet m_conversionDescriptorSet;
    std::unique_ptr<ComputeShaderManager> m_computeManager;
    std::unique_ptr<ASCIIComputePipeline> m_asciiPipeline;
    bool m_computeAvailable = false;
    
    // Density lookup table
    std::unordered_map<char, float32> m_densityLookup;
    std::vector<float32> m_sortedDensities;
    std::vector<char> m_sortedCharacters;
    
    // Performance settings
    uint32_t m_threadCount;
    bool m_useSIMD;
    bool m_antialiasing;
    bool m_subpixelRendering;
    float32 m_contrast;
    float32 m_brightness;
    
    // Configuration
    Config m_config;
    
    // Private methods
    Result CreateFontAtlas(uint32_t fontSize, const std::string& fontName);
    float32 EstimateCharacterDensity(char character);
    Result CreateAtlasImage(uint32_t width, uint32_t height, const std::vector<uint8_t>& data);
    Result CreateGPUResources();
    Result CreateComputePipeline();
    Result CreateDescriptorSets();
    
    // Character rendering
    Result RenderCharacterToAtlas(char character, uint32_t x, uint32_t y, uint32_t charWidth, uint32_t charHeight);
    float32 CalculatePixelDensity(const uint8_t* pixels, uint32_t width, uint32_t height);
    
    // Conversion algorithms
    Result ConvertCPU(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                     ASCIIMapping* outputBuffer, uint32_t bufferSize);
    Result ConvertGPU(VkImage inputImage, uint32_t width, uint32_t height,
                     VkBuffer outputBuffer, uint32_t& outputSize);
    
    // SIMD optimizations
    Result ConvertSIMD(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                      ASCIIMapping* outputBuffer, uint32_t bufferSize);
    
    // Threading
    void ConvertThreaded(const uint8_t* inputTexture, uint32_t width, uint32_t height, 
                        ASCIIMapping* outputBuffer, uint32_t startRow, uint32_t endRow);
    
    // Utility functions
    char FindBestCharacter(float32 density);
    uint32 FindBestColor(const uint8_t* pixel);
    float32 CalculateLuminance(const uint8_t* pixel);
    
    // Shader management
    Result CreateShaderModule(const std::vector<uint32_t>& code, VkShaderModule& shaderModule);
    std::vector<uint32_t> LoadShaderCode(const std::string& filename);
};

} // namespace NeonGlyph
