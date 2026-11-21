// Argon: a gfx::LayerTarget over Vulkan: an RGBA8 2D array image, a view of all its layers (sampled through
// texture sets) and per layer a view and a framebuffer of the layer pass. The handle holds the record's address.
#pragma once
#include "gfx/gfx_types.hpp"
#include "gfx/vulkan/vulkan_gpu_context.hpp"

#include <vector>

namespace argon::gfx::vulkan
{

struct VulkanLayerTarget
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView arrayView = VK_NULL_HANDLE;
    std::vector<VkImageView> layerViews;
    std::vector<VkFramebuffer> framebuffers;
    VkRenderPass pass = VK_NULL_HANDLE;
    uint32_t width = 0, height = 0;
};

// one colour attachment cleared on load and left readable by the fragment shaders (the RenderTarget::Layer pipelines)
VkRenderPass createLayerPass(VkDevice device);
// every layer black and readable from the start (layers never drawn stay black)
void createLayerTarget(GpuContext &gpu, const LayerTargetSize &size, VulkanLayerTarget &target);
void destroyLayerTarget(VkDevice device, VulkanLayerTarget &target);

inline LayerTarget layerTargetOf(const VulkanLayerTarget &target)
{
    return LayerTarget{reinterpret_cast<uint64_t>(&target)};
}
inline const VulkanLayerTarget &recordOf(LayerTarget target)
{
    return *reinterpret_cast<const VulkanLayerTarget *>(target.value);
}

}  // namespace argon::gfx::vulkan
