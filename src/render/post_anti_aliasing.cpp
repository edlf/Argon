// Argon: the post-process anti-aliasing over the 3D view (render\post_anti_aliasing.hpp).
#include "render/post_anti_aliasing.hpp"

#include <AreaTex.h>
#include <SearchTex.h>

#include <array>
#include <cstddef>
#include <span>

namespace
{
using argon::gfx::DepthCompare;
using argon::gfx::RenderTarget;
using argon::gfx::ShaderProgram;

// texels of `channels` bytes each widened to RGBA8 (the missing channels 0), one level
argon::TextureData rgbaTable(std::span<const unsigned char> texels, uint32_t width, uint32_t height, uint32_t channels)
{
    argon::TextureData table;
    table.width = width;
    table.height = height;
    table.format = argon::gfx::TextureFormat::Rgba8;
    table.bytes.assign(size_t(width) * height * 4, 0);
    for (size_t texel = 0; texel < size_t(width) * height; ++texel)
    {
        for (size_t channel = 0; channel < channels; ++channel)
        {
            table.bytes[texel * 4 + channel] = texels[texel * channels + channel];
        }
    }
    table.levels.push_back({width, height, 0, table.bytes.size()});
    return table;
}

// 1 / width, 1 / height, width, height: SMAA_RT_METRICS, and FXAA's texel size
struct TargetMetrics
{
    float values[4];

    explicit TargetMetrics(argon::gfx::WindowSize size)
        : values{1.f / float(size.width), 1.f / float(size.height), float(size.width), float(size.height)}
    {
    }
};
}  // namespace

PostAntiAliasingPass::PostAntiAliasingPass(argon::gfx::Device &device, argon::gfx::Presenter &presenter) : device_(device), presenter_(presenter)
{
    createPipelines();
    createLookupTables();
    frame_ = presenter_.createFrameCopy();
}

void PostAntiAliasingPass::destroy()
{
    device_.waitForIdle();
    destroySmaaTargets();
    device_.destroyTexture(areaTable_);
    device_.destroyTexture(searchTable_);
    presenter_.destroyFrameCopy(frame_);
    mode_ = PostAntiAliasing::Off;
}

// opaque, over every pixel: the depth test is off for them (record)
void PostAntiAliasingPass::createPipelines()
{
    fxaa_ = device_.createPipeline({.program = ShaderProgram::Fxaa, .depth = DepthCompare::Always});
    smaaEdges_ = device_.createPipeline({.program = ShaderProgram::SmaaEdges, .depth = DepthCompare::Always, .target = RenderTarget::Layer});
    smaaWeights_ = device_.createPipeline({.program = ShaderProgram::SmaaWeights, .depth = DepthCompare::Always, .target = RenderTarget::Layer});
    smaaBlend_ = device_.createPipeline({.program = ShaderProgram::SmaaBlend, .depth = DepthCompare::Always});
}

// SMAA's precomputed tables: the area each edge shape covers (RG) and the search's end lengths (R)
void PostAntiAliasingPass::createLookupTables()
{
    areaTable_ = device_.createTexture(rgbaTable(areaTexBytes, AREATEX_WIDTH, AREATEX_HEIGHT, 2));
    searchTable_ = device_.createTexture(rgbaTable(searchTexBytes, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1));
}

void PostAntiAliasingPass::prepare(PostAntiAliasing mode)
{
    mode_ = PostAntiAliasing::Off;
    bool remade = false;
    if (mode != PostAntiAliasing::Smaa && smaa_.edges)
    {
        device_.waitForIdle();
        destroySmaaTargets();
        remade = true;
    }
    if (mode == PostAntiAliasing::Off)
    {
        return;
    }
    remade = remade || !presenter_.frameCopyFits(frame_);
    if (!presenter_.fitFrameCopy(frame_))
    {
        return;
    }
    if (mode == PostAntiAliasing::Smaa)
    {
        remade = fitSmaaTargets() || remade;
    }
    if (remade || !frameSet_)
    {
        makeTextureSets();
    }
    mode_ = mode;
}

// true when they were made (again): the window's size, one layer each
bool PostAntiAliasingPass::fitSmaaTargets()
{
    const argon::gfx::WindowSize size = presenter_.windowSize();
    if (smaa_.edges && smaa_.size.width == size.width && smaa_.size.height == size.height)
    {
        return false;
    }
    device_.waitForIdle();
    destroySmaaTargets();
    smaa_.edges = device_.createLayerTarget({size.width, size.height, 1});
    smaa_.weights = device_.createLayerTarget({size.width, size.height, 1});
    smaa_.size = size;
    return true;
}

void PostAntiAliasingPass::destroySmaaTargets()
{
    device_.destroyLayerTarget(smaa_.edges);
    device_.destroyLayerTarget(smaa_.weights);
    smaa_.size = {};
}

// every set's four textures: the frame copy, the SMAA targets and tables (the GPU done with the old sets first)
void PostAntiAliasingPass::makeTextureSets()
{
    device_.waitForIdle();
    if (sets_)
    {
        device_.resetSetPool(sets_);
    }
    else
    {
        sets_ = device_.createSetPool({.sets = 4, .materialSets = 1});
    }
    const argon::gfx::TextureRef frame = presenter_.frameCopyTexture(frame_);
    frameSet_ = textureSet(frame, frame, frame);
    weightsSet_ = blendSet_ = {};
    if (smaa_.edges)
    {
        const argon::gfx::TextureRef edges = device_.layerTexture(smaa_.edges).handle;
        const argon::gfx::TextureRef weights = device_.layerTexture(smaa_.weights).handle;
        weightsSet_ = textureSet(edges, areaTable_.handle, searchTable_.handle);
        blendSet_ = textureSet(frame, weights, frame);
    }
}

// tex[0..2] of the passes' shaders, clamped (tex[3] is unread: the first again)
argon::gfx::TextureSet PostAntiAliasingPass::textureSet(argon::gfx::TextureRef first, argon::gfx::TextureRef second, argon::gfx::TextureRef third)
{
    argon::gfx::TextureSetDesc desc;
    desc.textures = {first, second, third, first};
    desc.samplers.fill(argon::gfx::SamplerMode::Clamp);
    return device_.createTextureSet(sets_, desc);
}

void PostAntiAliasingPass::record(argon::gfx::CommandList &commands)
{
    if (mode_ == PostAntiAliasing::Off)
    {
        return;
    }
    commands.setDepthCull({.test = false, .write = false, .cullBack = false});
    presenter_.suspendScenePass(frame_);
    if (mode_ == PostAntiAliasing::Smaa)
    {
        recordSmaa(commands);
        return;
    }
    presenter_.resumeScenePass();
    drawOverTarget(commands, fxaa_, frameSet_);
}

// the edges and the weights in their layer passes (cleared to 0: no edge, no weight), then the blend over the scene
void PostAntiAliasingPass::recordSmaa(argon::gfx::CommandList &commands)
{
    commands.beginLayerPass(smaa_.edges, 0);
    drawOverTarget(commands, smaaEdges_, frameSet_);
    commands.endLayerPass();
    commands.beginLayerPass(smaa_.weights, 0);
    drawOverTarget(commands, smaaWeights_, weightsSet_);
    commands.endLayerPass();
    presenter_.resumeScenePass();
    drawOverTarget(commands, smaaBlend_, blendSet_);
}

// one triangle over the whole target (the window's size, as the SMAA targets are)
void PostAntiAliasingPass::drawOverTarget(argon::gfx::CommandList &commands, argon::gfx::Pipeline pipeline, argon::gfx::TextureSet textures) const
{
    const argon::gfx::WindowSize size = presenter_.windowSize();
    const TargetMetrics metrics(size);
    commands.setViewport({0, 0, size.width, size.height});
    commands.bindPipeline(pipeline);
    commands.bindTextures(textures);
    commands.pushConstants(std::as_bytes(std::span(metrics.values)));
    commands.draw({.count = 3, .first = 0});
}
