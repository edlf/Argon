// Argon: gfx::Presenter over the Vulkan swapchain: a command buffer, an "image ready" semaphore and an "in flight"
// fence per frame slot, the acquire / submit / present, the swapchain made again on a resize, and the screenshot's
// read-back.
#pragma once
#include "gfx/presenter.hpp"
#include "gfx/vulkan/vulkan_command_list.hpp"
#include "gfx/vulkan/vulkan_device.hpp"
#include "gfx/vulkan/vulkan_frame_copy.hpp"

#include <memory>
#include <optional>
#include <vector>

struct GLFWwindow;

namespace argon::gfx::vulkan
{

class VulkanPresenter final : public Presenter
{
  public:
    void create(VulkanDevice &device, Swapchain &swapchain, GLFWwindow *window);
    void destroy();

    bool beginFrame() override;
    CommandList &commands() override;
    void beginScenePass(const SceneClear &clear) override;
    void endScenePass() override;
    void endFrame(FrameEnd end) override;
    bool pictureReady() const override;
    std::optional<Picture> takePicture() override;
    FrameCopy createFrameCopy() override;
    void destroyFrameCopy(FrameCopy &copy) override;
    bool fitFrameCopy(FrameCopy copy) override;
    bool frameCopyFits(FrameCopy copy) const override;
    TextureRef frameCopyTexture(FrameCopy copy) override;
    ImTextureID frameCopyImage(FrameCopy copy) const override;
    void suspendScenePass(FrameCopy copy) override;
    void resumeScenePass() override;
    void copySceneAndRestart(FrameCopy copy, const SceneClear &clear) override;
    void copyFinishedFrame(FrameCopy copy) override;
    void keepOitTargets(bool keep) override;
    void beginOitPass() override;
    void endOitPass() override;
    OitTargets oitTargets() override;
    uint64_t oitTargetsMade() const override
    {
        return oitMade_;
    }
    WindowSize windowSize() const override
    {
        return WindowSize{swapchain_->extent.width, swapchain_->extent.height};
    }
    uint32_t samples() const override
    {
        return uint32_t(swapchain_->samples);
    }
    int frameSlot() const override
    {
        return slot_;
    }
    uint64_t framesSubmitted() const override
    {
        return submitted_;
    }
    void waitForSubmittedWork() override;
    bool takeRemade() override;
    void setVsync(bool on) override;

    // for ImGui's Vulkan backend (VulkanUiRenderer)
    VkCommandBuffer commandBuffer() const
    {
        return cmd_[slot_];
    }

  private:
    void remakeSwapchain();
    void recordReadBack();
    void submit();
    void present();

    VulkanDevice *device_ = nullptr;
    GpuContext *gpu_ = nullptr;
    Swapchain *swapchain_ = nullptr;
    GLFWwindow *window_ = nullptr;
    VkCommandBuffer cmd_[kFramesInFlight]{};
    VkSemaphore imageReady_[kFramesInFlight]{};
    VkFence inFlight_[kFramesInFlight]{};
    std::optional<VulkanCommandList> commands_;
    int slot_ = 0;
    uint32_t imageIndex_ = 0;
    uint64_t submitted_ = 0;
    bool remade_ = false;
    Buffer readBack_;          // the kept frame's pixels (host visible, cached where the GPU allows)
    VkExtent2D readBackExtent_{};
    bool readBackBgra_ = false;
    bool pictureKept_ = false;
    int keptSlot_ = 0;
    bool keptSlotReused_ = false;   // beginFrame waited on the kept frame's fence and reset it
    std::vector<std::unique_ptr<VulkanFrameCopy>> copies_;   // by address (their handles)
    bool oitKept_ = false;
    uint64_t oitMade_ = 0;   // oitTargetsMade
};

}  // namespace argon::gfx::vulkan
