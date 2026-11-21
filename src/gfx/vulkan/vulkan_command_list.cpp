// Argon: gfx::CommandList over a Vulkan command buffer (gfx\vulkan\vulkan_command_list.hpp).
#include "gfx/vulkan/vulkan_command_list.hpp"
#include "gfx/vulkan/vulkan_device.hpp"
#include "gfx/vulkan/vulkan_handles.hpp"
#include "gfx/vulkan/vulkan_layer_target.hpp"

namespace argon::gfx::vulkan
{
namespace
{
const VkShaderStageFlags kVertexAndFragment = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
const uint32_t kTextureSet = 0, kSceneSet = 1, kMaterialSet = 2;
}  // namespace

VulkanCommandList::VulkanCommandList(VkCommandBuffer cmd, const VulkanDevice &device)
    : cmd_(cmd), device_(device), layout_(device.pipelineLayout())
{
}

void VulkanCommandList::bindPipeline(Pipeline pipeline)
{
    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, unwrap<VkPipeline>(pipeline));
}

void VulkanCommandList::bindSet(uint32_t index, VkDescriptorSet set)
{
    vkCmdBindDescriptorSets(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, layout_, index, 1, &set, 0, nullptr);
}

void VulkanCommandList::bindTextures(TextureSet set)
{
    bindSet(kTextureSet, unwrap<VkDescriptorSet>(set));
}

void VulkanCommandList::bindMaterials(MaterialSet set)
{
    bindSet(kMaterialSet, unwrap<VkDescriptorSet>(set));
}

void VulkanCommandList::bindScene(SceneSet set)
{
    bindSet(kSceneSet, unwrap<VkDescriptorSet>(set));
}

void VulkanCommandList::pushConstants(std::span<const std::byte> constants)
{
    vkCmdPushConstants(cmd_, layout_, kVertexAndFragment, 0, uint32_t(constants.size()), constants.data());
}

void VulkanCommandList::bindVertexBuffer(BufferRef buffer)
{
    const VkBuffer vulkanBuffer = unwrap<VkBuffer>(buffer);
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd_, 0, 1, &vulkanBuffer, &offset);
}

void VulkanCommandList::bindIndexBuffer(BufferRef buffer)
{
    vkCmdBindIndexBuffer(cmd_, unwrap<VkBuffer>(buffer), 0, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandList::setViewport(const Rect &area)
{
    const VkViewport viewport{float(area.x), float(area.y), float(area.width), float(area.height), 0, 1};
    const VkRect2D scissor = toVulkan(area);
    vkCmdSetViewport(cmd_, 0, 1, &viewport);
    vkCmdSetScissor(cmd_, 0, 1, &scissor);
}

void VulkanCommandList::setDepthCull(const DepthCull &state)
{
    vkCmdSetDepthTestEnable(cmd_, state.test);
    vkCmdSetDepthWriteEnable(cmd_, state.write);
    vkCmdSetCullMode(cmd_, state.cullBack ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE);
}

void VulkanCommandList::setFrontFace(Winding front)
{
    vkCmdSetFrontFace(cmd_, front == Winding::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE);
}

void VulkanCommandList::clearDepth(const Rect &area, float depth)
{
    VkClearAttachment attachment{VK_IMAGE_ASPECT_DEPTH_BIT, 0, {}};
    attachment.clearValue.depthStencil = {depth, 0};
    const VkClearRect rect{toVulkan(area), 0, 1};
    vkCmdClearAttachments(cmd_, 1, &attachment, 1, &rect);
}

void VulkanCommandList::clear(const Rect &area, const SceneClear &values)
{
    VkClearAttachment attachments[2]{};
    attachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    attachments[0].clearValue.color = {{values.colour.r, values.colour.g, values.colour.b, values.colour.a}};
    attachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    attachments[1].clearValue.depthStencil = {values.depth, 0};
    const VkClearRect rect{toVulkan(area), 0, 1};
    vkCmdClearAttachments(cmd_, 2, attachments, 1, &rect);
}

void VulkanCommandList::draw(const DrawRange &range)
{
    vkCmdDraw(cmd_, range.count, 1, range.first, 0);
}

void VulkanCommandList::copyToTexture(const TextureUpload &upload)
{
    const VkImage image = device_.gpuTexture(upload.texture).image;
    GpuContext::transition(cmd_, image, ImageTransition::sampledToTransferDst());
    VkBufferImageCopy region{};
    region.bufferRowLength = upload.rowPitch / 4;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {upload.width, upload.height, 1};
    vkCmdCopyBufferToImage(cmd_, unwrap<VkBuffer>(upload.source), image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    GpuContext::transition(cmd_, image, ImageTransition::transferDstToSampled());
}

void VulkanCommandList::beginLayerPass(LayerTarget target, uint32_t layer)
{
    const VulkanLayerTarget &record = recordOf(target);
    const VkClearValue black{};
    VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rb.renderPass = record.pass;
    rb.framebuffer = record.framebuffers[layer];
    rb.renderArea.extent = {record.width, record.height};
    rb.clearValueCount = 1;
    rb.pClearValues = &black;
    vkCmdBeginRenderPass(cmd_, &rb, VK_SUBPASS_CONTENTS_INLINE);
    setViewport(Rect{0, 0, record.width, record.height});
}

void VulkanCommandList::endLayerPass()
{
    vkCmdEndRenderPass(cmd_);
}

void VulkanCommandList::drawIndexed(const IndexedDraw &draw)
{
    vkCmdDrawIndexed(cmd_, draw.count, 1, draw.firstIndex, draw.vertexOffset, draw.firstInstance);
}

void VulkanCommandList::beginQuery(const OcclusionQuery &query)
{
    vkCmdBeginQuery(cmd_, unwrap<VkQueryPool>(query.pool), query.index, query.precise ? VK_QUERY_CONTROL_PRECISE_BIT : 0);
}

void VulkanCommandList::endQuery(const OcclusionQuery &query)
{
    vkCmdEndQuery(cmd_, unwrap<VkQueryPool>(query.pool), query.index);
}

void VulkanCommandList::resetQueries(const QueryRange &range)
{
    vkCmdResetQueryPool(cmd_, unwrap<VkQueryPool>(range.pool), range.first, range.count);
}

}  // namespace argon::gfx::vulkan
