// Argon: gfx::CommandList over a Vulkan command buffer and the device's pipeline layout (sets 0 textures, 1 scene,
// 2 materials; one push constant range for both stages).
#pragma once
#include "gfx/command_list.hpp"

#include <vulkan/vulkan.h>

namespace argon::gfx::vulkan
{
class VulkanDevice;
}

namespace argon::gfx::vulkan
{

class VulkanCommandList final : public CommandList
{
  public:
    VulkanCommandList(VkCommandBuffer cmd, const VulkanDevice &device);

    void bindPipeline(Pipeline pipeline) override;
    void bindTextures(TextureSet set) override;
    void bindMaterials(MaterialSet set) override;
    void bindScene(SceneSet set) override;
    void pushConstants(std::span<const std::byte> constants) override;
    void bindVertexBuffer(BufferRef buffer) override;
    void bindIndexBuffer(BufferRef buffer) override;
    void setViewport(const Rect &area) override;
    void setDepthCull(const DepthCull &state) override;
    void setFrontFace(Winding front) override;
    void clearDepth(const Rect &area, float depth) override;
    void clear(const Rect &area, const SceneClear &values) override;
    void copyToTexture(const TextureUpload &upload) override;
    void beginLayerPass(LayerTarget target, uint32_t layer) override;
    void endLayerPass() override;
    void draw(const DrawRange &range) override;
    void drawIndexed(const IndexedDraw &draw) override;
    void beginQuery(const OcclusionQuery &query) override;
    void endQuery(const OcclusionQuery &query) override;
    void resetQueries(const QueryRange &range) override;

  private:
    void bindSet(uint32_t index, VkDescriptorSet set);

    VkCommandBuffer cmd_;
    const VulkanDevice &device_;
    VkPipelineLayout layout_;
};

}  // namespace argon::gfx::vulkan
