// Argon: gfx::CommandList over a Direct3D 12 graphics command list and the device's root signature. The pipeline
// state object is picked at the draw from the bound family and the draw state; the vertex buffer is bound there too
// (its stride is the family's); the base vertex and instance go to root parameter 1 before each draw.
#pragma once
#include "gfx/command_list.hpp"
#include "gfx/d3d12/d3d12_device.hpp"

namespace argon::gfx::d3d12
{

// the scene pass's colour and depth targets the clears write
struct PassTargets
{
    D3D12_CPU_DESCRIPTOR_HANDLE colour{}, depth{};
};

class D3D12CommandList final : public CommandList
{
  public:
    D3D12CommandList(ID3D12GraphicsCommandList *commands, D3D12Device &device) : commands_(commands), device_(device)
    {
    }

    // the root signature and the shader-visible heaps again (after ImGui's own, or a new list)
    void bindRoot();
    void useTargets(const PassTargets &targets)
    {
        targets_ = targets;
    }

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

    ID3D12GraphicsCommandList *native() const
    {
        return commands_;
    }

  private:
    void prepareDraw(int32_t baseVertex, uint32_t baseInstance);

    ID3D12GraphicsCommandList *commands_;
    D3D12Device &device_;
    PassTargets targets_;
    Pipeline pipeline_;
    DrawState state_;
    bool stateChanged_ = true;
    BufferRef vertices_;
    bool verticesChanged_ = false;
    LayerTarget layerTarget_;   // the layer pass's
    uint32_t layer_ = 0;
};

}  // namespace argon::gfx::d3d12
