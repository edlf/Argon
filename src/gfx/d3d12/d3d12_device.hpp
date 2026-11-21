// Argon: gfx::Device over Direct3D 12. One root signature for every program, matching the cross-compiled registers
// (gfx\d3d12\d3d12_shaders.hpp): 0 the push constants (b0), 1 the base vertex / instance (b1), 2 the four
// stage textures (t0..t3) and 3 their samplers (s0..s3), 4 the scene constants, frame copy, OIT targets and mesh draw
// records (b0 / t1 / t2, t3 / t4, space 1; the frame copy's sampler static), 5 the materials (t0, space 2). A
// gfx::Pipeline is a family whose pipeline state objects are made per draw state (depth test / write, culling, front
// face), which Vulkan sets dynamically.
#pragma once
#include "gfx/d3d12/d3d12_context.hpp"
#include "gfx/device.hpp"
#include "gfx/range_allocator.hpp"

#include <array>
#include <map>
#include <memory>
#include <unordered_map>

namespace argon::gfx::d3d12
{

// the state a family's pipeline state objects differ in
struct DrawState
{
    DepthCull depthCull;
    Winding front = Winding::CounterClockwise;

    static constexpr size_t kKeyCount = 16;   // key(): 4 bits
    uint32_t key() const
    {
        return uint32_t(depthCull.test) | uint32_t(depthCull.write) << 1 | uint32_t(depthCull.cullBack) << 2 | uint32_t(front == Winding::Clockwise) << 3;
    }
};

// the OIT targets' formats (RenderTarget::Oit; the presenter keeps the targets)
inline constexpr DXGI_FORMAT kOitAccumulatedFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
inline constexpr DXGI_FORMAT kOitRevealageFormat = DXGI_FORMAT_R16_FLOAT;

// the scene pass's targets: their formats and samples
struct SceneTargets
{
    DXGI_FORMAT colour = DXGI_FORMAT_B8G8R8A8_UNORM, depth = DXGI_FORMAT_D32_FLOAT;
    uint32_t samples = 1;
};

struct D3D12Texture
{
    ComPtr<ID3D12Resource> resource;
    uint32_t cpuView = 0;   // its shader resource view (all layers as a 2D array) in the CPU view heap
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t width = 0, height = 0, levels = 1, layers = 1;
    bool owned = true;   // false: a view of a resource the presenter keeps (a frame copy)
};

struct D3D12LayerTarget
{
    TextureRef texture;               // all its layers (the device's texture record)
    std::vector<uint32_t> layerViews; // render target views, one per layer
    uint32_t width = 0, height = 0;
};

class D3D12Device final : public Device
{
  public:
    // pushConstantBytes: the largest push block (the mesh shaders' PushConstants)
    void create(D3D12Context &context, const SceneTargets &targets, uint32_t pushConstantBytes);
    void destroy();

    Pipeline createPipeline(const PipelineSpec &spec) override;
    Pipeline meshPipeline(const MeshState &state, Fill fill) override;
    void waitForIdle() override;
    void beginUploadBatch() override;
    void submitUploadBatch() override;
    bool wireframeSupported() const override
    {
        return true;
    }
    std::optional<uint64_t> videoMemoryInUse() const override;
    uint32_t uploadRowPitch(uint32_t width) const override;
    BufferRef createStreamBuffer(BufferUse use, size_t bytes) override;
    BufferRef uploadBuffer(BufferUse use, std::span<const std::byte> data) override;
    void *mapped(BufferRef buffer) const override;
    void destroyBuffer(BufferRef &buffer) override;
    LayerTarget createLayerTarget(const LayerTargetSize &size) override;
    void destroyLayerTarget(LayerTarget &target) override;
    Texture layerTexture(LayerTarget target) const override;
    Texture createTexture(const argon::TextureData &data) override;
    void destroyTexture(Texture &texture) override;
    SetPool createSetPool(const SetPoolSize &size) override;
    void resetSetPool(SetPool pool) override;
    TextureSet createTextureSet(SetPool pool, const TextureSetDesc &desc) override;
    MaterialSet createMaterialSet(SetPool pool, BufferRef materials) override;
    SceneSet createSceneSet(BufferRef constants, BufferRef meshDraws, TextureRef frameCopy) override;
    void useFrameCopy(SceneSet set, TextureRef frameCopy) override;
    void useOitTargets(SceneSet set, const OitTargets &targets) override;
    QueryPool createOcclusionQueries(uint32_t count) override;
    bool readQueries(const QueryRange &range, std::span<uint64_t> results) const override;
    bool preciseOcclusion() const override
    {
        return true;
    }

    // for the command list, the presenter and the UI renderer
    D3D12Context &context() const
    {
        return *context_;
    }
    ID3D12RootSignature *rootSignature() const
    {
        return rootSignature_.Get();
    }
    ID3D12PipelineState *pipelineState(Pipeline pipeline, const DrawState &state);
    D3D_PRIMITIVE_TOPOLOGY topologyOf(Pipeline pipeline) const;
    uint32_t vertexStride(Pipeline pipeline) const;
    D3D12_VERTEX_BUFFER_VIEW vertexView(BufferRef buffer, uint32_t stride) const;
    D3D12_INDEX_BUFFER_VIEW indexView(BufferRef buffer) const;
    ID3D12Resource *bufferResource(BufferRef buffer) const;
    D3D12Texture &texture(TextureRef texture);
    const D3D12LayerTarget &layerTargetOf(LayerTarget target) const;
    D3D12_GPU_DESCRIPTOR_HANDLE textureTable(TextureSet set) const;
    D3D12_GPU_DESCRIPTOR_HANDLE samplerTable(TextureSet set) const;
    D3D12_GPU_DESCRIPTOR_HANDLE setTable(uint64_t setValue) const;   // a material or scene set's
    D3D12_GPU_DESCRIPTOR_HANDLE samplerOf(SamplerMode mode) const;   // a single sampler (the UI's images)
    ID3D12QueryHeap *queryHeap(QueryPool pool) const;
    void queryEnded(const OcclusionQuery &query);
    void resolveEndedQueries(ID3D12GraphicsCommandList *commands);
    // the queries resolved since the last call are read once the GPU reaches `fence`
    void framesSubmitted(uint64_t fence);
    // a resource the presenter keeps, seen through `view` (a frame copy); forgetTexture before it goes
    TextureRef adoptTexture(ID3D12Resource *resource, const D3D12_SHADER_RESOURCE_VIEW_DESC &view);
    void forgetTexture(TextureRef texture);
    const SceneTargets &sceneTargets() const
    {
        return targets_;
    }

  private:
    struct Family
    {
        PipelineSpec spec;
        MeshState blend;   // spec.blend points here
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputs;   // the Vertex attributes its vertex shader reads
        std::array<ComPtr<ID3D12PipelineState>, DrawState::kKeyCount> states;   // by DrawState::key
    };
    struct Buffer
    {
        ComPtr<ID3D12Resource> resource;
        void *mapped = nullptr;
        uint64_t bytes = 0;
        uint64_t offset = 0;     // where it starts in resource (a shared block's range; 0 in its own resource)
        int block = -1;          // its BufferBlock (bufferBlocks_), -1: its own resource
        uint64_t reserved = 0;   // the bytes its range in the block takes
    };
    // a GPU-only buffer small uploadBuffers share (each committed resource takes at least 64 KB)
    struct BufferBlock
    {
        ComPtr<ID3D12Resource> resource;
        RangeAllocator ranges;
    };
    struct Pool
    {
        uint32_t first = 0, count = 0, used = 0;
    };
    struct Queries
    {
        ComPtr<ID3D12QueryHeap> heap;
        ComPtr<ID3D12Resource> results;   // readback, 8 bytes a query
        std::vector<uint64_t> resolvedAt;  // the fence value the frame that resolved each signals
        uint32_t endedFirst = 0, endedEnd = 0;   // the queries ended and not resolved yet (queryEnded)
        D3D12_QUERY_TYPE endedType = D3D12_QUERY_TYPE_OCCLUSION;
    };

    void createRootSignature(uint32_t pushConstantBytes);
    void createSamplerTables();
    ComPtr<ID3D12PipelineState> buildState(const Family &family, const DrawState &state) const;
    BufferRef keep(Buffer buffer);
    Buffer placeInBlock(uint64_t bytes);
    static D3D12_GPU_VIRTUAL_ADDRESS gpuAddress(const Buffer &buffer);
    TextureRef keepTexture(D3D12Texture texture);
    void uploadTexture(const argon::TextureData &data, ID3D12Resource *resource);
    uint32_t takeFromPool(SetPool pool, uint32_t descriptors);
    void useSceneTexture(SceneSet set, uint32_t slot, TextureRef view);
    void createStorageView(BufferRef storage, uint32_t index);

    D3D12Context *context_ = nullptr;
    SceneTargets targets_;
    ComPtr<ID3D12RootSignature> rootSignature_;
    std::vector<std::unique_ptr<Family>> families_;          // Pipeline value - 1
    std::map<uint32_t, Pipeline> meshPipelines_;             // MeshState::pipelineKey() | wireframe << 20
    std::unordered_map<uint64_t, Buffer> buffers_;           // by BufferRef
    std::vector<BufferBlock> bufferBlocks_;                  // Buffer::block
    uint64_t nextBuffer_ = 1;
    std::unordered_map<uint64_t, D3D12Texture> textures_;    // by TextureRef
    uint64_t nextTexture_ = 1;
    std::vector<D3D12LayerTarget> layerTargets_;             // LayerTarget value - 1
    std::vector<Pool> pools_;                                // SetPool value - 1
    std::vector<Queries> queries_;                           // QueryPool value - 1
    uint32_t samplerTables_ = 0;                             // 256 tables of four (every SamplerMode combination)
    uint32_t singleSamplers_ = 0;                            // one per SamplerMode
};

}  // namespace argon::gfx::d3d12
