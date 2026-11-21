// Argon: gfx::UiRenderer over Direct3D 12 (gfx\d3d12\d3d12_ui_renderer.hpp).
#include "gfx/d3d12/d3d12_ui_renderer.hpp"
#include "gfx/d3d12/d3d12_shaders.hpp"
#include "gfx/d3d12/d3d12_ui_image.hpp"
#include "files/dds.hpp"

#include <cstring>

namespace argon::gfx::d3d12
{
namespace
{
enum RootParameter : UINT
{
    kTransform,
    kImage,
    kSampler
};

template <size_t N> D3D12_SHADER_BYTECODE bytecode(const unsigned char (&blob)[N])
{
    return D3D12_SHADER_BYTECODE{blob, N};
}

// ImGui's vertices (ImDrawVert: pos, uv, col), TEXCOORD<location> as SPIRV-Cross names them
const D3D12_INPUT_ELEMENT_DESC kUiInputs[] = {
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

// ImGui's own blend (Normal), the game's additive (mode 2), the negative brightness, the plasma panel (mode 1)
D3D12_RENDER_TARGET_BLEND_DESC blendOf(int look)
{
    D3D12_RENDER_TARGET_BLEND_DESC rt{};
    rt.BlendEnable = TRUE;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    rt.BlendOp = rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    const bool alphaBlended = look == 0 || look == 3;
    rt.SrcBlend = alphaBlended ? D3D12_BLEND_SRC_ALPHA : D3D12_BLEND_ONE;
    rt.DestBlend = alphaBlended ? D3D12_BLEND_INV_SRC_ALPHA : D3D12_BLEND_ONE;
    rt.BlendOp = look == 2 ? D3D12_BLEND_OP_REV_SUBTRACT : D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = look == 0 ? D3D12_BLEND_ONE : D3D12_BLEND_ZERO;
    rt.DestBlendAlpha = look == 0 ? D3D12_BLEND_INV_SRC_ALPHA : D3D12_BLEND_ONE;
    return rt;
}

ComPtr<ID3D12Resource> growBuffer(D3D12Context &context, size_t &capacity, size_t needed)
{
    capacity = std::max(needed + needed / 2, size_t(64 * 1024));
    return context.createBuffer(D3D12_HEAP_TYPE_UPLOAD, capacity);
}
}  // namespace

void D3D12UiRenderer::create(D3D12Device &device, D3D12Presenter &presenter)
{
    device_ = &device;
    presenter_ = &presenter;
    ImGuiIO &io = ImGui::GetIO();
    io.BackendRendererName = "argon_d3d12";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    createRootSignature();
    createPipelines();
    createFontImage();
}

void D3D12UiRenderer::createRootSignature()
{
    const D3D12_DESCRIPTOR_RANGE image{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    const D3D12_DESCRIPTOR_RANGE sampler{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    D3D12_ROOT_PARAMETER params[3]{};
    params[kTransform].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[kTransform].Constants = {0, 0, 4};
    params[kTransform].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    params[kImage].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[kImage].DescriptorTable = {1, &image};
    params[kImage].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[kSampler].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[kSampler].DescriptorTable = {1, &sampler};
    params[kSampler].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC desc{3, params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    ComPtr<ID3DBlob> blob, error;
    check(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), "D3D12SerializeRootSignature (UI)");
    check(device_->context().device()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)),
          "CreateRootSignature (UI)");
}

void D3D12UiRenderer::createPipelines()
{
    const SceneTargets &targets = device_->sceneTargets();
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_.Get();
    desc.VS = bytecode(kHudAddVertDxil);
    desc.SampleMask = UINT_MAX;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.RasterizerState.DepthClipEnable = TRUE;
    desc.InputLayout = {kUiInputs, 3};
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = targets.colour;
    desc.DSVFormat = targets.depth;
    desc.SampleDesc.Count = targets.samples;
    for (int look = 0; look < 4; ++look)
    {
        desc.PS = look == int(Look::Panel) ? bytecode(kHudPanelFragDxil) : bytecode(kHudAddFragDxil);
        desc.BlendState.RenderTarget[0] = blendOf(look);
        check(device_->context().device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelines_[look])), "CreateGraphicsPipelineState (UI)");
    }
}

void D3D12UiRenderer::createFontImage()
{
    unsigned char *pixels = nullptr;
    int width = 0, height = 0;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    argon::TextureData picture;
    picture.width = uint32_t(width);
    picture.height = uint32_t(height);
    picture.format = argon::gfx::TextureFormat::Rgba8;
    picture.bytes.assign(pixels, pixels + size_t(width) * height * 4);
    picture.levels.push_back({picture.width, picture.height, 0, picture.bytes.size()});
    font_ = device_->createTexture(picture);
    fontImage_ = addImage(font_, SamplerMode::Wrap);
    ImGui::GetIO().Fonts->SetTexID(fontImage_);
}

void D3D12UiRenderer::destroy()
{
    removeImage(fontImage_);
    device_->destroyTexture(font_);
    for (Geometry &geometry : geometry_)
    {
        geometry = {};
    }
    for (ComPtr<ID3D12PipelineState> &pipeline : pipelines_)
    {
        pipeline.Reset();
    }
    rootSignature_.Reset();
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->SetTexID(ImTextureID{});
    io.BackendRendererName = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
}

void D3D12UiRenderer::newFrame()
{
}

void D3D12UiRenderer::windowRemade()
{
}

// ------------------------------------------------------------------ images
ImTextureID D3D12UiRenderer::addImage(const Texture &texture, SamplerMode sampling)
{
    const D3D12Texture &record = device_->texture(texture.handle);
    D3D12_SHADER_RESOURCE_VIEW_DESC view{record.format, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
    view.Texture2D.MipLevels = record.levels;
    const uint32_t index = device_->context().shaderViews().take();
    device_->context().device()->CreateShaderResourceView(record.resource.Get(), &view, device_->context().shaderViews().cpu(index));
    return uiImageOf(index, sampling);
}

ImTextureID D3D12UiRenderer::addLayeredImage(const Texture &texture, SamplerMode sampling)
{
    const D3D12Texture &record = device_->texture(texture.handle);
    const uint32_t index = device_->context().shaderViews().take();
    device_->context().device()->CopyDescriptorsSimple(1, device_->context().shaderViews().cpu(index), device_->context().cpuViews().cpu(record.cpuView),
                                                       D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return uiImageOf(index, sampling);
}

void D3D12UiRenderer::removeImage(ImTextureID image)
{
    if (image)
    {
        device_->context().shaderViews().give(viewIndexOf(image));
    }
}

// ------------------------------------------------------------------ drawing
// this frame slot's buffers (free again: the presenter waited for the slot), grown as needed
void D3D12UiRenderer::uploadGeometry(const ImDrawData &data)
{
    Geometry &geometry = geometry_[presenter_->frameSlot()];
    const size_t vertexBytes = size_t(data.TotalVtxCount) * sizeof(ImDrawVert), indexBytes = size_t(data.TotalIdxCount) * sizeof(ImDrawIdx);
    if (vertexBytes > geometry.vertexCapacity)
    {
        geometry.vertices = growBuffer(device_->context(), geometry.vertexCapacity, vertexBytes);
    }
    if (indexBytes > geometry.indexCapacity)
    {
        geometry.indices = growBuffer(device_->context(), geometry.indexCapacity, indexBytes);
    }
    uint8_t *vertices = nullptr, *indices = nullptr;
    const D3D12_RANGE nothingRead{0, 0};
    check(geometry.vertices->Map(0, &nothingRead, reinterpret_cast<void **>(&vertices)), "Map (UI vertices)");
    check(geometry.indices->Map(0, &nothingRead, reinterpret_cast<void **>(&indices)), "Map (UI indices)");
    for (int i = 0; i < data.CmdListsCount; ++i)
    {
        const ImDrawList &list = *data.CmdLists[i];
        std::memcpy(vertices, list.VtxBuffer.Data, size_t(list.VtxBuffer.Size) * sizeof(ImDrawVert));
        std::memcpy(indices, list.IdxBuffer.Data, size_t(list.IdxBuffer.Size) * sizeof(ImDrawIdx));
        vertices += size_t(list.VtxBuffer.Size) * sizeof(ImDrawVert);
        indices += size_t(list.IdxBuffer.Size) * sizeof(ImDrawIdx);
    }
    geometry.vertices->Unmap(0, nullptr);
    geometry.indices->Unmap(0, nullptr);
}

// ImGui's root signature, the look's pipeline, the window's viewport, this frame's geometry and the transform
// (ImGui's Vulkan one: the shader flips y for Direct3D)
void D3D12UiRenderer::setRenderState(Look look)
{
    ID3D12GraphicsCommandList *list = presenter_->frameCommands().native();
    const ImDrawData &data = *drawing_;
    const Geometry &geometry = geometry_[presenter_->frameSlot()];
    ID3D12DescriptorHeap *heaps[] = {device_->context().shaderViews().heap(), device_->context().samplers().heap()};
    list->SetDescriptorHeaps(2, heaps);
    list->SetGraphicsRootSignature(rootSignature_.Get());
    list->SetPipelineState(pipelines_[int(look)].Get());
    const D3D12_VIEWPORT viewport{0.f, 0.f, data.DisplaySize.x * data.FramebufferScale.x, data.DisplaySize.y * data.FramebufferScale.y, 0.f, 1.f};
    list->RSSetViewports(1, &viewport);
    if (geometry.vertices)
    {
        const D3D12_VERTEX_BUFFER_VIEW vertices{geometry.vertices->GetGPUVirtualAddress(), UINT(geometry.vertexCapacity), sizeof(ImDrawVert)};
        const D3D12_INDEX_BUFFER_VIEW indices{geometry.indices->GetGPUVirtualAddress(), UINT(geometry.indexCapacity),
                                              sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT};
        list->IASetVertexBuffers(0, 1, &vertices);
        list->IASetIndexBuffer(&indices);
    }
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    const float scale[2] = {2.f / data.DisplaySize.x, 2.f / data.DisplaySize.y};
    const float transform[4] = {scale[0], scale[1], -1.f - data.DisplayPos.x * scale[0], -1.f - data.DisplayPos.y * scale[1]};
    list->SetGraphicsRoot32BitConstants(kTransform, 4, transform, 0);
    boundImage_ = ImTextureID{};
}

void D3D12UiRenderer::drawCommand(const ImDrawCmd &command, uint32_t firstIndex, int32_t firstVertex)
{
    const ImDrawData &data = *drawing_;
    const ImVec2 scale = data.FramebufferScale;
    const D3D12_RECT clip{LONG((command.ClipRect.x - data.DisplayPos.x) * scale.x), LONG((command.ClipRect.y - data.DisplayPos.y) * scale.y),
                          LONG((command.ClipRect.z - data.DisplayPos.x) * scale.x), LONG((command.ClipRect.w - data.DisplayPos.y) * scale.y)};
    if (clip.right <= clip.left || clip.bottom <= clip.top)
    {
        return;
    }
    ID3D12GraphicsCommandList *list = presenter_->frameCommands().native();
    list->RSSetScissorRects(1, &clip);
    const ImTextureID image = command.GetTexID();
    if (image != boundImage_)
    {
        list->SetGraphicsRootDescriptorTable(kImage, device_->context().shaderViews().gpu(viewIndexOf(image)));
        list->SetGraphicsRootDescriptorTable(kSampler, device_->samplerOf(samplingOf(image)));
        boundImage_ = image;
    }
    list->DrawIndexedInstanced(command.ElemCount, 1, command.IdxOffset + firstIndex, int32_t(command.VtxOffset) + firstVertex, 0);
}

void D3D12UiRenderer::render()
{
    const ImDrawData *data = ImGui::GetDrawData();
    if (!data || data->DisplaySize.x <= 0.f || data->DisplaySize.y <= 0.f)
    {
        return;
    }
    drawing_ = data;
    if (data->TotalVtxCount > 0)
    {
        uploadGeometry(*data);
    }
    setRenderState(Look::Normal);
    uint32_t firstIndex = 0;
    int32_t firstVertex = 0;
    for (int i = 0; i < data->CmdListsCount; ++i)
    {
        const ImDrawList &list = *data->CmdLists[i];
        for (const ImDrawCmd &command : list.CmdBuffer)
        {
            if (command.UserCallback == ImDrawCallback_ResetRenderState)
            {
                setRenderState(Look::Normal);
            }
            else if (command.UserCallback)
            {
                presenter_->frameCommands().bindRoot();   // the scene's: the menus' models draw here
                command.UserCallback(&list, &command);
            }
            else
            {
                drawCommand(command, firstIndex, firstVertex);
            }
        }
        firstIndex += uint32_t(list.IdxBuffer.Size);
        firstVertex += list.VtxBuffer.Size;
    }
    drawing_ = nullptr;
}

void D3D12UiRenderer::useBlend(UiBlend blend)
{
    if (!drawing_)
    {
        return;
    }
    switch (blend)
    {
    case UiBlend::Additive:
        setRenderState(Look::Additive);
        break;
    case UiBlend::Subtractive:
        setRenderState(Look::Subtractive);
        break;
    case UiBlend::Panel:
        setRenderState(Look::Panel);
        break;
    }
}

}  // namespace argon::gfx::d3d12
