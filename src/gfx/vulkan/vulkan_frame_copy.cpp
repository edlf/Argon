// Argon: gfx::FrameCopy over Vulkan (gfx\vulkan\vulkan_frame_copy.hpp).
#include "gfx/vulkan/vulkan_frame_copy.hpp"

#include <imgui_impl_vulkan.h>

namespace argon::gfx::vulkan
{
namespace
{
const VkImageSubresourceRange kColourImage{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

struct ClearToBlack
{
    VkImage image;

    void operator()(VkCommandBuffer commandBuffer) const
    {
        GpuContext::transition(commandBuffer,
                               image,
                               {VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                0,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT});
        const VkClearColorValue black{{0, 0, 0, 1}};
        vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1, &kColourImage);
        GpuContext::transition(commandBuffer, image, ImageTransition::transferDstToSampled());
    }
};
}  // namespace

bool VulkanFrameCopy::canCopy(const GpuContext &gpu, const Swapchain &swapchain)
{
    VkFormatProperties fp;
    vkGetPhysicalDeviceFormatProperties(gpu.physical, swapchain.format, &fp);
    return swapchain.canCopy && (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
}

bool VulkanFrameCopy::ensure(GpuContext &gpu, const Swapchain &swapchain, VkSampler sampler)
{
    if (!canCopy(gpu, swapchain))
    {
        return false;
    }
    if (sizedFor(swapchain.extent))
    {
        return true;
    }
    gpu_ = &gpu;
    vkDeviceWaitIdle(gpu.device);
    destroy();
    create(swapchain);
    clearToBlack();
    set_ = ImGui_ImplVulkan_AddTexture(sampler, view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    extent_ = swapchain.extent;
    return true;
}

// alpha read as 1: the copy is drawn opaque whatever the swapchain's alpha holds
void VulkanFrameCopy::create(const Swapchain &swapchain)
{
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = swapchain.format;
    ii.extent = {swapchain.extent.width, swapchain.extent.height, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VK_CHECK(vkCreateImage(gpu_->device, &ii, nullptr, &image_));
    memory_ = gpu_->allocateImageMemory(image_);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = image_;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = swapchain.format;
    vi.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_ONE};
    vi.subresourceRange = kColourImage;
    VK_CHECK(vkCreateImageView(gpu_->device, &vi, nullptr, &view_));
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    VK_CHECK(vkCreateImageView(gpu_->device, &vi, nullptr, &arrayView_));
}

void VulkanFrameCopy::clearToBlack()
{
    gpu_->oneShot(ClearToBlack{image_});
}

void VulkanFrameCopy::destroy()
{
    if (set_)
    {
        ImGui_ImplVulkan_RemoveTexture(set_);
    }
    if (view_)
    {
        vkDestroyImageView(gpu_->device, view_, nullptr);
    }
    if (arrayView_)
    {
        vkDestroyImageView(gpu_->device, arrayView_, nullptr);
    }
    if (image_)
    {
        vkDestroyImage(gpu_->device, image_, nullptr);
    }
    if (memory_)
    {
        vkFreeMemory(gpu_->device, memory_, nullptr);
    }
    set_ = VK_NULL_HANDLE;
    view_ = arrayView_ = VK_NULL_HANDLE;
    image_ = VK_NULL_HANDLE;
    memory_ = VK_NULL_HANDLE;
    extent_ = {};
}

void VulkanFrameCopy::copyFrom(VkCommandBuffer commandBuffer, VkImage swapImage, VkImageLayout after) const
{
    GpuContext::transition(commandBuffer, swapImage, ImageTransition::presentToTransferSrc());
    GpuContext::transition(commandBuffer, image_, ImageTransition::sampledToTransferDst());
    VkImageCopy region{};
    region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.dstSubresource = region.srcSubresource;
    region.extent = {extent_.width, extent_.height, 1};
    vkCmdCopyImage(commandBuffer, swapImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    GpuContext::transition(commandBuffer, image_, ImageTransition::transferDstToSampled());
    GpuContext::transition(commandBuffer, swapImage, ImageTransition::transferSrcTo(after));
}

}  // namespace argon::gfx::vulkan
