// Argon: gfx::UiRenderer over Direct3D 12: ImGui's draw data drawn with the HUD shaders (hudadd.vert / .frag,
// hudpanel.frag) and a root signature of their own (0 the scale and translate, b0; 1 the image, t0; 2 its sampler, s0),
// so each image keeps the SamplerMode it was added with (ImGui's own DX12 backend has one clamping sampler). Draw
// callbacks run with the scene's root signature bound (the menus' models); ImDrawCallback_ResetRenderState and
// useBlend set ImGui's state again.
#pragma once
#include "gfx/d3d12/d3d12_presenter.hpp"
#include "gfx/ui_renderer.hpp"

#include <unordered_map>

namespace argon::gfx::d3d12
{

class D3D12UiRenderer final : public UiRenderer
{
  public:
    // ImGui's context made first
    void create(D3D12Device &device, D3D12Presenter &presenter);
    // every image removed first
    void destroy();

    void newFrame() override;
    void render() override;
    ImTextureID addImage(const Texture &texture, SamplerMode sampling) override;
    ImTextureID addLayeredImage(const Texture &texture, SamplerMode sampling) override;
    void removeImage(ImTextureID image) override;
    void useBlend(UiBlend blend) override;
    void windowRemade() override;

  private:
    enum class Look
    {
        Normal,   // ImGui's: alpha blended
        Additive,
        Subtractive,
        Panel
    };
    struct Geometry
    {
        ComPtr<ID3D12Resource> vertices, indices;
        size_t vertexCapacity = 0, indexCapacity = 0;
    };

    void createRootSignature();
    void createPipelines();
    void createFontImage();
    void uploadGeometry(const ImDrawData &data);
    void setRenderState(Look look);
    void drawCommand(const ImDrawCmd &command, uint32_t firstIndex, int32_t firstVertex);

    D3D12Device *device_ = nullptr;
    D3D12Presenter *presenter_ = nullptr;
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelines_[4];   // by Look
    Texture font_;
    ImTextureID fontImage_{};
    Geometry geometry_[kFramesInFlight];
    const ImDrawData *drawing_ = nullptr;   // during render
    ImTextureID boundImage_{};
};

}  // namespace argon::gfx::d3d12
