// Argon: gfx::LayerTarget over Vulkan (gfx\vulkan\vulkan_layer_target.hpp).
#include "gfx/vulkan/vulkan_layer_target.hpp"

namespace argon::gfx::vulkan
{
namespace
{
const VkFormat kLayerFormat = VK_FORMAT_R8G8B8A8_UNORM;

// the image's layers from undefined to shader-readable
struct MakeLayersReadable
{
    VkImage image;
    uint32_t layers;

    void operator()(VkCommandBuffer commands) const
    {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers};
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);
    }
};

VkImageView createView(VkDevice device, const VkImageViewCreateInfo &info)
{
    VkImageView view;
    VK_CHECK(vkCreateImageView(device, &info, nullptr, &view));
    return view;
}

void createLayerFramebuffers(VkDevice device, VulkanLayerTarget &target)
{
    const uint32_t layers = uint32_t(target.layerViews.size());
    target.framebuffers.resize(layers);
    for (uint32_t i = 0; i < layers; ++i)
    {
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass = target.pass;
        fi.attachmentCount = 1;
        fi.pAttachments = &target.layerViews[i];
        fi.width = target.width;
        fi.height = target.height;
        fi.layers = 1;
        VK_CHECK(vkCreateFramebuffer(device, &fi, nullptr, &target.framebuffers[i]));
    }
}
}  // namespace

VkRenderPass createLayerPass(VkDevice device)
{
    VkAttachmentDescription attachment{};
    attachment.format = kLayerFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const VkAttachmentReference colour{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments = &colour;
    VkSubpassDependency dep[2]{};
    dep[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dep[0].dstSubpass = 0;
    dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep[1].srcSubpass = 0;
    dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 1;
    rp.pAttachments = &attachment;
    rp.subpassCount = 1;
    rp.pSubpasses = &sp;
    rp.dependencyCount = 2;
    rp.pDependencies = dep;
    VkRenderPass pass;
    VK_CHECK(vkCreateRenderPass(device, &rp, nullptr, &pass));
    return pass;
}

// target.pass is the layer pass its framebuffers are made for
void createLayerTarget(GpuContext &gpu, const LayerTargetSize &size, VulkanLayerTarget &target)
{
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = kLayerFormat;
    ii.extent = {size.width, size.height, 1};
    ii.mipLevels = 1;
    ii.arrayLayers = size.layers;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VK_CHECK(vkCreateImage(gpu.device, &ii, nullptr, &target.image));
    target.memory = gpu.allocateImageMemory(target.image);
    target.width = size.width;
    target.height = size.height;
    gpu.oneShot(MakeLayersReadable{target.image, size.layers});
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = target.image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    vi.format = kLayerFormat;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, size.layers};
    target.arrayView = createView(gpu.device, vi);
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    for (uint32_t i = 0; i < size.layers; ++i)
    {
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, i, 1};
        target.layerViews.push_back(createView(gpu.device, vi));
    }
    createLayerFramebuffers(gpu.device, target);
}

void destroyLayerTarget(VkDevice device, VulkanLayerTarget &target)
{
    for (const VkFramebuffer framebuffer : target.framebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (const VkImageView view : target.layerViews)
    {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroyImageView(device, target.arrayView, nullptr);
    vkDestroyImage(device, target.image, nullptr);
    vkFreeMemory(device, target.memory, nullptr);
    target = {};
}

}  // namespace argon::gfx::vulkan
