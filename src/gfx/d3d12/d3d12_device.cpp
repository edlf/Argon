// Argon: gfx::Device over Direct3D 12 (gfx\d3d12\d3d12_device.hpp).
#include "gfx/d3d12/d3d12_device.hpp"
#include "gfx/d3d12/d3d12_shaders.hpp"
#include "model/cmm.hpp"
#include "track/marker_lines.hpp"

#include <algorithm>
#include <cstring>

namespace argon::gfx::d3d12
{
namespace
{
const uint32_t kWireframeKey = 1u << 20;
const uint64_t kQueryNeverResolved = UINT64_MAX, kQueryResolvedUnsubmitted = UINT64_MAX - 1;
// uploadBuffer: up to this size a buffer takes a range of a shared block (D3D12Device::placeInBlock)
const uint64_t kSharedBufferMaxBytes = 256 * 1024, kSharedBufferAlignment = 256, kBufferBlockBytes = 8 * 1024 * 1024;
// a scene set's table: the constants, then its views t1..t4 (the frame copy, the OIT targets, the mesh draw records)
const UINT kSceneViews = 4;
const uint32_t kFrameCopySlot = 1, kOitAccumulatedSlot = 2, kOitRevealageSlot = 3, kMeshDrawsSlot = 4;
enum RootParameter : UINT
{
    kPush,
    kVertexInfo,
    kTextures,
    kSamplers,
    kScene,
    kMaterials
};

struct ShaderBytes
{
    D3D12_SHADER_BYTECODE vertex, pixel;
};

template <size_t N> D3D12_SHADER_BYTECODE bytecode(const unsigned char (&blob)[N])
{
    return D3D12_SHADER_BYTECODE{blob, N};
}

ShaderBytes shadersOf(ShaderProgram program, uint32_t sceneSamples)
{
    switch (program)
    {
    case ShaderProgram::Mesh:
        break;
    case ShaderProgram::Line:
        return {bytecode(kLineVertDxil), bytecode(kLineFragDxil)};
    case ShaderProgram::Water:
        return {bytecode(kWaterVertDxil), bytecode(kWaterFragDxil)};
    case ShaderProgram::Trail:
        return {bytecode(kTrailVertDxil), bytecode(kTrailFragDxil)};
    case ShaderProgram::Spray:
        return {bytecode(kTrailVertDxil), bytecode(kSprayFragDxil)};
    case ShaderProgram::QuadSprite:
        return {bytecode(kQuadSpriteVertDxil), bytecode(kQuadSpriteFragDxil)};
    case ShaderProgram::Occlusion:
        return {bytecode(kOcclVertDxil), bytecode(kOcclFragDxil)};
    case ShaderProgram::ShadowCast:
        return {bytecode(kMeshVertDxil), bytecode(kShadowCastFragDxil)};
    case ShaderProgram::ShadowSolid:
        return {bytecode(kMeshVertDxil), bytecode(kShadowSolidFragDxil)};
    case ShaderProgram::ShadowReceive:
        return {bytecode(kShadowVertDxil), bytecode(kShadowFragDxil)};
    case ShaderProgram::Fxaa:
        return {bytecode(kFullscreenVertDxil), bytecode(kFxaaFragDxil)};
    case ShaderProgram::SmaaEdges:
        return {bytecode(kFullscreenVertDxil), bytecode(kSmaaEdgesFragDxil)};
    case ShaderProgram::SmaaWeights:
        return {bytecode(kFullscreenVertDxil), bytecode(kSmaaWeightsFragDxil)};
    case ShaderProgram::SmaaBlend:
        return {bytecode(kFullscreenVertDxil), bytecode(kSmaaBlendFragDxil)};
    case ShaderProgram::MeshOit:
        return {bytecode(kMeshVertDxil), bytecode(kMeshOitFragDxil)};
    case ShaderProgram::MeshOitOpaque:
        return {bytecode(kMeshVertDxil), bytecode(kMeshOitOpaqueFragDxil)};
    case ShaderProgram::OitComposite:
        return {bytecode(kFullscreenVertDxil), sceneSamples > 1 ? bytecode(kOitCompositeMsFragDxil) : bytecode(kOitCompositeFragDxil)};
    }
    return {bytecode(kMeshVertDxil), bytecode(kMeshFragDxil)};
}

// SPIRV-Cross names a vertex input TEXCOORD<location>
const D3D12_INPUT_ELEMENT_DESC kMeshInputs[] = {
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 4, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
const D3D12_INPUT_ELEMENT_DESC kLineInputs[] = {
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

D3D12_BLEND blendOf(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::Zero:
        return D3D12_BLEND_ZERO;
    case BlendFactor::SrcColor:
        return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor:
        return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::SrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::DstAlpha:
        return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::OneMinusDstAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::DstColor:
        return D3D12_BLEND_DEST_COLOR;
    case BlendFactor::OneMinusDstColor:
        return D3D12_BLEND_INV_DEST_COLOR;
    default:
        return D3D12_BLEND_ONE;
    }
}

D3D12_BLEND_OP blendOpOf(BlendOp op)
{
    switch (op)
    {
    case BlendOp::Subtract:
        return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::ReverseSubtract:
        return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Min:
        return D3D12_BLEND_OP_MIN;
    case BlendOp::Max:
        return D3D12_BLEND_OP_MAX;
    default:
        return D3D12_BLEND_OP_ADD;
    }
}

// colour blended by the mesh state; alpha kept (the game's framebuffer alpha is unused)
D3D12_BLEND_DESC blendStateOf(const PipelineSpec &spec)
{
    D3D12_BLEND_DESC desc{};
    desc.AlphaToCoverageEnable = spec.blend && spec.blend->a2c;
    D3D12_RENDER_TARGET_BLEND_DESC &rt = desc.RenderTarget[0];
    rt.RenderTargetWriteMask = spec.topology == Topology::OcclusionPoints ? 0 : D3D12_COLOR_WRITE_ENABLE_ALL;
    rt.SrcBlend = rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlend = rt.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt.BlendOp = rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    if (spec.blend && spec.blend->blend)
    {
        rt.BlendEnable = TRUE;
        rt.SrcBlend = blendOf(spec.blend->src);
        rt.DestBlend = blendOf(spec.blend->dst);
        rt.BlendOp = blendOpOf(spec.blend->op);
        rt.SrcBlendAlpha = D3D12_BLEND_ZERO;
        rt.DestBlendAlpha = D3D12_BLEND_ONE;
    }
    return desc;
}

// RenderTarget::Oit (pipeline_spec.hpp): the accumulated colour and alpha add up, the revealage is multiplied by
// 1 - alpha
D3D12_BLEND_DESC oitBlendState()
{
    D3D12_BLEND_DESC desc{};
    desc.IndependentBlendEnable = TRUE;
    for (uint32_t target = 0; target < kOitTargetCount; ++target)
    {
        D3D12_RENDER_TARGET_BLEND_DESC &rt = desc.RenderTarget[target];
        rt.BlendEnable = TRUE;
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        rt.BlendOp = rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rt.LogicOp = D3D12_LOGIC_OP_NOOP;
    }
    D3D12_RENDER_TARGET_BLEND_DESC &accumulated = desc.RenderTarget[0], &revealage = desc.RenderTarget[1];
    accumulated.SrcBlend = accumulated.DestBlend = accumulated.SrcBlendAlpha = accumulated.DestBlendAlpha = D3D12_BLEND_ONE;
    revealage.SrcBlend = revealage.SrcBlendAlpha = D3D12_BLEND_ZERO;
    revealage.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
    revealage.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    return desc;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyTypeOf(Topology topology)
{
    if (topology == Topology::OcclusionPoints)
    {
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    }
    return topology == Topology::Lines ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

DXGI_FORMAT formatOf(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::Bc1:
        return DXGI_FORMAT_BC1_UNORM;
    case TextureFormat::Bc2:
        return DXGI_FORMAT_BC2_UNORM;
    case TextureFormat::Bc3:
        return DXGI_FORMAT_BC3_UNORM;
    case TextureFormat::Bgra8:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case TextureFormat::Rgba8:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
        throw std::runtime_error("Direct3D 12: a texture format it does not take (" + std::to_string(int(format)) + ")");
    }
}

D3D12_TEXTURE_ADDRESS_MODE addressOf(SamplerMode mode, int clampBit)
{
    return (int(mode) & clampBit) ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

D3D12_RESOURCE_BARRIER transition(ID3D12Resource *resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = from;
    barrier.Transition.StateAfter = to;
    return barrier;
}

const D3D12_RESOURCE_STATES kShaderRead = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
}  // namespace

void D3D12Device::create(D3D12Context &context, const SceneTargets &targets, uint32_t pushConstantBytes)
{
    context_ = &context;
    targets_ = targets;
    createRootSignature(pushConstantBytes);
    createSamplerTables();
}

void D3D12Device::createRootSignature(uint32_t pushConstantBytes)
{
    const D3D12_DESCRIPTOR_RANGE textures{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    const D3D12_DESCRIPTOR_RANGE samplers{D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    const D3D12_DESCRIPTOR_RANGE scene[2] = {{D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND},
                                             {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kSceneViews, 1, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND}};
    const D3D12_DESCRIPTOR_RANGE materials{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND};
    D3D12_ROOT_PARAMETER params[6]{};
    params[kPush].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[kPush].Constants = {0, 0, pushConstantBytes / 4};
    params[kVertexInfo].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[kVertexInfo].Constants = {1, 0, 2};
    params[kTextures].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[kTextures].DescriptorTable = {1, &textures};
    params[kSamplers].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[kSamplers].DescriptorTable = {1, &samplers};
    params[kScene].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[kScene].DescriptorTable = {2, scene};
    params[kMaterials].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[kMaterials].DescriptorTable = {1, &materials};
    D3D12_STATIC_SAMPLER_DESC frameCopy{};   // the frame copy's (s1, space 1)
    frameCopy.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    frameCopy.AddressU = frameCopy.AddressV = frameCopy.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    frameCopy.MaxLOD = D3D12_FLOAT32_MAX;
    frameCopy.ShaderRegister = 1;
    frameCopy.RegisterSpace = 1;
    frameCopy.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC desc{6, params, 1, &frameCopy, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    ComPtr<ID3DBlob> blob, error;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)))
    {
        throw std::runtime_error(std::string("Direct3D 12: root signature: ") + static_cast<const char *>(error->GetBufferPointer()));
    }
    check(context_->device()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)), "CreateRootSignature");
}

// every combination of the four stages' SamplerModes as a table of four (combination s0 + 4 s1 + 16 s2 + 64 s3), and
// one sampler per mode
void D3D12Device::createSamplerTables()
{
    DescriptorHeap &heap = context_->samplers();
    samplerTables_ = heap.reserve(256 * 4);
    singleSamplers_ = heap.reserve(4);
    D3D12_SAMPLER_DESC desc{};
    desc.Filter = D3D12_FILTER_ANISOTROPIC;
    desc.MaxAnisotropy = 8;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.MaxLOD = 16.f;
    for (uint32_t i = 0; i < 256 * 4 + 4; ++i)
    {
        const SamplerMode mode = i < 256 * 4 ? SamplerMode((i / 4 >> (2 * (i % 4))) & 3) : SamplerMode(i - 256 * 4);
        desc.AddressU = addressOf(mode, 1);
        desc.AddressV = addressOf(mode, 2);
        context_->device()->CreateSampler(&desc, heap.cpu(samplerTables_ + i));
    }
}

void D3D12Device::destroy()
{
    families_.clear();
    meshPipelines_.clear();
    buffers_.clear();
    bufferBlocks_.clear();
    for (auto &[ref, texture] : textures_)
    {
        context_->cpuViews().give(texture.cpuView);
    }
    textures_.clear();
    layerTargets_.clear();
    queries_.clear();
    rootSignature_.Reset();
}

// ------------------------------------------------------------------ pipelines
Pipeline D3D12Device::createPipeline(const PipelineSpec &spec)
{
    auto family = std::make_unique<Family>();
    family->spec = spec;
    if (spec.blend)
    {
        family->blend = *spec.blend;
        family->spec.blend = &family->blend;
    }
    for (uint32_t location = 0; location < 5; ++location)
    {
        if (vertexInputsOf(spec.program) & (1u << location))
        {
            family->inputs.push_back(kMeshInputs[location]);
        }
    }
    families_.push_back(std::move(family));
    return Pipeline{families_.size()};
}

Pipeline D3D12Device::meshPipeline(const MeshState &state, Fill fill)
{
    const uint32_t key = state.pipelineKey() | (fill == Fill::Wireframe ? kWireframeKey : 0u);
    const auto known = meshPipelines_.find(key);
    if (known != meshPipelines_.end())
    {
        return known->second;
    }
    PipelineSpec spec;
    spec.fill = fill;
    spec.blend = state.blend || state.a2c ? &state : nullptr;
    return meshPipelines_[key] = createPipeline(spec);
}

ID3D12PipelineState *D3D12Device::pipelineState(Pipeline pipeline, const DrawState &state)
{
    Family &family = *families_[pipeline.value - 1];
    ComPtr<ID3D12PipelineState> &made = family.states[state.key()];
    if (!made)
    {
        made = buildState(family, state);
    }
    return made.Get();
}

ComPtr<ID3D12PipelineState> D3D12Device::buildState(const Family &family, const DrawState &state) const
{
    const PipelineSpec &spec = family.spec;
    const bool layer = spec.target == RenderTarget::Layer, lines = spec.topology == Topology::Lines, oit = spec.target == RenderTarget::Oit;
    const ShaderBytes shaders = shadersOf(spec.program, targets_.samples);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_.Get();
    desc.VS = shaders.vertex;
    desc.PS = shaders.pixel;
    desc.BlendState = oit ? oitBlendState() : blendStateOf(spec);
    desc.SampleMask = UINT_MAX;
    desc.RasterizerState.FillMode = spec.fill == Fill::Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = state.depthCull.cullBack ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    desc.RasterizerState.FrontCounterClockwise = state.front == Winding::CounterClockwise;
    desc.RasterizerState.DepthClipEnable = TRUE;
    desc.RasterizerState.MultisampleEnable = !layer && targets_.samples > 1;
    desc.DepthStencilState.DepthEnable = !layer && state.depthCull.test;
    desc.DepthStencilState.DepthWriteMask = state.depthCull.write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthStencilState.DepthFunc = spec.depth == DepthCompare::Always ? D3D12_COMPARISON_FUNC_ALWAYS : D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    desc.InputLayout = lines ? D3D12_INPUT_LAYOUT_DESC{kLineInputs, 2} : D3D12_INPUT_LAYOUT_DESC{family.inputs.data(), UINT(family.inputs.size())};
    desc.PrimitiveTopologyType = topologyTypeOf(spec.topology);
    desc.NumRenderTargets = oit ? kOitTargetCount : 1;
    desc.RTVFormats[0] = layer ? DXGI_FORMAT_R8G8B8A8_UNORM : oit ? kOitAccumulatedFormat : targets_.colour;
    desc.RTVFormats[1] = oit ? kOitRevealageFormat : DXGI_FORMAT_UNKNOWN;
    desc.DSVFormat = layer ? DXGI_FORMAT_UNKNOWN : targets_.depth;
    desc.SampleDesc.Count = layer ? 1 : targets_.samples;
    ComPtr<ID3D12PipelineState> made;
    check(context_->device()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&made)), "CreateGraphicsPipelineState");
    return made;
}

D3D_PRIMITIVE_TOPOLOGY D3D12Device::topologyOf(Pipeline pipeline) const
{
    const Topology topology = families_[pipeline.value - 1]->spec.topology;
    if (topology == Topology::OcclusionPoints)
    {
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    }
    return topology == Topology::Lines ? D3D_PRIMITIVE_TOPOLOGY_LINELIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

uint32_t D3D12Device::vertexStride(Pipeline pipeline) const
{
    return families_[pipeline.value - 1]->spec.topology == Topology::Lines ? uint32_t(sizeof(MarkerVertex)) : uint32_t(sizeof(Vertex));
}

// ------------------------------------------------------------------ the GPU's work
void D3D12Device::waitForIdle()
{
    context_->waitIdle();
}

void D3D12Device::beginUploadBatch()
{
    context_->beginUploadBatch();
}

void D3D12Device::submitUploadBatch()
{
    context_->submitUploadBatch();
}

std::optional<uint64_t> D3D12Device::videoMemoryInUse() const
{
    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if (FAILED(context_->adapter()->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
    {
        return std::nullopt;
    }
    return info.CurrentUsage;
}

uint32_t D3D12Device::uploadRowPitch(uint32_t width) const
{
    const uint32_t align = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    return (width * 4 + align - 1) / align * align;
}

// ------------------------------------------------------------------ buffers
BufferRef D3D12Device::keep(Buffer buffer)
{
    const BufferRef ref{nextBuffer_++};
    buffers_[ref.value] = std::move(buffer);
    return ref;
}

// constant buffers in 256-byte steps
BufferRef D3D12Device::createStreamBuffer(BufferUse use, size_t bytes)
{
    Buffer buffer;
    buffer.bytes = use == BufferUse::Uniform ? (bytes + 255) / 256 * 256 : bytes;
    buffer.resource = context_->createBuffer(D3D12_HEAP_TYPE_UPLOAD, buffer.bytes);
    const D3D12_RANGE nothingRead{0, 0};
    check(buffer.resource->Map(0, &nothingRead, &buffer.mapped), "Map");
    return keep(std::move(buffer));
}

// small ones share blocks (placeInBlock), larger ones get their own resource
BufferRef D3D12Device::uploadBuffer(BufferUse use, std::span<const std::byte> data)
{
    (void)use;
    Buffer buffer;
    if (data.size() <= kSharedBufferMaxBytes)
    {
        buffer = placeInBlock(data.size());
    }
    else
    {
        buffer.bytes = data.size();
        buffer.resource = context_->createBuffer(D3D12_HEAP_TYPE_DEFAULT, buffer.bytes);
    }
    const ComPtr<ID3D12Resource> staging = context_->createBuffer(D3D12_HEAP_TYPE_UPLOAD, std::max<uint64_t>(buffer.bytes, 1));
    void *mapped = nullptr;
    check(staging->Map(0, nullptr, &mapped), "Map");
    std::memcpy(mapped, data.data(), data.size());
    staging->Unmap(0, nullptr);
    ID3D12GraphicsCommandList *commands = context_->beginUpload();
    commands->CopyBufferRegion(buffer.resource.Get(), buffer.offset, staging.Get(), 0, buffer.bytes);
    context_->keepUntilUploaded(staging);
    context_->endUpload();
    return keep(std::move(buffer));
}

// a range of the first block with room, else of a new block. 256-byte aligned: a constant buffer's start, and more
// than a raw view's 16 or a vertex / index buffer's 4 need
D3D12Device::Buffer D3D12Device::placeInBlock(uint64_t bytes)
{
    Buffer buffer;
    buffer.bytes = bytes;
    buffer.reserved = std::max<uint64_t>((bytes + kSharedBufferAlignment - 1) / kSharedBufferAlignment * kSharedBufferAlignment, kSharedBufferAlignment);
    for (size_t b = 0; b < bufferBlocks_.size(); ++b)
    {
        if (const std::optional<uint64_t> offset = bufferBlocks_[b].ranges.allocate(buffer.reserved, kSharedBufferAlignment))
        {
            buffer.resource = bufferBlocks_[b].resource;
            buffer.offset = *offset;
            buffer.block = int(b);
            return buffer;
        }
    }
    BufferBlock block{context_->createBuffer(D3D12_HEAP_TYPE_DEFAULT, kBufferBlockBytes), RangeAllocator(kBufferBlockBytes)};
    buffer.offset = *block.ranges.allocate(buffer.reserved, kSharedBufferAlignment);
    buffer.resource = block.resource;
    buffer.block = int(bufferBlocks_.size());
    bufferBlocks_.push_back(std::move(block));
    return buffer;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Device::gpuAddress(const Buffer &buffer)
{
    return buffer.resource->GetGPUVirtualAddress() + buffer.offset;
}

void *D3D12Device::mapped(BufferRef buffer) const
{
    return buffers_.at(buffer.value).mapped;
}

// a shared block's range is free for the next buffer at once: as with a resource of its own, the caller has waited
// for the frames that read it
void D3D12Device::destroyBuffer(BufferRef &buffer)
{
    const auto kept = buffers_.find(buffer.value);
    if (kept != buffers_.end())
    {
        if (kept->second.block >= 0)
        {
            bufferBlocks_[size_t(kept->second.block)].ranges.release(kept->second.offset, kept->second.reserved);
        }
        buffers_.erase(kept);
    }
    buffer = {};
}

ID3D12Resource *D3D12Device::bufferResource(BufferRef buffer) const
{
    return buffers_.at(buffer.value).resource.Get();
}

D3D12_VERTEX_BUFFER_VIEW D3D12Device::vertexView(BufferRef buffer, uint32_t stride) const
{
    const Buffer &kept = buffers_.at(buffer.value);
    return D3D12_VERTEX_BUFFER_VIEW{gpuAddress(kept), UINT(kept.bytes), stride};
}

D3D12_INDEX_BUFFER_VIEW D3D12Device::indexView(BufferRef buffer) const
{
    const Buffer &kept = buffers_.at(buffer.value);
    return D3D12_INDEX_BUFFER_VIEW{gpuAddress(kept), UINT(kept.bytes), DXGI_FORMAT_R32_UINT};
}

// ------------------------------------------------------------------ textures
TextureRef D3D12Device::keepTexture(D3D12Texture texture)
{
    const TextureRef ref{nextTexture_++};
    textures_[ref.value] = std::move(texture);
    return ref;
}

D3D12Texture &D3D12Device::texture(TextureRef texture)
{
    return textures_.at(texture.value);
}

// every layer and level through the upload list, then readable by the shaders; the source's rows are tightly packed
void D3D12Device::uploadTexture(const argon::TextureData &data, ID3D12Resource *resource)
{
    const D3D12_RESOURCE_DESC desc = resource->GetDesc();
    const UINT levels = UINT(data.levels.size()), subresources = levels * data.layers;
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(subresources);
    std::vector<UINT> rows(subresources);
    std::vector<UINT64> rowBytes(subresources);
    UINT64 total = 0;
    context_->device()->GetCopyableFootprints(&desc, 0, subresources, 0, footprints.data(), rows.data(), rowBytes.data(), &total);
    const ComPtr<ID3D12Resource> staging = context_->createBuffer(D3D12_HEAP_TYPE_UPLOAD, total);
    uint8_t *mapped = nullptr;
    check(staging->Map(0, nullptr, reinterpret_cast<void **>(&mapped)), "Map");
    ID3D12GraphicsCommandList *commands = context_->beginUpload();
    for (UINT i = 0; i < subresources; ++i)
    {
        const UINT level = i % levels, layer = i / levels;
        const uint8_t *source = data.bytes.data() + layer * data.layerStride + data.levels[level].offset;
        const size_t sourcePitch = data.levels[level].size / rows[i];
        for (UINT row = 0; row < rows[i]; ++row)
        {
            std::memcpy(mapped + footprints[i].Offset + row * footprints[i].Footprint.RowPitch, source + row * sourcePitch, size_t(rowBytes[i]));
        }
        D3D12_TEXTURE_COPY_LOCATION to{resource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        to.SubresourceIndex = i;
        D3D12_TEXTURE_COPY_LOCATION from{staging.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint = footprints[i];
        commands->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
    }
    staging->Unmap(0, nullptr);
    const D3D12_RESOURCE_BARRIER readable = transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, kShaderRead);
    commands->ResourceBarrier(1, &readable);
    context_->keepUntilUploaded(staging);
    context_->endUpload();
}

Texture D3D12Device::createTexture(const argon::TextureData &data)
{
    if (data.levels.empty())
    {
        return {};
    }
    D3D12Texture made;
    made.format = formatOf(data.format);
    made.width = data.width;
    made.height = data.height;
    made.levels = uint32_t(data.levels.size());
    made.layers = data.layers;
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = data.width;
    desc.Height = data.height;
    desc.DepthOrArraySize = UINT16(data.layers);
    desc.MipLevels = UINT16(made.levels);
    desc.Format = made.format;
    desc.SampleDesc.Count = 1;
    ID3D12Device *device = context_->device();
    check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&made.resource)),
          "CreateCommittedResource (texture)");
    uploadTexture(data, made.resource.Get());
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = made.format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2DArray.MipLevels = made.levels;
    view.Texture2DArray.ArraySize = made.layers;
    made.cpuView = context_->cpuViews().take();
    context_->device()->CreateShaderResourceView(made.resource.Get(), &view, context_->cpuViews().cpu(made.cpuView));
    Texture texture;
    texture.handle = keepTexture(std::move(made));
    texture.valid = true;
    texture.cube = data.cube;
    texture.layers = data.layers;
    texture.alpha = data.hasAlpha;
    texture.cutout = data.looksLikeCutout();
    return texture;
}

void D3D12Device::destroyTexture(Texture &texture)
{
    const auto kept = textures_.find(texture.handle.value);
    if (kept != textures_.end())
    {
        context_->cpuViews().give(kept->second.cpuView);
        textures_.erase(kept);
    }
    texture = {};
}

TextureRef D3D12Device::adoptTexture(ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC &view)
{
    D3D12Texture adopted;
    adopted.resource = resource;
    adopted.owned = false;
    adopted.format = view.Format;
    adopted.cpuView = context_->cpuViews().take();
    context_->device()->CreateShaderResourceView(resource, &view, context_->cpuViews().cpu(adopted.cpuView));
    return keepTexture(std::move(adopted));
}

void D3D12Device::forgetTexture(TextureRef texture)
{
    Texture forgotten;
    forgotten.handle = texture;
    destroyTexture(forgotten);
}

// ------------------------------------------------------------------ layer targets
// cleared to black once, then left readable
LayerTarget D3D12Device::createLayerTarget(const LayerTargetSize &size)
{
    D3D12Texture made;
    made.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    made.width = size.width;
    made.height = size.height;
    made.layers = size.layers;
    D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = size.width;
    desc.Height = size.height;
    desc.DepthOrArraySize = UINT16(size.layers);
    desc.MipLevels = 1;
    desc.Format = made.format;
    desc.SampleDesc.Count = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE black{made.format};
    black.Color[3] = 0.f;
    ID3D12Device *device = context_->device();
    check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &black, IID_PPV_ARGS(&made.resource)),
          "CreateCommittedResource (layer target)");
    D3D12LayerTarget target;
    target.width = size.width;
    target.height = size.height;
    ID3D12GraphicsCommandList *commands = context_->beginUpload();
    for (uint32_t i = 0; i < size.layers; ++i)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtv{made.format, D3D12_RTV_DIMENSION_TEXTURE2DARRAY};
        rtv.Texture2DArray = {0, i, 1, 0};
        const uint32_t index = context_->renderTargets().take();
        context_->device()->CreateRenderTargetView(made.resource.Get(), &rtv, context_->renderTargets().cpu(index));
        commands->ClearRenderTargetView(context_->renderTargets().cpu(index), black.Color, 0, nullptr);
        target.layerViews.push_back(index);
    }
    const D3D12_RESOURCE_BARRIER readable = transition(made.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, kShaderRead);
    commands->ResourceBarrier(1, &readable);
    context_->endUpload();
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = made.format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2DArray.MipLevels = 1;
    view.Texture2DArray.ArraySize = size.layers;
    made.cpuView = context_->cpuViews().take();
    context_->device()->CreateShaderResourceView(made.resource.Get(), &view, context_->cpuViews().cpu(made.cpuView));
    target.texture = keepTexture(std::move(made));
    layerTargets_.push_back(std::move(target));
    return LayerTarget{layerTargets_.size()};
}

// its slot stays (the handles are indices), empty
void D3D12Device::destroyLayerTarget(LayerTarget &target)
{
    if (!target)
    {
        return;
    }
    D3D12LayerTarget &kept = layerTargets_[target.value - 1];
    for (const uint32_t view : kept.layerViews)
    {
        context_->renderTargets().give(view);
    }
    forgetTexture(kept.texture);
    kept = {};
    target = {};
}

Texture D3D12Device::layerTexture(LayerTarget target) const
{
    const D3D12LayerTarget &kept = layerTargetOf(target);
    Texture texture;
    texture.handle = kept.texture;
    texture.valid = true;
    texture.layers = uint32_t(kept.layerViews.size());
    return texture;
}

const D3D12LayerTarget &D3D12Device::layerTargetOf(LayerTarget target) const
{
    return layerTargets_[target.value - 1];
}

// ------------------------------------------------------------------ sets
SetPool D3D12Device::createSetPool(const SetPoolSize &size)
{
    Pool pool;
    pool.count = size.sets * 4;
    pool.first = context_->shaderViews().reserve(pool.count);
    pools_.push_back(pool);
    return SetPool{pools_.size()};
}

void D3D12Device::resetSetPool(SetPool pool)
{
    pools_[pool.value - 1].used = 0;
}

uint32_t D3D12Device::takeFromPool(SetPool pool, uint32_t descriptors)
{
    Pool &kept = pools_[pool.value - 1];
    if (kept.used + descriptors > kept.count)
    {
        throw std::runtime_error("Direct3D 12: a set pool is full");
    }
    const uint32_t first = kept.first + kept.used;
    kept.used += descriptors;
    return first;
}

// the value: the first view's index << 8 | the sampler combination
TextureSet D3D12Device::createTextureSet(SetPool pool, const TextureSetDesc &desc)
{
    const uint32_t first = takeFromPool(pool, 4);
    uint32_t combination = 0;
    for (uint32_t i = 0; i < 4; ++i)
    {
        const D3D12Texture &source = texture(desc.textures[i]);
        context_->device()->CopyDescriptorsSimple(1, context_->shaderViews().cpu(first + i), context_->cpuViews().cpu(source.cpuView),
                                                  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        combination |= uint32_t(desc.samplers[i]) << (2 * i);
    }
    return TextureSet{uint64_t(first) << 8 | combination};
}

MaterialSet D3D12Device::createMaterialSet(SetPool pool, BufferRef materials)
{
    const uint32_t index = takeFromPool(pool, 1);
    createStorageView(materials, index);
    return MaterialSet{index};
}

// a storage buffer as the shaders read it (SPIRV-Cross: a ByteAddressBuffer), into the shader-visible heap at `index`
void D3D12Device::createStorageView(BufferRef storage, uint32_t index)
{
    const Buffer &buffer = buffers_.at(storage.value);
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = DXGI_FORMAT_R32_TYPELESS;
    view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Buffer.FirstElement = buffer.offset / 4;
    view.Buffer.NumElements = UINT(buffer.bytes / 4);
    view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    context_->device()->CreateShaderResourceView(buffer.resource.Get(), &view, context_->shaderViews().cpu(index));
}

// the OIT targets' slots hold null views until useOitTargets
SceneSet D3D12Device::createSceneSet(BufferRef constants, BufferRef meshDraws, TextureRef frameCopy)
{
    const uint32_t first = context_->shaderViews().reserve(1 + kSceneViews);
    const Buffer &buffer = buffers_.at(constants.value);
    const D3D12_CONSTANT_BUFFER_VIEW_DESC view{gpuAddress(buffer), UINT(buffer.bytes)};
    context_->device()->CreateConstantBufferView(&view, context_->shaderViews().cpu(first));
    D3D12_SHADER_RESOURCE_VIEW_DESC nothing{DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING};
    nothing.Texture2D.MipLevels = 1;
    for (const uint32_t slot : {kOitAccumulatedSlot, kOitRevealageSlot})
    {
        context_->device()->CreateShaderResourceView(nullptr, &nothing, context_->shaderViews().cpu(first + slot));
    }
    createStorageView(meshDraws, first + kMeshDrawsSlot);
    const SceneSet set{first};
    useFrameCopy(set, frameCopy);
    return set;
}

void D3D12Device::useFrameCopy(SceneSet set, TextureRef frameCopy)
{
    useSceneTexture(set, kFrameCopySlot, frameCopy);
}

void D3D12Device::useOitTargets(SceneSet set, const OitTargets &targets)
{
    useSceneTexture(set, kOitAccumulatedSlot, targets.accumulated);
    useSceneTexture(set, kOitRevealageSlot, targets.revealage);
}

void D3D12Device::useSceneTexture(SceneSet set, uint32_t slot, TextureRef view)
{
    context_->device()->CopyDescriptorsSimple(1, context_->shaderViews().cpu(uint32_t(set.value) + slot), context_->cpuViews().cpu(texture(view).cpuView),
                                              D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::textureTable(TextureSet set) const
{
    return context_->shaderViews().gpu(uint32_t(set.value >> 8));
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::samplerTable(TextureSet set) const
{
    return context_->samplers().gpu(samplerTables_ + uint32_t(set.value & 0xff) * 4);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::setTable(uint64_t setValue) const
{
    return context_->shaderViews().gpu(uint32_t(setValue));
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Device::samplerOf(SamplerMode mode) const
{
    return context_->samplers().gpu(singleSamplers_ + uint32_t(mode));
}

// ------------------------------------------------------------------ occlusion queries
QueryPool D3D12Device::createOcclusionQueries(uint32_t count)
{
    Queries queries;
    const D3D12_QUERY_HEAP_DESC desc{D3D12_QUERY_HEAP_TYPE_OCCLUSION, count, 0};
    check(context_->device()->CreateQueryHeap(&desc, IID_PPV_ARGS(&queries.heap)), "CreateQueryHeap");
    queries.results = context_->createBuffer(D3D12_HEAP_TYPE_READBACK, uint64_t(count) * 8);
    queries.resolvedAt.assign(count, kQueryNeverResolved);
    queries_.push_back(std::move(queries));
    return QueryPool{queries_.size()};
}

ID3D12QueryHeap *D3D12Device::queryHeap(QueryPool pool) const
{
    return queries_[pool.value - 1].heap.Get();
}

// the frame's queries are numbered one after another: their results are resolved together (resolveEndedQueries)
void D3D12Device::queryEnded(const OcclusionQuery &query)
{
    Queries &queries = queries_[query.pool.value - 1];
    queries.endedType = query.precise ? D3D12_QUERY_TYPE_OCCLUSION : D3D12_QUERY_TYPE_BINARY_OCCLUSION;
    if (queries.endedFirst == queries.endedEnd)
    {
        queries.endedFirst = query.index;
        queries.endedEnd = query.index + 1;
        return;
    }
    queries.endedFirst = std::min(queries.endedFirst, query.index);
    queries.endedEnd = std::max(queries.endedEnd, query.index + 1);
}

// one ResolveQueryData a pool for the queries ended since the last frame was recorded
void D3D12Device::resolveEndedQueries(ID3D12GraphicsCommandList *commands)
{
    for (Queries &queries : queries_)
    {
        if (queries.endedFirst == queries.endedEnd)
        {
            continue;
        }
        const UINT count = queries.endedEnd - queries.endedFirst;
        commands->ResolveQueryData(queries.heap.Get(), queries.endedType, queries.endedFirst, count, queries.results.Get(), uint64_t(queries.endedFirst) * 8);
        std::fill_n(queries.resolvedAt.begin() + queries.endedFirst, count, kQueryResolvedUnsubmitted);
        queries.endedFirst = queries.endedEnd = 0;
    }
}

void D3D12Device::framesSubmitted(uint64_t fence)
{
    for (Queries &queries : queries_)
    {
        std::replace(queries.resolvedAt.begin(), queries.resolvedAt.end(), kQueryResolvedUnsubmitted, fence);
    }
}

bool D3D12Device::readQueries(const QueryRange &range, std::span<uint64_t> results) const
{
    const Queries &queries = queries_[range.pool.value - 1];
    for (uint32_t i = range.first; i < range.first + range.count; ++i)
    {
        if (queries.resolvedAt[i] >= kQueryResolvedUnsubmitted || !context_->reached(queries.resolvedAt[i]))
        {
            return false;
        }
    }
    const D3D12_RANGE read{size_t(range.first) * 8, size_t(range.first + range.count) * 8};
    uint8_t *mapped = nullptr;
    check(queries.results->Map(0, &read, reinterpret_cast<void **>(&mapped)), "Map (queries)");
    std::memcpy(results.data(), mapped + read.Begin, size_t(range.count) * 8);
    const D3D12_RANGE nothingWritten{0, 0};
    queries.results->Unmap(0, &nothingWritten);
    return true;
}

}  // namespace argon::gfx::d3d12
