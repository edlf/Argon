// Argon: the GPU objects the renderer makes, the same for every backend: pipelines, buffers, textures, the texture /
// material sets in their pools, the scene sets and the occlusion queries.
#pragma once
#include "gfx/gfx_types.hpp"
#include "gfx/pipeline_spec.hpp"
#include "gfx/texture.hpp"
#include "files/dds.hpp"

#include <array>
#include <optional>

#include <cstddef>
#include <span>

namespace argon::gfx
{

enum class BufferUse
{
    Vertex,
    Index,     // 32-bit indices
    Storage,   // read by the shaders (the materials, the water's wave frames)
    Uniform,   // the scene constants
    Upload     // copied into a texture (CommandList::copyToTexture: a video's frames)
};

// the four stage textures of a draw and how each is sampled
struct TextureSetDesc
{
    std::array<TextureRef, 4> textures;
    std::array<SamplerMode, 4> samplers{};
};

// how many sets a pool holds and how many of them are material sets
struct SetPoolSize
{
    uint32_t sets = 4096, materialSets = 8;
};

class Device
{
  public:
    virtual ~Device() = default;

    // kept until the device is destroyed
    virtual Pipeline createPipeline(const PipelineSpec &spec) = 0;
    // the mesh program's pipeline for a mesh state, made once per blend state and fill
    virtual Pipeline meshPipeline(const MeshState &state, Fill fill) = 0;

    // everything submitted done on the GPU (before freeing what it may still read)
    virtual void waitForIdle() = 0;
    // the uploads between them (a track load's frame slice) go to the GPU together, without waiting for them
    virtual void beginUploadBatch() = 0;
    virtual void submitUploadBatch() = 0;
    // Fill::Wireframe pipelines draw (else they draw solid)
    virtual bool wireframeSupported() const = 0;
    // what this process uses of the GPU's own memory; none when the driver does not say
    virtual std::optional<uint64_t> videoMemoryInUse() const = 0;
    // the bytes between the rows of a 4-byte-texel texture `width` wide in an upload buffer (TextureUpload)
    virtual uint32_t uploadRowPitch(uint32_t width) const = 0;

    // a buffer the CPU writes as it likes (each frame's vertices, the scene constants), mapped until destroyed
    virtual BufferRef createStreamBuffer(BufferUse use, size_t bytes) = 0;
    // a buffer written once with `data` (in the open upload batch, if any) and only read by the GPU after
    virtual BufferRef uploadBuffer(BufferUse use, std::span<const std::byte> data) = 0;
    // a stream buffer's memory
    virtual void *mapped(BufferRef buffer) const = 0;
    // frees it (none: nothing) and empties the handle; the GPU must be done with it
    virtual void destroyBuffer(BufferRef &buffer) = 0;

    // RGBA8 layers drawn one at a time (CommandList::beginLayerPass, the RenderTarget::Layer pipelines), every one
    // black until drawn; kept until destroyLayerTarget or the device goes
    virtual LayerTarget createLayerTarget(const LayerTargetSize &size) = 0;
    // frees it and empties the handle; the GPU must be done with it
    virtual void destroyLayerTarget(LayerTarget &target) = 0;
    // all its layers, sampled through texture sets
    virtual Texture layerTexture(LayerTarget target) const = 0;

    // a 2D array (one layer, or one per animation frame) or a cube; not valid when the data has no levels
    virtual Texture createTexture(const argon::TextureData &data) = 0;
    // frees it (not valid: nothing) and empties the record; the GPU must be done with it
    virtual void destroyTexture(Texture &texture) = 0;

    // freed with the device
    virtual SetPool createSetPool(const SetPoolSize &size) = 0;
    // every set made from it is gone (the GPU must be done with them)
    virtual void resetSetPool(SetPool pool) = 0;
    virtual TextureSet createTextureSet(SetPool pool, const TextureSetDesc &desc) = 0;
    // a model's materials (a storage buffer), read by the draws through their instance index
    virtual MaterialSet createMaterialSet(SetPool pool, BufferRef materials) = 0;
    // a view's scene constants (a uniform buffer), its frame's mesh draw records (a storage buffer,
    // render\mesh_draw_records.hpp) and the frame copy the Hud Lens samples; kept until the device goes
    virtual SceneSet createSceneSet(BufferRef constants, BufferRef meshDraws, TextureRef frameCopy) = 0;
    // a new frame copy for the set (the GPU must be done with the set)
    virtual void useFrameCopy(SceneSet set, TextureRef frameCopy) = 0;
    // the OIT targets the composite reads through the set (the GPU must be done with the set)
    virtual void useOitTargets(SceneSet set, const OitTargets &targets) = 0;

    // `count` occlusion queries, kept until the device goes
    virtual QueryPool createOcclusionQueries(uint32_t count) = 0;
    // the range's sample counts (or 0 / 1 when the GPU only answers "any"); false while any is not ready
    virtual bool readQueries(const QueryRange &range, std::span<uint64_t> results) const = 0;
    // the queries count samples (else any sample passing reads 1)
    virtual bool preciseOcclusion() const = 0;
};

}  // namespace argon::gfx
