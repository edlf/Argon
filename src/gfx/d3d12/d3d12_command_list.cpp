// Argon: gfx::CommandList over Direct3D 12 (gfx\d3d12\d3d12_command_list.hpp).
#include "gfx/d3d12/d3d12_command_list.hpp"

namespace argon::gfx::d3d12
{
namespace
{
enum RootParameter : UINT
{
    kPush,
    kVertexInfo,
    kTextures,
    kSamplers,
    kScene,
    kMaterials
};

const D3D12_RESOURCE_STATES kShaderRead = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

D3D12_RECT rectOf(const Rect &area)
{
    return D3D12_RECT{area.x, area.y, LONG(area.x + int32_t(area.width)), LONG(area.y + int32_t(area.height))};
}

D3D12_RESOURCE_BARRIER transition(ID3D12Resource *resource, UINT subresource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition = {resource, subresource, from, to};
    return barrier;
}
}  // namespace

void D3D12CommandList::bindRoot()
{
    ID3D12DescriptorHeap *heaps[] = {device_.context().shaderViews().heap(), device_.context().samplers().heap()};
    commands_->SetDescriptorHeaps(2, heaps);
    commands_->SetGraphicsRootSignature(device_.rootSignature());
    stateChanged_ = true;
    verticesChanged_ = bool(vertices_);
}

void D3D12CommandList::bindPipeline(Pipeline pipeline)
{
    pipeline_ = pipeline;
    stateChanged_ = true;
    verticesChanged_ = bool(vertices_);   // the stride may change
}

void D3D12CommandList::bindTextures(TextureSet set)
{
    commands_->SetGraphicsRootDescriptorTable(kTextures, device_.textureTable(set));
    commands_->SetGraphicsRootDescriptorTable(kSamplers, device_.samplerTable(set));
}

void D3D12CommandList::bindMaterials(MaterialSet set)
{
    commands_->SetGraphicsRootDescriptorTable(kMaterials, device_.setTable(set.value));
}

void D3D12CommandList::bindScene(SceneSet set)
{
    commands_->SetGraphicsRootDescriptorTable(kScene, device_.setTable(set.value));
}

void D3D12CommandList::pushConstants(std::span<const std::byte> constants)
{
    commands_->SetGraphicsRoot32BitConstants(kPush, UINT(constants.size() / 4), constants.data(), 0);
}

void D3D12CommandList::bindVertexBuffer(BufferRef buffer)
{
    vertices_ = buffer;
    verticesChanged_ = true;
}

void D3D12CommandList::bindIndexBuffer(BufferRef buffer)
{
    const D3D12_INDEX_BUFFER_VIEW view = device_.indexView(buffer);
    commands_->IASetIndexBuffer(&view);
}

void D3D12CommandList::setViewport(const Rect &area)
{
    const D3D12_VIEWPORT viewport{float(area.x), float(area.y), float(area.width), float(area.height), 0.f, 1.f};
    const D3D12_RECT scissor = rectOf(area);
    commands_->RSSetViewports(1, &viewport);
    commands_->RSSetScissorRects(1, &scissor);
}

// an unchanged state keeps the bound state object (SetPipelineState is the costly part of a draw)
void D3D12CommandList::setDepthCull(const DepthCull &state)
{
    if (state_.depthCull == state)
    {
        return;
    }
    state_.depthCull = state;
    stateChanged_ = true;
}

void D3D12CommandList::setFrontFace(Winding front)
{
    if (state_.front == front)
    {
        return;
    }
    state_.front = front;
    stateChanged_ = true;
}

void D3D12CommandList::clearDepth(const Rect &area, float depth)
{
    const D3D12_RECT rect = rectOf(area);
    commands_->ClearDepthStencilView(targets_.depth, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 1, &rect);
}

void D3D12CommandList::clear(const Rect &area, const SceneClear &values)
{
    const D3D12_RECT rect = rectOf(area);
    const float colour[4] = {values.colour.r, values.colour.g, values.colour.b, values.colour.a};
    commands_->ClearRenderTargetView(targets_.colour, colour, 1, &rect);
    commands_->ClearDepthStencilView(targets_.depth, D3D12_CLEAR_FLAG_DEPTH, values.depth, 0, 1, &rect);
}

void D3D12CommandList::copyToTexture(const TextureUpload &upload)
{
    const D3D12Texture &texture = device_.texture(upload.texture);
    ID3D12Resource *resource = texture.resource.Get();
    const D3D12_RESOURCE_BARRIER writable = transition(resource, 0, kShaderRead, D3D12_RESOURCE_STATE_COPY_DEST);
    commands_->ResourceBarrier(1, &writable);
    D3D12_TEXTURE_COPY_LOCATION to{resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    D3D12_TEXTURE_COPY_LOCATION from{device_.bufferResource(upload.source), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    from.PlacedFootprint.Footprint = {texture.format, upload.width, upload.height, 1, upload.rowPitch};
    commands_->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
    const D3D12_RESOURCE_BARRIER readable = transition(resource, 0, D3D12_RESOURCE_STATE_COPY_DEST, kShaderRead);
    commands_->ResourceBarrier(1, &readable);
}

void D3D12CommandList::beginLayerPass(LayerTarget target, uint32_t layer)
{
    const D3D12LayerTarget &kept = device_.layerTargetOf(target);
    ID3D12Resource *resource = device_.texture(kept.texture).resource.Get();
    const D3D12_RESOURCE_BARRIER drawn = transition(resource, layer, kShaderRead, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commands_->ResourceBarrier(1, &drawn);
    const D3D12_CPU_DESCRIPTOR_HANDLE view = device_.context().renderTargets().cpu(kept.layerViews[layer]);
    const float black[4] = {0.f, 0.f, 0.f, 0.f};
    commands_->ClearRenderTargetView(view, black, 0, nullptr);
    commands_->OMSetRenderTargets(1, &view, FALSE, nullptr);
    setViewport(Rect{0, 0, kept.width, kept.height});
    layerTarget_ = target;
    layer_ = layer;
}

void D3D12CommandList::endLayerPass()
{
    const D3D12LayerTarget &kept = device_.layerTargetOf(layerTarget_);
    ID3D12Resource *resource = device_.texture(kept.texture).resource.Get();
    const D3D12_RESOURCE_BARRIER readable = transition(resource, layer_, D3D12_RESOURCE_STATE_RENDER_TARGET, kShaderRead);
    commands_->ResourceBarrier(1, &readable);
}

// the family's state object for the draw state, its primitives and vertex stride, the base vertex / instance
void D3D12CommandList::prepareDraw(int32_t baseVertex, uint32_t baseInstance)
{
    if (stateChanged_)
    {
        commands_->SetPipelineState(device_.pipelineState(pipeline_, state_));
        commands_->IASetPrimitiveTopology(device_.topologyOf(pipeline_));
        stateChanged_ = false;
    }
    if (verticesChanged_)
    {
        const D3D12_VERTEX_BUFFER_VIEW view = device_.vertexView(vertices_, device_.vertexStride(pipeline_));
        commands_->IASetVertexBuffers(0, 1, &view);
        verticesChanged_ = false;
    }
    const int32_t vertexInfo[2] = {baseVertex, int32_t(baseInstance)};
    commands_->SetGraphicsRoot32BitConstants(kVertexInfo, 2, vertexInfo, 0);
}

void D3D12CommandList::draw(const DrawRange &range)
{
    prepareDraw(0, 0);
    commands_->DrawInstanced(range.count, 1, range.first, 0);
}

void D3D12CommandList::drawIndexed(const IndexedDraw &draw)
{
    prepareDraw(draw.vertexOffset, draw.firstInstance);
    commands_->DrawIndexedInstanced(draw.count, 1, draw.firstIndex, draw.vertexOffset, draw.firstInstance);
}

void D3D12CommandList::beginQuery(const OcclusionQuery &query)
{
    commands_->BeginQuery(device_.queryHeap(query.pool), query.precise ? D3D12_QUERY_TYPE_OCCLUSION : D3D12_QUERY_TYPE_BINARY_OCCLUSION, query.index);
}

void D3D12CommandList::endQuery(const OcclusionQuery &query)
{
    commands_->EndQuery(device_.queryHeap(query.pool), query.precise ? D3D12_QUERY_TYPE_OCCLUSION : D3D12_QUERY_TYPE_BINARY_OCCLUSION, query.index);
    device_.queryEnded(query);
}

// Direct3D 12's queries need no reset
void D3D12CommandList::resetQueries(const QueryRange &range)
{
    (void)range;
}

}  // namespace argon::gfx::d3d12
