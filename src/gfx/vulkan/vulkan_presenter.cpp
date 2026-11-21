// Argon: gfx::Presenter over the Vulkan swapchain (gfx\vulkan\vulkan_presenter.hpp).
#include "gfx/vulkan/vulkan_presenter.hpp"

#include <GLFW/glfw3.h>
#include <cstring>

namespace argon::gfx::vulkan
{
namespace
{
struct IsCopy
{
    const VulkanFrameCopy *copy;

    bool operator()(const std::unique_ptr<VulkanFrameCopy> &kept) const
    {
        return kept.get() == copy;
    }
};
}  // namespace

void VulkanPresenter::create(VulkanDevice &device, Swapchain &swapchain, GLFWwindow *window)
{
    device_ = &device;
    gpu_ = &device.gpu();
    swapchain_ = &swapchain;
    window_ = window;
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ca.commandPool = gpu_->cmdPool;
    ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ca.commandBufferCount = kFramesInFlight;
    VK_CHECK(vkAllocateCommandBuffers(gpu_->device, &ca, cmd_));
    for (int i = 0; i < kFramesInFlight; ++i)
    {
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(gpu_->device, &semaphoreInfo, nullptr, &imageReady_[i]));
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(gpu_->device, &fenceInfo, nullptr, &inFlight_[i]));
    }
}

void VulkanPresenter::destroy()
{
    if (!gpu_)
    {
        return;
    }
    gpu_->destroyBuffer(readBack_);
    for (int i = 0; i < kFramesInFlight; ++i)
    {
        vkDestroySemaphore(gpu_->device, imageReady_[i], nullptr);
        vkDestroyFence(gpu_->device, inFlight_[i], nullptr);
    }
}

bool VulkanPresenter::beginFrame()
{
    vkWaitForFences(gpu_->device, 1, &inFlight_[slot_], VK_TRUE, UINT64_MAX);
    keptSlotReused_ = keptSlotReused_ || (pictureKept_ && slot_ == keptSlot_);
    const VkResult acquired = vkAcquireNextImageKHR(gpu_->device, swapchain_->handle, UINT64_MAX, imageReady_[slot_], VK_NULL_HANDLE, &imageIndex_);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR)
    {
        remakeSwapchain();
        return false;
    }
    if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR)
    {
        VK_CHECK(acquired);
    }
    vkResetFences(gpu_->device, 1, &inFlight_[slot_]);
    vkResetCommandBuffer(cmd_[slot_], 0);
    const VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_CHECK(vkBeginCommandBuffer(cmd_[slot_], &bi));
    commands_.emplace(cmd_[slot_], *device_);
    return true;
}

CommandList &VulkanPresenter::commands()
{
    return *commands_;
}

void VulkanPresenter::beginScenePass(const SceneClear &clear)
{
    VkClearValue values[2];
    values[0].color = {{clear.colour.r, clear.colour.g, clear.colour.b, clear.colour.a}};
    values[1].depthStencil = {clear.depth, 0};
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = swapchain_->renderPass;
    rp.framebuffer = swapchain_->framebuffers[imageIndex_];
    rp.renderArea.extent = swapchain_->extent;
    rp.clearValueCount = 2;
    rp.pClearValues = values;
    vkCmdBeginRenderPass(cmd_[slot_], &rp, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanPresenter::endScenePass()
{
    vkCmdEndRenderPass(cmd_[slot_]);
}

void VulkanPresenter::endFrame(FrameEnd end)
{
    if (end == FrameEnd::ShowAndKeep)
    {
        recordReadBack();
    }
    VK_CHECK(vkEndCommandBuffer(cmd_[slot_]));
    commands_.reset();
    submit();
    present();
    if (end == FrameEnd::ShowAndKeep)
    {
        pictureKept_ = true;
        keptSlot_ = slot_;
        keptSlotReused_ = false;
    }
    slot_ = (slot_ + 1) % kFramesInFlight;
}

// the finished image copied into the read-back buffer (made again when the window's size changed)
void VulkanPresenter::recordReadBack()
{
    const VkExtent2D extent = swapchain_->extent;
    if (!readBack_.buf || readBackExtent_.width != extent.width || readBackExtent_.height != extent.height)
    {
        vkDeviceWaitIdle(gpu_->device);
        gpu_->destroyBuffer(readBack_);
        readBack_ = gpu_->createReadBackBuffer(VkDeviceSize(extent.width) * extent.height * 4);
        readBackExtent_ = extent;
    }
    readBackBgra_ = swapchain_->format == VK_FORMAT_B8G8R8A8_UNORM || swapchain_->format == VK_FORMAT_B8G8R8A8_SRGB;
    const VkImage image = swapchain_->images[imageIndex_];
    GpuContext::transition(cmd_[slot_], image, ImageTransition::presentToTransferSrc());
    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {swapchain_->extent.width, swapchain_->extent.height, 1};
    vkCmdCopyImageToBuffer(cmd_[slot_], image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readBack_.buf, 1, &region);
    GpuContext::transition(cmd_[slot_], image, ImageTransition::transferSrcTo(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
}

void VulkanPresenter::submit()
{
    const VkPipelineStageFlags wait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &imageReady_[slot_];
    si.pWaitDstStageMask = &wait;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd_[slot_];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &swapchain_->renderDone[imageIndex_];
    VK_CHECK(vkQueueSubmit(gpu_->queue, 1, &si, inFlight_[slot_]));
    ++submitted_;
}

void VulkanPresenter::present()
{
    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &swapchain_->renderDone[imageIndex_];
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain_->handle;
    pi.pImageIndices = &imageIndex_;
    const VkResult presented = vkQueuePresentKHR(gpu_->queue, &pi);
    if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR)
    {
        remakeSwapchain();
    }
    else if (presented != VK_SUCCESS)
    {
        VK_CHECK(presented);
    }
}

// a minimised window waits until it has a size again
void VulkanPresenter::remakeSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &width, &height);
    }
    vkDeviceWaitIdle(gpu_->device);
    swapchain_->recreate();
    remade_ = true;
    if (oitKept_)
    {
        ++oitMade_;
    }
}

void VulkanPresenter::setVsync(bool on)
{
    if (swapchain_->vsync == on)
    {
        return;
    }
    swapchain_->vsync = on;
    remakeSwapchain();
}

bool VulkanPresenter::takeRemade()
{
    const bool remade = remade_;
    remade_ = false;
    return remade;
}

void VulkanPresenter::waitForSubmittedWork()
{
    vkWaitForFences(gpu_->device, kFramesInFlight, inFlight_, VK_TRUE, UINT64_MAX);
    gpu_->waitForUploads();
}

FrameCopy VulkanPresenter::createFrameCopy()
{
    return frameCopyOf(*copies_.emplace_back(std::make_unique<VulkanFrameCopy>()));
}

void VulkanPresenter::destroyFrameCopy(FrameCopy &copy)
{
    VulkanFrameCopy &record = recordOf(copy);
    record.destroy();
    std::erase_if(copies_, IsCopy{&record});
    copy = {};
}

bool VulkanPresenter::fitFrameCopy(FrameCopy copy)
{
    return recordOf(copy).ensure(*gpu_, *swapchain_, device_->sampler(SamplerMode::Clamp));
}

bool VulkanPresenter::frameCopyFits(FrameCopy copy) const
{
    return recordOf(copy).sizedFor(swapchain_->extent);
}

TextureRef VulkanPresenter::frameCopyTexture(FrameCopy copy)
{
    return device_->adoptView(recordOf(copy).arrayView());
}

ImTextureID VulkanPresenter::frameCopyImage(FrameCopy copy) const
{
    return ImTextureID(recordOf(copy).imguiTexture());
}

// the scene pass ended and the window's image copied (left a colour attachment for resumeScenePass)
void VulkanPresenter::suspendScenePass(FrameCopy copy)
{
    vkCmdEndRenderPass(cmd_[slot_]);
    recordOf(copy).copyFrom(cmd_[slot_], swapchain_->images[imageIndex_], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

// the pass begun again on what it left (Swapchain::resumePass)
void VulkanPresenter::resumeScenePass()
{
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = swapchain_->resumePass;
    rp.framebuffer = swapchain_->framebuffers[imageIndex_];
    rp.renderArea.extent = swapchain_->extent;
    vkCmdBeginRenderPass(cmd_[slot_], &rp, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanPresenter::copySceneAndRestart(FrameCopy copy, const SceneClear &clear)
{
    vkCmdEndRenderPass(cmd_[slot_]);
    recordOf(copy).copyFrom(cmd_[slot_], swapchain_->images[imageIndex_], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    beginScenePass(clear);
}

void VulkanPresenter::copyFinishedFrame(FrameCopy copy)
{
    recordOf(copy).copyFrom(cmd_[slot_], swapchain_->images[imageIndex_], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void VulkanPresenter::keepOitTargets(bool keep)
{
    if (keep == oitKept_)
    {
        return;
    }
    vkDeviceWaitIdle(gpu_->device);
    swapchain_->keepOitTargets(keep);
    oitKept_ = keep;
    ++oitMade_;
}

// the scene pass ended (its image left as resumePass takes it), the OIT pass begun: accumulated colour cleared to 0,
// revealage to 1, the scene's depth loaded (Swapchain::oitPass)
void VulkanPresenter::beginOitPass()
{
    vkCmdEndRenderPass(cmd_[slot_]);
    GpuContext::transition(cmd_[slot_], swapchain_->images[imageIndex_], ImageTransition::presentToColourAttachment());
    VkClearValue values[2];
    values[0].color = {{0.f, 0.f, 0.f, 0.f}};
    values[1].color = {{1.f, 1.f, 1.f, 1.f}};
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass = swapchain_->oitPass;
    rp.framebuffer = swapchain_->oitFramebuffer;
    rp.renderArea.extent = swapchain_->extent;
    rp.clearValueCount = 2;
    rp.pClearValues = values;
    vkCmdBeginRenderPass(cmd_[slot_], &rp, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanPresenter::endOitPass()
{
    vkCmdEndRenderPass(cmd_[slot_]);
    resumeScenePass();
}

OitTargets VulkanPresenter::oitTargets()
{
    return OitTargets{device_->adoptView(swapchain_->oitAccumulatedView()), device_->adoptView(swapchain_->oitRevealageView())};
}

bool VulkanPresenter::pictureReady() const
{
    return !pictureKept_ || (keptSlotReused_ || vkGetFenceStatus(gpu_->device, inFlight_[keptSlot_]) == VK_SUCCESS);
}

// the read-back buffer as it is (BGRA or RGBA as the swapchain's format has it)
std::optional<Picture> VulkanPresenter::takePicture()
{
    if (!pictureKept_)
    {
        return std::nullopt;
    }
    if (!keptSlotReused_)
    {
        vkWaitForFences(gpu_->device, 1, &inFlight_[keptSlot_], VK_TRUE, UINT64_MAX);
    }
    pictureKept_ = false;
    Picture picture;
    picture.width = readBackExtent_.width;
    picture.height = readBackExtent_.height;
    picture.bgra = readBackBgra_;
    picture.pixels.resize(size_t(picture.width) * picture.height * 4);
    void *mapped = nullptr;
    VK_CHECK(vkMapMemory(gpu_->device, readBack_.mem, 0, VK_WHOLE_SIZE, 0, &mapped));
    const VkMappedMemoryRange whole{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, readBack_.mem, 0, VK_WHOLE_SIZE};
    VK_CHECK(vkInvalidateMappedMemoryRanges(gpu_->device, 1, &whole));
    std::memcpy(picture.pixels.data(), mapped, picture.pixels.size());
    vkUnmapMemory(gpu_->device, readBack_.mem);
    return picture;
}

}  // namespace argon::gfx::vulkan
