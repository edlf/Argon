#include "gfx/vulkan/vulkan_swapchain.hpp"
#include <algorithm>

namespace
{

VkAttachmentDescription clearedAttachment(VkFormat format, VkSampleCountFlagBits samples)
{
    VkAttachmentDescription attachment{};
    attachment.format = format;
    attachment.samples = samples;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return attachment;
}

// shared by renderPass and resumePass: dependencies count towards render pass compatibility, and the pipelines made
// against renderPass also draw in resumePass. Covers the Hud Lens copy (transfer) the resumed pass waits for
VkSubpassDependency externalDependency()
{
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    return dep;
}

} // namespace

void Swapchain::create(GpuContext &gpu, GLFWwindow *window, int wantedSamples)
{
    gpu_ = &gpu;
    window_ = window;
    chooseFormat();
    chooseSamples(wantedSamples);
    createRenderPass();
    createResumePass();
    createOitPass();
    createTargets();
}

void Swapchain::recreate()
{
    destroyTargets();
    createTargets();
}

void Swapchain::destroy()
{
    if (!gpu_)
    {
        return;
    }
    destroyTargets();
    vkDestroySwapchainKHR(gpu_->device, handle, nullptr);
    vkDestroyRenderPass(gpu_->device, renderPass, nullptr);
    vkDestroyRenderPass(gpu_->device, resumePass, nullptr);
    vkDestroyRenderPass(gpu_->device, oitPass, nullptr);
}

void Swapchain::chooseFormat()
{
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu_->physical, gpu_->surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu_->physical, gpu_->surface, &count, formats.data());
    format = formats[0].format;
    for (auto &surfaceFormat : formats)
    {
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            format = surfaceFormat.format;
        }
    }
}

// the highest supported sample count <= the requested one (--msaa N)
void Swapchain::chooseSamples(int wanted)
{
    const VkPhysicalDeviceLimits &limits = gpu_->properties.limits;
    const VkSampleCountFlags supported = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
    samples = VK_SAMPLE_COUNT_1_BIT;
    for (int sampleCount : {8, 4, 2})
    {
        if (sampleCount <= wanted && (supported & VkSampleCountFlags(sampleCount)))
        {
            samples = VkSampleCountFlagBits(sampleCount);
            return;
        }
    }
}

// multisampled: attachment 0 = MSAA colour, 2 = the swapchain image (resolve target). Colour and depth are stored, so
// resumePass can go on where a pass ended (SceneRecorder::drawHudLenses)
void Swapchain::sceneAttachments(VkAttachmentDescription att[3]) const
{
    att[0] = clearedAttachment(format, samples);
    att[1] = clearedAttachment(kDepthFormat, samples);
    att[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (multisampled())
    {
        att[2] = clearedAttachment(format, VK_SAMPLE_COUNT_1_BIT);
        att[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
}

void Swapchain::createRenderPass()
{
    VkAttachmentDescription att[3]{};
    sceneAttachments(att);
    renderPass = createPass(att);
}

// the swapchain image comes back from a copy in COLOR_ATTACHMENT_OPTIMAL, the MSAA colour and depth as stored
void Swapchain::createResumePass()
{
    VkAttachmentDescription att[3]{};
    sceneAttachments(att);
    att[0].loadOp = att[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    att[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    att[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (multisampled())
    {
        att[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    resumePass = createPass(att);
}

// attachments 0 the accumulated colour, 1 the revealage (both cleared, then read by the composite), 2 the scene's depth
// as the scene pass stored it (tested, not written). Before: the scene's depth writes and the last composite's reads of
// the targets; after: the composite's reads
void Swapchain::createOitPass()
{
    VkAttachmentDescription att[3]{};
    att[0] = clearedAttachment(kOitAccumulatedFormat, samples);
    att[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    att[1] = clearedAttachment(kOitRevealageFormat, samples);
    att[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    att[2] = clearedAttachment(kDepthFormat, samples);
    att[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    att[2].initialLayout = att[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    const VkAttachmentReference colourRefs[2] = {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
    const VkAttachmentReference depthRef{2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 2;
    sp.pColorAttachments = colourRefs;
    sp.pDepthStencilAttachment = &depthRef;
    VkSubpassDependency deps[2]{};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 3;
    rp.pAttachments = att;
    rp.subpassCount = 1;
    rp.pSubpasses = &sp;
    rp.dependencyCount = 2;
    rp.pDependencies = deps;
    VK_CHECK(vkCreateRenderPass(gpu_->device, &rp, nullptr, &oitPass));
}

void Swapchain::keepOitTargets(bool keep)
{
    if (keep == oitKept_)
    {
        return;
    }
    oitKept_ = keep;
    if (keep)
    {
        createOitTargets();
        return;
    }
    destroyOitTargets();
}

void Swapchain::createOitTargets()
{
    const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    oitAccumulated_ = createTarget(kOitAccumulatedFormat, usage, VK_IMAGE_ASPECT_COLOR_BIT);
    oitRevealage_ = createTarget(kOitRevealageFormat, usage, VK_IMAGE_ASPECT_COLOR_BIT);
    const VkImageView attachments[3] = {oitAccumulated_.view, oitRevealage_.view, depth_.view};
    VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fi.renderPass = oitPass;
    fi.attachmentCount = 3;
    fi.pAttachments = attachments;
    fi.width = extent.width;
    fi.height = extent.height;
    fi.layers = 1;
    VK_CHECK(vkCreateFramebuffer(gpu_->device, &fi, nullptr, &oitFramebuffer));
}

void Swapchain::destroyOitTargets()
{
    if (oitFramebuffer)
    {
        vkDestroyFramebuffer(gpu_->device, oitFramebuffer, nullptr);
        oitFramebuffer = VK_NULL_HANDLE;
    }
    destroyTarget(oitAccumulated_);
    destroyTarget(oitRevealage_);
}

VkRenderPass Swapchain::createPass(const VkAttachmentDescription att[3]) const
{
    const VkAttachmentReference colourRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    const VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    const VkAttachmentReference resolveRef{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments = &colourRef;
    sp.pDepthStencilAttachment = &depthRef;
    if (multisampled())
    {
        sp.pResolveAttachments = &resolveRef;
    }
    const VkSubpassDependency dep = externalDependency();
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = multisampled() ? 3 : 2;
    rp.pAttachments = att;
    rp.subpassCount = 1;
    rp.pSubpasses = &sp;
    rp.dependencyCount = 1;
    rp.pDependencies = &dep;
    VkRenderPass pass;
    VK_CHECK(vkCreateRenderPass(gpu_->device, &rp, nullptr, &pass));
    return pass;
}

void Swapchain::createTargets()
{
    createHandle();
    fetchImages();
    depth_ = createTarget(kDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    if (multisampled())   // the multisampled colour target, resolved at the end of the pass
    {
        msaaColour_ = createTarget(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    createFramebuffers();
    if (oitKept_)
    {
        createOitTargets();
    }
}

void Swapchain::createHandle()
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_->physical, gpu_->surface, &caps);
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    extent = caps.currentExtent.width != UINT32_MAX ? caps.currentExtent : VkExtent2D{uint32_t(w), uint32_t(h)};
    minImageCount = std::max(2u, caps.minImageCount);
    uint32_t count = caps.minImageCount + 1;
    if (caps.maxImageCount && count > caps.maxImageCount)
    {
        count = caps.maxImageCount;
    }
    canCopy = (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = gpu_->surface;
    sci.minImageCount = count;
    sci.imageFormat = format;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = choosePresentMode();
    sci.clipped = VK_TRUE;
    const VkSwapchainKHR old = handle;
    sci.oldSwapchain = old;
    VK_CHECK(vkCreateSwapchainKHR(gpu_->device, &sci, nullptr, &handle));
    if (old)
    {
        vkDestroySwapchainKHR(gpu_->device, old, nullptr);
    }
}

VkPresentModeKHR Swapchain::choosePresentMode() const
{
    if (vsync)
    {
        return VK_PRESENT_MODE_FIFO_KHR;   // the one every surface has
    }
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_->physical, gpu_->surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_->physical, gpu_->surface, &count, modes.data());
    for (const VkPresentModeKHR unsynced : {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR})
    {
        if (std::find(modes.begin(), modes.end(), unsynced) != modes.end())
        {
            return unsynced;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

void Swapchain::fetchImages()
{
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(gpu_->device, handle, &count, nullptr);
    images.resize(count);
    vkGetSwapchainImagesKHR(gpu_->device, handle, &count, images.data());
}

Swapchain::ImageTarget Swapchain::createTarget(VkFormat targetFormat, VkImageUsageFlags usage, VkImageAspectFlags aspect) const
{
    ImageTarget target;
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = targetFormat;
    ii.extent = {extent.width, extent.height, 1};
    ii.mipLevels = ii.arrayLayers = 1;
    ii.samples = samples;
    ii.usage = usage;
    VK_CHECK(vkCreateImage(gpu_->device, &ii, nullptr, &target.image));
    target.memory = gpu_->allocateImageMemory(target.image);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = target.image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = targetFormat;
    vi.subresourceRange = {aspect, 0, 1, 0, 1};
    VK_CHECK(vkCreateImageView(gpu_->device, &vi, nullptr, &target.view));
    return target;
}

void Swapchain::destroyTarget(ImageTarget &target) const
{
    if (target.view)
    {
        vkDestroyImageView(gpu_->device, target.view, nullptr);
    }
    if (target.image)
    {
        vkDestroyImage(gpu_->device, target.image, nullptr);
    }
    if (target.memory)
    {
        vkFreeMemory(gpu_->device, target.memory, nullptr);
    }
    target = {};
}

void Swapchain::createFramebuffers()
{
    for (VkImage image : images)
    {
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = image;
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = format;
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkImageView view;
        VK_CHECK(vkCreateImageView(gpu_->device, &vi, nullptr, &view));
        views_.push_back(view);
        const VkImageView attachments[3] = {multisampled() ? msaaColour_.view : view, depth_.view, view};
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass = renderPass;
        fi.attachmentCount = multisampled() ? 3 : 2;
        fi.pAttachments = attachments;
        fi.width = extent.width;
        fi.height = extent.height;
        fi.layers = 1;
        VkFramebuffer fb;
        VK_CHECK(vkCreateFramebuffer(gpu_->device, &fi, nullptr, &fb));
        framebuffers.push_back(fb);
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore done;
        VK_CHECK(vkCreateSemaphore(gpu_->device, &si, nullptr, &done));
        renderDone.push_back(done);
    }
}

void Swapchain::destroyTargets()
{
    for (auto fb : framebuffers)
    {
        vkDestroyFramebuffer(gpu_->device, fb, nullptr);
    }
    for (auto view : views_)
    {
        vkDestroyImageView(gpu_->device, view, nullptr);
    }
    for (auto semaphore : renderDone)
    {
        vkDestroySemaphore(gpu_->device, semaphore, nullptr);
    }
    framebuffers.clear();
    views_.clear();
    renderDone.clear();
    destroyOitTargets();
    destroyTarget(depth_);
    destroyTarget(msaaColour_);
}
