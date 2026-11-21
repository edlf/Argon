// Argon: a gfx::FrameCopy over Vulkan: a window-sized image in the swapchain's format the swapchain image is copied
// into, with a 2D view (ImGui's, for now) and a 1-layer 2D array view (mesh_shading.glsl's sampler2DArray). The handle holds
// the copy's address (VulkanPresenter keeps them).
#pragma once
#include "gfx/gfx_types.hpp"
#include "gfx/vulkan/vulkan_swapchain.hpp"

namespace argon::gfx::vulkan
{

class VulkanFrameCopy
{
  public:
    // (re)created at the swapchain's size, black; false when the swapchain images cannot be copied and sampled
    bool ensure(GpuContext &gpu, const Swapchain &swapchain, VkSampler sampler);
    void destroy();
    bool sizedFor(VkExtent2D extent) const
    {
        return image_ != VK_NULL_HANDLE && extent_.width == extent.width && extent_.height == extent.height;
    }
    VkDescriptorSet imguiTexture() const
    {
        return set_;
    }
    VkImageView arrayView() const
    {
        return arrayView_;
    }
    // the swapchain image (in PRESENT_SRC) copied in; it is left in `after`: PRESENT_SRC, or COLOR_ATTACHMENT_OPTIMAL
    // for a render pass that goes on
    void copyFrom(VkCommandBuffer commandBuffer, VkImage swapImage, VkImageLayout after) const;

  private:
    static bool canCopy(const GpuContext &gpu, const Swapchain &swapchain);
    void create(const Swapchain &swapchain);
    void clearToBlack();

    GpuContext *gpu_ = nullptr;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE, arrayView_ = VK_NULL_HANDLE;
    VkDescriptorSet set_ = VK_NULL_HANDLE;
    VkExtent2D extent_{};
};

inline FrameCopy frameCopyOf(const VulkanFrameCopy &copy)
{
    return FrameCopy{reinterpret_cast<uint64_t>(&copy)};
}
inline VulkanFrameCopy &recordOf(FrameCopy copy)
{
    return *reinterpret_cast<VulkanFrameCopy *>(copy.value);
}

}  // namespace argon::gfx::vulkan
