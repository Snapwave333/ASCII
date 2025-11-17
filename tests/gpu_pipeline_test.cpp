#include "NeonGlyph.h"
#include "VulkanContext.h"
#include "ASCIIConverter.h"
#include "ConfigManager.h"
#include <iostream>

using namespace NeonGlyph;

static VkImage CreateAndClearImage(VulkanContext* vc, uint32_t w, uint32_t h, VkDeviceMemory& mem) {
    VkImage img = VK_NULL_HANDLE;
    Result r = vc->CreateImage(w, h, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, img, mem);
    if (r != Result::Success) return VK_NULL_HANDLE;
    VkDevice dev = vc->GetDevice();
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo ai{}; ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = vc->GetCommandPool(); ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(dev, &ai, &cmd) != VK_SUCCESS) return VK_NULL_HANDLE;
    VkCommandBufferBeginInfo bi{}; bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    VkImageMemoryBarrier b{}; b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; b.srcAccessMask = 0; b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img; b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; b.subresourceRange.baseMipLevel = 0; b.subresourceRange.levelCount = 1; b.subresourceRange.baseArrayLayer = 0; b.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    VkClearColorValue clear{}; clear.float32[0] = 0.25f; clear.float32[1] = 0.5f; clear.float32[2] = 0.75f; clear.float32[3] = 1.0f;
    VkImageSubresourceRange range{}; range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; range.baseMipLevel = 0; range.levelCount = 1; range.baseArrayLayer = 0; range.layerCount = 1;
    vkCmdClearColorImage(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
    VkImageMemoryBarrier b2{}; b2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    b2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b2.newLayout = VK_IMAGE_LAYOUT_GENERAL; b2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b2.image = img; b2.subresourceRange = range;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b2);
    vkEndCommandBuffer(cmd);
    vc->SubmitComputeWork(cmd);
    return img;
}

int main() {
    ConfigManager cm; Config cfg; if (cm.LoadDefaultConfig(cfg) != Result::Success) { std::cout << "FAIL: config" << std::endl; return 1; }
    VulkanContext vc; if (vc.Initialize(cfg) != Result::Success) { std::cout << "SKIP: vulkan init" << std::endl; return 2; }
    ASCIIConverter conv; if (conv.Initialize(&vc, cfg) != Result::Success) { std::cout << "SKIP: ascii init" << std::endl; return 2; }
    VkDeviceMemory mem = VK_NULL_HANDLE; VkImage img = CreateAndClearImage(&vc, 640, 360, mem);
    if (img == VK_NULL_HANDLE) { std::cout << "SKIP: image" << std::endl; return 2; }
    uint32_t outSize = 0; Result r = conv.ConvertFrameGPU(img, 640, 360, VK_NULL_HANDLE, outSize);
    bool ok = (r == Result::Success) && (outSize > 0);
    std::cout << (ok ? "PASS" : "FAIL") << ": gpu pipeline convert" << std::endl;
    return ok ? 0 : 1;
}