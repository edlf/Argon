// The window's swapchain and the scene's render pass: MSAA colour + depth, resolved into the swapchain image (ImGui
// draws into the same pass).
#pragma once
#include "gfx/vulkan/vulkan_gpu_context.hpp"

class Swapchain
{
  public:
    static constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

    VkSwapchainKHR handle = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D extent{};
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkRenderPass resumePass = VK_NULL_HANDLE;   // renderPass again on what it left (no clears): the Hud Lens copy
    // the order-independent translucency's pass: its two targets cleared, the scene's depth tested (gfx::Presenter::beginOitPass)
    VkRenderPass oitPass = VK_NULL_HANDLE;
    VkFramebuffer oitFramebuffer = VK_NULL_HANDLE;   // while the OIT targets are kept
    std::vector<VkImage> images;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> renderDone;   // per swapchain image
    uint32_t minImageCount = 2;
    bool canCopy = false;                  // the images allow transfer reads (screenshots, the menu feedback)
    bool vsync = true;                     // FIFO; off: IMMEDIATE, else MAILBOX, where the surface has them (recreate after a change)

    void create(GpuContext &gpu, GLFWwindow *window, int wantedSamples);
    void recreate();   // after a resize (the device idle)
    void destroy();
    bool multisampled() const
    {
        return samples != VK_SAMPLE_COUNT_1_BIT;
    }
    // the OIT targets made or freed (the device idle); kept ones are made again with the other targets
    void keepOitTargets(bool keep);
    VkImageView oitAccumulatedView() const
    {
        return oitAccumulated_.view;
    }
    VkImageView oitRevealageView() const
    {
        return oitRevealage_.view;
    }

    static constexpr VkFormat kOitAccumulatedFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    static constexpr VkFormat kOitRevealageFormat = VK_FORMAT_R16_SFLOAT;

  private:
    struct ImageTarget
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    void chooseFormat();
    VkPresentModeKHR choosePresentMode() const;
    void chooseSamples(int wanted);
    void createRenderPass();
    void createResumePass();
    void createOitPass();
    void createOitTargets();
    void destroyOitTargets();
    void sceneAttachments(VkAttachmentDescription att[3]) const;
    VkRenderPass createPass(const VkAttachmentDescription att[3]) const;
    void createTargets();
    void createHandle();
    void fetchImages();
    ImageTarget createTarget(VkFormat targetFormat, VkImageUsageFlags usage, VkImageAspectFlags aspect) const;
    void destroyTarget(ImageTarget &target) const;
    void createFramebuffers();
    void destroyTargets();

    GpuContext *gpu_ = nullptr;
    GLFWwindow *window_ = nullptr;
    std::vector<VkImageView> views_;
    ImageTarget depth_, msaaColour_;
    ImageTarget oitAccumulated_, oitRevealage_;
    bool oitKept_ = false;
};
