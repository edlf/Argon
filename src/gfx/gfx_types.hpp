// Argon: what the renderer's drawing code holds of the GPU, the same for every backend (Vulkan, Direct3D 12): handles
// to pipelines, texture sets, buffers and query pools, and the small values a command takes. A backend keeps its own
// objects behind the handles (gfx\vulkan\vulkan_handles.hpp for Vulkan).
#pragma once
#include <cstdint>

namespace argon::gfx
{

template <class Tag> struct Handle
{
    uint64_t value = 0;   // 0: none

    explicit operator bool() const
    {
        return value != 0;
    }
    bool operator==(const Handle &) const = default;
};

struct PipelineTag;
struct TextureSetTag;
struct MaterialSetTag;
struct SceneSetTag;
struct BufferTag;
struct QueryPoolTag;
struct TextureTag;
struct SetPoolTag;
struct LayerTargetTag;
struct FrameCopyTag;

using Pipeline = Handle<PipelineTag>;          // shaders, vertex layout, blending (gfx::PipelineSpec)
using TextureSet = Handle<TextureSetTag>;      // the four stage textures and their samplers (set 0)
using MaterialSet = Handle<MaterialSetTag>;    // a model's per-draw materials (set 2)
using SceneSet = Handle<SceneSetTag>;          // a view's scene constants, the frame copy and the OIT targets (set 1)
using BufferRef = Handle<BufferTag>;           // a vertex or index buffer
using QueryPool = Handle<QueryPoolTag>;        // occlusion queries
using TextureRef = Handle<TextureTag>;         // a texture (2D array, cube) the shaders sample
using SetPool = Handle<SetPoolTag>;            // texture and material sets that are freed together (a track, the props)
using LayerTarget = Handle<LayerTargetTag>;    // a 2D array drawn one layer at a time and sampled whole (the shadow maps)
using FrameCopy = Handle<FrameCopyTag>;        // a window-sized copy of the frame (gfx::Presenter)

// how a stage samples its texture: bit 0 clamps U, bit 1 clamps V (the stage flags 0x40 / 0x80); else it repeats
enum class SamplerMode
{
    Wrap = 0,
    ClampU = 1,
    ClampV = 2,
    Clamp = 3
};

// a part of the render target in pixels
struct Rect
{
    int32_t x = 0, y = 0;
    uint32_t width = 0, height = 0;
};

enum class Winding
{
    CounterClockwise,
    Clockwise
};

struct Colour
{
    float r = 0, g = 0, b = 0, a = 1;
};

// what a pass or an area is cleared to
struct SceneClear
{
    Colour colour;
    float depth = 0;
};

// the depth test and write and the back-face culling of the draws that follow
// a whole one-level texture written from an upload buffer (BufferUse::Upload) of width x height 4-byte texels, its rows
// rowPitch bytes apart (Device::uploadRowPitch)
struct TextureUpload
{
    BufferRef source;
    TextureRef texture;
    uint32_t width = 0, height = 0, rowPitch = 0;
};

// the window's drawable size in pixels
struct WindowSize
{
    uint32_t width = 0, height = 0;
};

// the order-independent translucency's targets as the scene sets read them (gfx::Presenter::oitTargets): the weighted
// colour sum and the revealage, at the scene's size and samples
struct OitTargets
{
    TextureRef accumulated, revealage;
};

// a layer target's layers: their width and height, and how many
struct LayerTargetSize
{
    uint32_t width = 0, height = 0, layers = 1;
};

struct DepthCull
{
    bool test = true, write = true, cullBack = false;

    bool operator==(const DepthCull &) const = default;
};

// `count` vertices from `first`
struct DrawRange
{
    uint32_t count = 0, first = 0;
};

// `count` indices from `firstIndex`, each added to vertexOffset; firstInstance is the material index (gl_InstanceIndex)
struct IndexedDraw
{
    uint32_t count = 0, firstIndex = 0;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
};

// queries first .. first + count - 1 of a pool
struct QueryRange
{
    QueryPool pool;
    uint32_t first = 0, count = 0;
};

// one occlusion query: its pool and index; precise counts samples, else any sample passing reads 1
struct OcclusionQuery
{
    QueryPool pool;
    uint32_t index = 0;
    bool precise = false;
};

}  // namespace argon::gfx
