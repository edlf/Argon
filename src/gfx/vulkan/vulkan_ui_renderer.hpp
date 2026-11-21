// Argon: gfx::UiRenderer over ImGui's Vulkan backend: its descriptor pool (the fonts and the images), the images'
// 2D views of their textures' first layers, and the HUD's three blend pipelines over ImGui's pipeline layout (made at
// their first use).
#pragma once
#include "gfx/ui_renderer.hpp"
#include "gfx/vulkan/vulkan_device.hpp"
#include "gfx/vulkan/vulkan_presenter.hpp"

#include <unordered_map>

namespace argon::gfx::vulkan
{

class VulkanUiRenderer final : public UiRenderer
{
  public:
    // ImGui's context and platform backend made first
    void create(VulkanDevice &device, VulkanPresenter &presenter, const Swapchain &swapchain);
    // every image removed first
    void destroy();

    void newFrame() override;
    void render() override;
    ImTextureID addImage(const Texture &texture, SamplerMode sampling) override;
    ImTextureID addLayeredImage(const Texture &texture, SamplerMode sampling) override;
    void removeImage(ImTextureID image) override;
    void useBlend(UiBlend blend) override;
    void windowRemade() override;

  private:
    VkPipeline createBlendPipeline(UiBlend blend, VkPipelineLayout layout) const;

    VulkanDevice *device_ = nullptr;
    VulkanPresenter *presenter_ = nullptr;
    const Swapchain *swapchain_ = nullptr;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    std::unordered_map<ImTextureID, VkImageView> firstLayerViews_;   // the addImage views, by image
    VkPipeline blendPipelines_[3]{};                                 // by UiBlend
};

}  // namespace argon::gfx::vulkan
