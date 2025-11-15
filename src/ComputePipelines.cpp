#include "ComputePipelines.h"
#include "VulkanContext.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <filesystem>

namespace NeonGlyph {

// Built-in shader sources
const char* ComputeShaderManager::GetASCIIShaderSource() {
    return R"(
#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Input texture
layout(binding = 0, rgba8) uniform readonly image2D inputTexture;

// ASCII conversion parameters
layout(binding = 1) uniform ConversionParams {
    uvec2 textureSize;
    uvec2 asciiSize;
    float brightness;
    float contrast;
    uint charsetSize;
    uint padding[3];
} params;

// Character density lookup table
layout(binding = 2) buffer CharsetBuffer {
    float densities[];
} charset;

// Output ASCII data
layout(binding = 3) buffer OutputBuffer {
    uint asciiData[];
} output;

// Utility functions
float luminance(vec4 color) {
    return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

float adjustBrightnessContrast(float value, float brightness, float contrast) {
    return (value * contrast) + brightness;
}

char findBestCharacter(float density) {
    float minDiff = 1.0;
    int bestIndex = 0;
    
    for (uint i = 0; i < params.charsetSize; i++) {
        float diff = abs(density - charset.densities[i]);
        if (diff < minDiff) {
            minDiff = diff;
            bestIndex = int(i);
        }
    }
    
    return char(bestIndex);
}

void main() {
    ivec2 globalID = ivec2(gl_GlobalInvocationID.xy);
    
    // Check bounds
    if (globalID.x >= params.asciiSize.x || globalID.y >= params.asciiSize.y) {
        return;
    }
    
    // Calculate texture sampling region
    ivec2 texelSize = ivec2(params.textureSize) / ivec2(params.asciiSize);
    ivec2 sampleStart = globalID * texelSize;
    ivec2 sampleEnd = min(sampleStart + texelSize, ivec2(params.textureSize));
    
    // Sample and average the region
    float totalLuminance = 0.0;
    int sampleCount = 0;
    
    for (int y = sampleStart.y; y < sampleEnd.y; y++) {
        for (int x = sampleStart.x; x < sampleEnd.x; x++) {
            vec4 color = imageLoad(inputTexture, ivec2(x, y));
            float lum = luminance(color);
            lum = adjustBrightnessContrast(lum, params.brightness, params.contrast);
            totalLuminance += lum;
            sampleCount++;
        }
    }
    
    float avgLuminance = sampleCount > 0 ? totalLuminance / float(sampleCount) : 0.0;
    
    // Map luminance to character density
    float normalizedDensity = clamp(avgLuminance, 0.0, 1.0);
    
    // Find best matching character
    char bestChar = findBestCharacter(normalizedDensity);
    
    // Calculate output index
    uint outputIndex = globalID.y * params.asciiSize.x + globalID.x;
    
    // Store ASCII character and properties
    uint asciiValue = uint(bestChar);
    uint packedData = (asciiValue & 0xFF) | ((uint(avgLuminance * 255.0) & 0xFF) << 8);
    
    output.asciiData[outputIndex] = packedData;
}
)";
}

const char* ComputeShaderManager::GetBeatDetectionShaderSource() {
    return R"(
#version 450

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Input spectrum data
layout(binding = 0) buffer SpectrumBuffer {
    float magnitudes[];
} spectrum;

// Beat detection parameters
layout(binding = 1) uniform BeatParams {
    uint spectrumSize;
    float threshold;
    float sensitivity;
    uint historySize;
    uint padding;
} params;

// Beat detection output
layout(binding = 2) buffer BeatOutput {
    uint isBeat;
    float bpm;
    float energy;
    uint timestamp;
} output;

// Energy history for adaptive threshold
layout(binding = 3) buffer EnergyHistory {
    float energies[];
} history;

// Utility functions
float calculateEnergy(float[] magnitudes, uint size) {
    float energy = 0.0;
    for (uint i = 0; i < size; i++) {
        energy += magnitudes[i] * magnitudes[i];
    }
    return sqrt(energy / float(size));
}

float calculateAverageEnergy(float[] energies, uint size) {
    float sum = 0.0;
    for (uint i = 0; i < size; i++) {
        sum += energies[i];
    }
    return sum / float(size);
}

float calculateVariance(float[] values, uint size, float mean) {
    float variance = 0.0;
    for (uint i = 0; i < size; i++) {
        float diff = values[i] - mean;
        variance += diff * diff;
    }
    return variance / float(size);
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    
    // Only process on the first thread
    if (globalID != 0) {
        return;
    }
    
    // Calculate current energy
    float currentEnergy = calculateEnergy(spectrum.magnitudes, params.spectrumSize);
    
    // Calculate average energy from history
    float avgEnergy = calculateAverageEnergy(history.energies, params.historySize);
    float variance = calculateVariance(history.energies, params.historySize, avgEnergy);
    float stdDev = sqrt(variance);
    
    // Adaptive threshold
    float threshold = avgEnergy + params.sensitivity * stdDev;
    
    // Beat detection
    bool isBeat = currentEnergy > threshold;
    
    // Update output
    output.isBeat = isBeat ? 1u : 0u;
    output.energy = currentEnergy;
    output.timestamp = uint(gl_GlobalInvocationID.x);
    
    // Simple BPM estimation
    if (isBeat) {
        output.bpm = 120.0; // This would be calculated from beat intervals
    } else {
        output.bpm = 0.0;
    }
    
    // Update energy history (shift and add new)
    for (uint i = params.historySize - 1; i > 0; i--) {
        history.energies[i] = history.energies[i - 1];
    }
    history.energies[0] = currentEnergy;
}
)";
}

const char* ComputeShaderManager::GetAudioAnalysisShaderSource() {
    return R"(
#version 450

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Audio data
layout(binding = 0) buffer AudioBuffer {
    float samples[];
} audio;

// Analysis parameters
layout(binding = 1) uniform AnalysisParams {
    uint sampleCount;
    uint sampleRate;
    uint windowSize;
    uint padding;
} params;

// Output spectrum
layout(binding = 2) buffer SpectrumOutput {
    float magnitudes[];
} spectrum;

// FFT implementation (simplified)
void fft(inout float[] real, inout float[] imag, uint n) {
    uint j = 0;
    for (uint i = 1; i < n; i++) {
        uint bit = n >> 1;
        while (j >= bit) {
            j -= bit;
            bit >>= 1;
        }
        j += bit;
        if (i < j) {
            float tempReal = real[i];
            float tempImag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tempReal;
            imag[j] = tempImag;
        }
    }
    
    for (uint len = 2; len <= n; len <<= 1) {
        float angle = -2.0 * 3.14159265 / float(len);
        float wlenReal = cos(angle);
        float wlenImag = sin(angle);
        
        for (uint i = 0; i < n; i += len) {
            float wReal = 1.0;
            float wImag = 0.0;
            
            for (uint j = 0; j < len / 2; j++) {
                uint u = i + j;
                uint v = i + j + len / 2;
                
                float tReal = wReal * real[v] - wImag * imag[v];
                float tImag = wReal * imag[v] + wImag * real[v];
                
                real[v] = real[u] - tReal;
                imag[v] = imag[u] - tImag;
                real[u] += tReal;
                imag[u] += tImag;
                
                float nextWReal = wReal * wlenReal - wImag * wlenImag;
                float nextWImag = wReal * wlenImag + wImag * wlenReal;
                wReal = nextWReal;
                wImag = nextWImag;
            }
        }
    }
}

void main() {
    uint globalID = gl_GlobalInvocationID.x;
    
    if (globalID >= params.sampleCount / 2) {
        return;
    }
    
    // Simple FFT implementation
    
    // Calculate magnitude
    float real = audio.samples[globalID * 2];
    float imag = globalID == 0 ? 0.0 : audio.samples[globalID * 2 + 1];
    spectrum.magnitudes[globalID] = sqrt(real * real + imag * imag) / float(params.sampleCount);
}
)";
}

ComputeShaderManager::ComputeShaderManager(VulkanContext* context) 
    : m_context(context), m_device(context->GetDevice()) {
}

ComputeShaderManager::~ComputeShaderManager() {
    Shutdown();
}

static std::string GetAsciiConvertShaderSource();

Result ComputeShaderManager::Initialize() {
    // Load built-in shaders
    Result result = CompileShader("ascii_convert", GetAsciiConvertShaderSource());
    if (result != Result::Success) return result;
    
    result = CompileShader("beat_detection", GetBeatDetectionShaderSource());
    if (result != Result::Success) return result;
    
    result = CompileShader("audio_analysis", GetAudioAnalysisShaderSource());
    if (result != Result::Success) return result;
    
    
    
    return Result::Success;
}

void ComputeShaderManager::Shutdown() {
    for (auto it = m_shaders.begin(); it != m_shaders.end(); ++it) {
        if (it->second.module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, it->second.module, nullptr);
        }
    }
    m_shaders.clear();
    
    for (auto it = m_pipelines.begin(); it != m_pipelines.end(); ++it) {
        if (it->second != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device, it->second, nullptr);
        }
    }
    m_pipelines.clear();
}

Result ComputeShaderManager::LoadShader(const std::string& name, const std::string& filepath) {
    if (filepath.size() >= 4 && filepath.substr(filepath.size() - 4) == ".spv") {
        std::ifstream bin(filepath, std::ios::binary);
        if (!bin.is_open()) return Result::FileNotFound;
        bin.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(bin.tellg());
        bin.seekg(0, std::ios::beg);
        if (size % 4 != 0) return Result::ValidationFailed;
        std::vector<uint32_t> spirv(size / 4);
        bin.read(reinterpret_cast<char*>(spirv.data()), size);
        VkShaderModule module;
        Result r = CreateShaderModule(spirv, module);
        if (r != Result::Success) return r;
        ShaderModule sm; sm.module = module; sm.spirv = spirv; sm.source = "";
        m_shaders[name] = sm;
        return Result::Success;
    } else {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return Result::FileNotFound;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();
        return CompileShader(name, source);
    }
}

Result ComputeShaderManager::CompileShader(const std::string& name, const std::string& source) {
    // Validate shader source
    Result result = ValidateShader(source);
    if (result != Result::Success) {
        return result;
    }
    
    // Compile to SPIR-V
    std::vector<uint32_t> spirv;
    result = CompileGLSLToSPIRV(source, spirv);
    if (result != Result::Success) {
        return result;
    }
    
    // Create shader module
    VkShaderModule module;
    result = CreateShaderModule(spirv, module);
    if (result != Result::Success) {
        return result;
    }
    
    // Store shader
    ShaderModule shader;
    shader.module = module;
    shader.source = source;
    shader.spirv = spirv;
    
    m_shaders[name] = shader;
    
    return Result::Success;
}

VkShaderModule ComputeShaderManager::GetShader(const std::string& name) const {
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second.module : VK_NULL_HANDLE;
}

Result ComputeShaderManager::CreateComputePipeline(const std::string& shaderName, VkPipelineLayout layout, VkPipeline& pipeline) {
    VkShaderModule shaderModule = GetShader(shaderName);
    if (shaderModule == VK_NULL_HANDLE) {
        return Result::Error;
    }
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shaderModule;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = layout;
    
    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    m_pipelines[shaderName] = pipeline;
    return Result::Success;
}

Result ComputeShaderManager::Dispatch(VkCommandBuffer commandBuffer, const std::string& pipelineName, 
                                     uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    auto it = m_pipelines.find(pipelineName);
    if (it == m_pipelines.end()) {
        return Result::Error;
    }
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, it->second);
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
    
    return Result::Success;
}

Result ComputeShaderManager::CompileGLSLToSPIRV(const std::string& source, std::vector<uint32_t>& spirv) {
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
        return Result::ValidationFailed;
    }
    std::filesystem::path tmpDir = std::filesystem::temp_directory_path();
    std::filesystem::path srcPath = tmpDir / "ng_ascii.comp";
    std::filesystem::path spvPath = tmpDir / "ng_ascii.spv";
    {
        std::ofstream out(srcPath.string(), std::ios::out | std::ios::binary);
        out.write(source.data(), static_cast<std::streamsize>(source.size()));
    }
    std::string cmd = "\"" + exe.string() + "\" -V \"" + srcPath.string() + "\" -o \"" + spvPath.string() + "\"";
    std::cout << "[ComputeShaderManager] Running: " << cmd << std::endl;
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
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
    return Result::Success;
}

Result ComputeShaderManager::CreateShaderModule(const std::vector<uint32_t>& spirv, VkShaderModule& module) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();
    
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result ComputeShaderManager::ValidateShader(const std::string& source) {
    // Basic validation - check for required elements
    if (source.find("#version 450") == std::string::npos) {
        return Result::ValidationFailed;
    }
    
    if (source.find("layout(local_size_") == std::string::npos) {
        return Result::ValidationFailed;
    }
    
    if (source.find("void main()") == std::string::npos) {
        return Result::ValidationFailed;
    }
    
    return Result::Success;
}

// ASCII Compute Pipeline Implementation
ASCIIComputePipeline::ASCIIComputePipeline(VulkanContext* context, ComputeShaderManager* shaderManager)
    : m_context(context), m_shaderManager(shaderManager), m_device(context->GetDevice())
    , m_pipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE)
    , m_descriptorSetLayout(VK_NULL_HANDLE), m_descriptorPool(VK_NULL_HANDLE)
    , m_parameterBuffer(VK_NULL_HANDLE), m_parameterMemory(VK_NULL_HANDLE)
    , m_charsetBuffer(VK_NULL_HANDLE), m_charsetMemory(VK_NULL_HANDLE)
    , m_inputImageView(VK_NULL_HANDLE) {
}

ASCIIComputePipeline::~ASCIIComputePipeline() {
    Shutdown();
}

Result ASCIIComputePipeline::Initialize(uint32_t maxWidth, uint32_t maxHeight) {
    Result result = CreateDescriptorSetLayout();
    if (result != Result::Success) return result;
    
    result = CreatePipelineLayout();
    if (result != Result::Success) return result;
    
    result = CreatePipeline();
    if (result != Result::Success) return result;
    
    result = CreateDescriptorPool();
    if (result != Result::Success) return result;
    
    result = CreateBuffers();
    if (result != Result::Success) return result;
    
    // Set default parameters
    m_params.textureWidth = maxWidth;
    m_params.textureHeight = maxHeight;
    m_params.asciiWidth = 80;
    m_params.asciiHeight = 24;
    m_params.brightness = 0.0f;
    m_params.contrast = 1.0f;
    m_params.charsetSize = 10;
    
    result = UpdateParameters();
    if (result != Result::Success) return result;
    
    return Result::Success;
}

void ASCIIComputePipeline::Shutdown() {
    if (m_inputImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_inputImageView, nullptr);
        m_inputImageView = VK_NULL_HANDLE;
    }
    if (m_charsetMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_charsetMemory, nullptr);
        m_charsetMemory = VK_NULL_HANDLE;
    }
    
    if (m_charsetBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_charsetBuffer, nullptr);
        m_charsetBuffer = VK_NULL_HANDLE;
    }
    
    if (m_parameterMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_parameterMemory, nullptr);
        m_parameterMemory = VK_NULL_HANDLE;
    }
    
    if (m_parameterBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_parameterBuffer, nullptr);
        m_parameterBuffer = VK_NULL_HANDLE;
    }
    
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
}

Result ASCIIComputePipeline::Convert(VkCommandBuffer commandBuffer, VkImage inputTexture, 
                                  uint32_t width, uint32_t height, VkBuffer outputBuffer) {
    Result result = UpdateDescriptorSets(inputTexture, outputBuffer);
    if (result != Result::Success) return result;
    
    // Update parameters
    m_params.textureWidth = width;
    m_params.textureHeight = height;
    result = UpdateParameters();
    if (result != Result::Success) return result;
    
    // Bind pipeline and descriptor sets
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 
                           0, 1, &m_descriptorSets[0], 0, nullptr);
    
    // Dispatch compute work
    uint32_t groupCountX = (m_params.asciiWidth + 15) / 16;
    uint32_t groupCountY = (m_params.asciiHeight + 15) / 16;
    
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    
    return Result::Success;
}

void ASCIIComputePipeline::SetCharset(const std::string& charset) {
    m_charset = charset;
    // Update charset buffer
    std::vector<float32> densities;
    densities.reserve(charset.size());
    
    // Simple density estimation for now
    for (size_t i = 0; i < charset.size(); i++) {
        densities.push_back(1.0f - (float)i / charset.size());
    }
    
    m_charsetDensities = densities;
    m_params.charsetSize = charset.size();
}

void ASCIIComputePipeline::SetBrightness(float32 brightness) {
    m_params.brightness = brightness;
}

void ASCIIComputePipeline::SetContrast(float32 contrast) {
    m_params.contrast = contrast;
}

void ASCIIComputePipeline::SetColorPalette(const ColorPalette& palette) {
    // Color palette would be used for color ASCII conversion
}

void ASCIIComputePipeline::SetAsciiDimensions(uint32_t cols, uint32_t rows) {
    m_params.asciiWidth = cols;
    m_params.asciiHeight = rows;
    UpdateParameters();
}

Result ASCIIComputePipeline::CreateDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    
    // Input texture
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = nullptr;
    
    // Parameters buffer
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].pImmutableSamplers = nullptr;
    
    // Charset buffer
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[2].pImmutableSamplers = nullptr;
    
    // Output buffer
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[3].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result ASCIIComputePipeline::CreatePipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result ASCIIComputePipeline::CreatePipeline() {
    return m_shaderManager->CreateComputePipeline("ascii_convert", m_pipelineLayout, m_pipeline);
}

Result ASCIIComputePipeline::CreateDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 2;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result ASCIIComputePipeline::CreateBuffers() {
    // Create parameter buffer
    VkDeviceSize bufferSize = sizeof(ConversionParams);
    
    Result result = m_context->CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_parameterBuffer,
        m_parameterMemory
    );
    
    if (result != Result::Success) return result;
    
    // Create charset buffer
    bufferSize = sizeof(float32) * MAX_ASCII_CHARS;
    
    result = m_context->CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_charsetBuffer,
        m_charsetMemory
    );
    
    if (result != Result::Success) return result;
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;
    
    m_descriptorSets.resize(1);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    
    return Result::Success;
}

Result ASCIIComputePipeline::UpdateDescriptorSets(VkImage inputTexture, VkBuffer outputBuffer) {
    // Update descriptor sets with current resources
    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    
    // Input texture - Note: We need an image view, not just VkImage
    if (m_inputImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_inputImageView, nullptr);
        m_inputImageView = VK_NULL_HANDLE;
    }
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = inputTexture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_inputImageView) != VK_SUCCESS) {
        return Result::InitializationFailed;
    }
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = m_inputImageView;
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_descriptorSets[0];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;
    
    // Parameters buffer
    VkDescriptorBufferInfo paramInfo{};
    paramInfo.buffer = m_parameterBuffer;
    paramInfo.offset = 0;
    paramInfo.range = sizeof(ConversionParams);
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_descriptorSets[0];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &paramInfo;
    
    // Charset buffer
    VkDescriptorBufferInfo charsetInfo{};
    charsetInfo.buffer = m_charsetBuffer;
    charsetInfo.offset = 0;
    charsetInfo.range = VK_WHOLE_SIZE;
    
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = m_descriptorSets[0];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &charsetInfo;
    
    // Output buffer
    VkDescriptorBufferInfo outputInfo{};
    outputInfo.buffer = outputBuffer;
    outputInfo.offset = 0;
    outputInfo.range = VK_WHOLE_SIZE;
    
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = m_descriptorSets[0];
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &outputInfo;
    
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    
    return Result::Success;
}

Result ASCIIComputePipeline::UpdateParameters() {
    void* data;
    vkMapMemory(m_device, m_parameterMemory, 0, sizeof(ConversionParams), 0, &data);
    memcpy(data, &m_params, sizeof(ConversionParams));
    vkUnmapMemory(m_device, m_parameterMemory);
    
    // Update charset densities
    if (!m_charsetDensities.empty()) {
        vkMapMemory(m_device, m_charsetMemory, 0, m_charsetDensities.size() * sizeof(float32), 0, &data);
        memcpy(data, m_charsetDensities.data(), m_charsetDensities.size() * sizeof(float32));
        vkUnmapMemory(m_device, m_charsetMemory);
    }
    
    return Result::Success;
}

} // namespace NeonGlyph
namespace NeonGlyph {
static std::string GetAsciiConvertShaderSource() {
    return std::string(
        "#version 450\n"
        "layout(local_size_x=16, local_size_y=16, local_size_z=1) in;\n"
        "layout(binding=0, rgba8) uniform readonly image2D inputImage;\n"
        "layout(std140, binding=1) uniform Params { int textureWidth; int textureHeight; int asciiWidth; int asciiHeight; float brightness; float contrast; int charsetSize; };\n"
        "layout(std430, binding=2) buffer Charset { float densities[]; };\n"
        "layout(std430, binding=3) buffer Out { uint outData[]; };\n"
        "void main() {\n"
        "  ivec2 gid = ivec2(gl_GlobalInvocationID.xy);\n"
        "  if (gid.x >= asciiWidth || gid.y >= asciiHeight) return;\n"
        "  uint idx = uint(gid.y * asciiWidth + gid.x);\n"
        "  outData[idx] = 0u;\n"
        "}\n"
    );
}
}