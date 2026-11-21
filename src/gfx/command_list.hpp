// Argon: recording draws into a frame, the same for every backend: the operations the scene, the effect passes and the
// editor's markings use. A backend implements it over its own command buffer (gfx\vulkan\vulkan_command_list.hpp).
#pragma once
#include "gfx/gfx_types.hpp"

#include <cstddef>
#include <span>

namespace argon::gfx
{

class CommandList
{
  public:
    virtual ~CommandList() = default;

    virtual void bindPipeline(Pipeline pipeline) = 0;
    virtual void bindTextures(TextureSet set) = 0;
    virtual void bindMaterials(MaterialSet set) = 0;
    virtual void bindScene(SceneSet set) = 0;
    // the push constants (the mesh shaders' PushConstants), for both stages
    virtual void pushConstants(std::span<const std::byte> constants) = 0;
    virtual void bindVertexBuffer(BufferRef buffer) = 0;
    virtual void bindIndexBuffer(BufferRef buffer) = 0;   // 32-bit indices

    virtual void setViewport(const Rect &area) = 0;   // the scissor too
    virtual void setDepthCull(const DepthCull &state) = 0;
    virtual void setFrontFace(Winding front) = 0;
    // the depth inside `area` back to the far plane
    virtual void clearDepth(const Rect &area, float depth) = 0;
    // the colour and the depth inside `area` (a split screen's view)
    virtual void clear(const Rect &area, const SceneClear &values) = 0;

    // outside the passes: the texture written, then readable by the shaders again
    virtual void copyToTexture(const TextureUpload &upload) = 0;
    // one layer of a layer target cleared to black, the viewport and scissor the whole layer; outside other passes
    virtual void beginLayerPass(LayerTarget target, uint32_t layer) = 0;
    virtual void endLayerPass() = 0;

    virtual void draw(const DrawRange &range) = 0;
    virtual void drawIndexed(const IndexedDraw &draw) = 0;
    virtual void beginQuery(const OcclusionQuery &query) = 0;
    virtual void endQuery(const OcclusionQuery &query) = 0;
    // before the queries are used again (outside a render pass)
    virtual void resetQueries(const QueryRange &range) = 0;
};

}  // namespace argon::gfx
