// Argon: gfx::UiRenderer over ImGui's Vulkan backend (gfx\vulkan\vulkan_ui_renderer.hpp).
#include "gfx/vulkan/vulkan_ui_renderer.hpp"
#include "gfx/vulkan/vulkan_shaders.hpp"

#include <imgui_impl_vulkan.h>

#include <cstddef>

namespace argon::gfx::vulkan
{
namespace
{
// the HUD / menu textures (~45) and the credits' 16 pictures, with room to spare
const uint32_t kImages = 128;

VkPipelineColorBlendAttachmentState blendOf(UiBlend blend)
{
    const bool panel = blend == UiBlend::Panel;
    VkPipelineColorBlendAttachmentState cb{};
    cb.blendEnable = VK_TRUE;
    cb.srcColorBlendFactor = panel ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
    cb.dstColorBlendFactor = panel ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
    cb.colorBlendOp = blend == UiBlend::Subtractive ? VK_BLEND_OP_REVERSE_SUBTRACT : VK_BLEND_OP_ADD;
    cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cb.alphaBlendOp = VK_BLEND_OP_ADD;
    cb.colorWriteMask = 0xf;
    return cb;
}
}  // namespace

void VulkanUiRenderer::create(VulkanDevice &device, VulkanPresenter &presenter, const Swapchain &swapchain)
{
    device_ = &device;
    presenter_ = &presenter;
    swapchain_ = &swapchain;
    const GpuContext &gpu = device.gpu();
    const VkDescriptorPoolSize images{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kImages};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dp.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dp.maxSets = kImages;
    dp.poolSizeCount = 1;
    dp.pPoolSizes = &images;
    VK_CHECK(vkCreateDescriptorPool(gpu.device, &dp, nullptr, &pool_));
    ImGui_ImplVulkan_InitInfo ii{};
    ii.ApiVersion = VK_API_VERSION_1_3;
    ii.Instance = gpu.instance;
    ii.PhysicalDevice = gpu.physical;
    ii.Device = gpu.device;
    ii.QueueFamily = gpu.queueFamily;
    ii.Queue = gpu.queue;
    ii.DescriptorPool = pool_;
    ii.RenderPass = swapchain.renderPass;
    ii.MinImageCount = swapchain.minImageCount;
    ii.ImageCount = uint32_t(swapchain.images.size());
    ii.MSAASamples = swapchain.samples;
    ImGui_ImplVulkan_Init(&ii);
    ImGui_ImplVulkan_CreateFontsTexture();
}

void VulkanUiRenderer::destroy()
{
    const VkDevice device = device_->gpu().device;
    for (VkPipeline &pipeline : blendPipelines_)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device, pool_, nullptr);
    pool_ = VK_NULL_HANDLE;
}

void VulkanUiRenderer::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
}

void VulkanUiRenderer::render()
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), presenter_->commandBuffer());
}

ImTextureID VulkanUiRenderer::addImage(const Texture &texture, SamplerMode sampling)
{
    const GpuTexture &record = device_->gpuTexture(texture.handle);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = record.image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = record.format;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, record.levels, 0, 1};
    VkImageView view;
    VK_CHECK(vkCreateImageView(device_->gpu().device, &vi, nullptr, &view));
    const auto image = ImTextureID(ImGui_ImplVulkan_AddTexture(device_->sampler(sampling), view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    firstLayerViews_[image] = view;
    return image;
}

ImTextureID VulkanUiRenderer::addLayeredImage(const Texture &texture, SamplerMode sampling)
{
    return ImTextureID(ImGui_ImplVulkan_AddTexture(device_->sampler(sampling), device_->viewOf(texture.handle), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}

void VulkanUiRenderer::removeImage(ImTextureID image)
{
    if (!image)
    {
        return;
    }
    ImGui_ImplVulkan_RemoveTexture(VkDescriptorSet(image));
    const auto view = firstLayerViews_.find(image);
    if (view != firstLayerViews_.end())
    {
        vkDestroyImageView(device_->gpu().device, view->second, nullptr);
        firstLayerViews_.erase(view);
    }
}

// the pipeline goes on ImGui's command buffer, made against ImGui's pipeline layout the first time
void VulkanUiRenderer::useBlend(UiBlend blend)
{
    const auto *state = static_cast<ImGui_ImplVulkan_RenderState *>(ImGui::GetPlatformIO().Renderer_RenderState);
    if (!state)
    {
        return;
    }
    VkPipeline &pipeline = blendPipelines_[size_t(blend)];
    if (!pipeline)
    {
        pipeline = createBlendPipeline(blend, state->PipelineLayout);
    }
    vkCmdBindPipeline(state->CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void VulkanUiRenderer::windowRemade()
{
    ImGui_ImplVulkan_SetMinImageCount(swapchain_->minImageCount);
}

// ImGui's vertices; the panel's fragment shader samples a 2D array (the plasma), the others a 2D image
VkPipeline VulkanUiRenderer::createBlendPipeline(UiBlend blend, VkPipelineLayout layout) const
{
    const GpuContext &gpu = device_->gpu();
    const VkShaderModule vs = gpu.createShader(kHudAddVert, sizeof kHudAddVert);
    const VkShaderModule fs =
        blend == UiBlend::Panel ? gpu.createShader(kHudPanelFrag, sizeof kHudPanelFrag) : gpu.createShader(kHudAddFrag, sizeof kHudAddFrag);
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main"};
    st[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main"};
    const VkVertexInputBindingDescription bind{0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription at[3] = {{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},
                                                     {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},
                                                     {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)}};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bind;
    vi.vertexAttributeDescriptionCount = 3;
    vi.pVertexAttributeDescriptions = at;
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.lineWidth = 1.f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = swapchain_->samples;
    const VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    const VkPipelineColorBlendAttachmentState cb = blendOf(blend);
    VkPipelineColorBlendStateCreateInfo cbs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbs.attachmentCount = 1;
    cbs.pAttachments = &cb;
    const VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2;
    dy.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2;
    gp.pStages = st;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cbs;
    gp.pDynamicState = &dy;
    gp.layout = layout;
    gp.renderPass = swapchain_->renderPass;
    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(gpu.device, VK_NULL_HANDLE, 1, &gp, nullptr, &pipeline));
    vkDestroyShaderModule(gpu.device, vs, nullptr);
    vkDestroyShaderModule(gpu.device, fs, nullptr);
    return pipeline;
}

}  // namespace argon::gfx::vulkan
