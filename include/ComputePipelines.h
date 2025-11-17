#pragma once

#include "NeonGlyph.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <map>

namespace NeonGlyph {

// Compute shader compiler and manager
class ComputeShaderManager {
public:
    ComputeShaderManager(VulkanContext* context);
    ~ComputeShaderManager();
    
    Result Initialize();
    void Shutdown();
    
    // Load and compile compute shaders
    Result LoadShader(const std::string& name, const std::string& filepath);
    Result CompileShader(const std::string& name, const std::string& source);
    
    // Get shader by name
    VkShaderModule GetShader(const std::string& name) const;
    
    // Create compute pipeline
    Result CreateComputePipeline(const std::string& shaderName, VkPipelineLayout layout, VkPipeline& pipeline);
    
    // Execute compute work
    Result Dispatch(VkCommandBuffer commandBuffer, const std::string& pipelineName, 
                   uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    
    // Built-in shader sources
    static const char* GetASCIIShaderSource();
    static const char* GetBeatDetectionShaderSource();
    static const char* GetAudioAnalysisShaderSource();

private:
    VulkanContext* m_context;
    VkDevice m_device;
    
    struct ShaderModule {
        VkShaderModule module;
        std::string source;
        std::vector<uint32_t> spirv;
    };
    
    std::map<std::string, ShaderModule> m_shaders;
    std::map<std::string, VkPipeline> m_pipelines;
    
    // SPIR-V compilation
    Result CompileGLSLToSPIRV(const std::string& source, std::vector<uint32_t>& spirv);
    Result CreateShaderModule(const std::vector<uint32_t>& spirv, VkShaderModule& module);
    
    // Shader validation
    Result ValidateShader(const std::string& source);
};

// ASCII conversion compute pipeline
class ASCIIComputePipeline {
public:
    ASCIIComputePipeline(VulkanContext* context, ComputeShaderManager* shaderManager);
    ~ASCIIComputePipeline();
    
    Result Initialize(uint32_t maxWidth, uint32_t maxHeight);
    void Shutdown();
    
    // Convert texture to ASCII
    Result Convert(VkCommandBuffer commandBuffer, VkImage inputTexture, 
                  uint32_t width, uint32_t height, VkBuffer outputBuffer);
    
    // Update conversion parameters
    void SetCharset(const std::string& charset);
    void SetBrightness(float32 brightness);
    void SetContrast(float32 contrast);
    void SetColorPalette(const ColorPalette& palette);
    void SetAsciiDimensions(uint32_t cols, uint32_t rows);

private:
    VulkanContext* m_context;
    ComputeShaderManager* m_shaderManager;
    VkDevice m_device;
    
    // Pipeline objects
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    
    // Buffers
    VkBuffer m_parameterBuffer;
    VkDeviceMemory m_parameterMemory;
    VkBuffer m_charsetBuffer;
    VkDeviceMemory m_charsetMemory;
    VkImageView m_inputImageView;
    VkImage m_lastInputImage;
    
    // Parameters
    struct ConversionParams {
        uint32_t textureWidth;
        uint32_t textureHeight;
        uint32_t asciiWidth;
        uint32_t asciiHeight;
        float32 brightness;
        float32 contrast;
        uint32_t charsetSize;
        uint32_t padding[3]; // 16-byte alignment
    } m_params;
    
    std::string m_charset;
    std::vector<float32> m_charsetDensities;
    
    // Private methods
    Result CreateDescriptorSetLayout();
    Result CreatePipelineLayout();
    Result CreatePipeline();
    Result CreateDescriptorPool();
    Result CreateBuffers();
    Result UpdateDescriptorSets(VkImage inputTexture, VkBuffer outputBuffer);
    Result UpdateParameters();
};

// Audio analysis compute pipeline
class AudioComputePipeline {
public:
    AudioComputePipeline(VulkanContext* context, ComputeShaderManager* shaderManager);
    ~AudioComputePipeline();
    
    Result Initialize(uint32_t maxSpectrumSize);
    void Shutdown();
    
    // Analyze audio spectrum
    Result AnalyzeSpectrum(VkCommandBuffer commandBuffer, VkBuffer spectrumBuffer, 
                          uint32_t spectrumSize, VkBuffer outputBuffer);
    
    // Beat detection
    Result DetectBeat(VkCommandBuffer commandBuffer, VkBuffer spectrumBuffer, 
                     uint32_t spectrumSize, VkBuffer beatOutputBuffer);
    
    // Update analysis parameters
    void SetBeatThreshold(float32 threshold);
    void SetSensitivity(float32 sensitivity);

private:
    VulkanContext* m_context;
    ComputeShaderManager* m_shaderManager;
    VkDevice m_device;
    
    // Pipeline objects
    VkPipeline m_spectrumPipeline;
    VkPipeline m_beatPipeline;
    VkPipelineLayout m_spectrumLayout;
    VkPipelineLayout m_beatLayout;
    VkDescriptorSetLayout m_spectrumDescriptorLayout;
    VkDescriptorSetLayout m_beatDescriptorLayout;
    VkDescriptorPool m_descriptorPool;
    
    // Buffers
    VkBuffer m_parameterBuffer;
    VkDeviceMemory m_parameterMemory;
    VkBuffer m_historyBuffer;
    VkDeviceMemory m_historyMemory;
    
    // Parameters
    struct AnalysisParams {
        uint32_t spectrumSize;
        float32 beatThreshold;
        float32 sensitivity;
        uint32_t historySize;
        uint32_t padding[3]; // 16-byte alignment
    } m_params;
    
    // Private methods
    Result CreateDescriptorSetLayouts();
    Result CreatePipelineLayouts();
    Result CreatePipelines();
    Result CreateDescriptorPool();
    Result CreateBuffers();
    Result UpdateParameters();
};

} // namespace NeonGlyph