// Argon: gfx::Backend over Vulkan: the GPU context, the window's swapchain, and the Vulkan device, presenter and UI
// renderer made over them.
#pragma once
#include "gfx/backend.hpp"
#include "gfx/vulkan/vulkan_device.hpp"
#include "gfx/vulkan/vulkan_presenter.hpp"
#include "gfx/vulkan/vulkan_ui_renderer.hpp"

namespace argon::gfx::vulkan
{

class VulkanBackend final : public Backend
{
  public:
    VulkanBackend(const BackendOptions &options, GLFWwindow *window, uint32_t pushConstantBytes);

    Device &device() override
    {
        return device_;
    }
    Presenter &presenter() override
    {
        return presenter_;
    }
    UiRenderer &ui() override
    {
        return ui_;
    }
    void startUi() override;
    void stopUi() override;
    void destroy() override;

  private:
    GLFWwindow *window_;
    GpuContext gpu_;
    Swapchain swapchain_;
    VulkanDevice device_;
    VulkanPresenter presenter_;
    VulkanUiRenderer ui_;
};

}  // namespace argon::gfx::vulkan
