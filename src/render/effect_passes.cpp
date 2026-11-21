// Argon: the race's effect passes, see effect_passes.hpp.
#include "render/effect_passes.hpp"

#include <cmath>

#include "render/scene_recorder.hpp"

EffectPasses::EffectPasses(const EffectSources &sources, const SceneRecorder &scene)
    : sources_(sources), scene_(scene), commands_(scene.commands()), viewProj_(scene.viewProj())
{
}

// each effect's vertices for this view (render\effect_buffers.hpp), then its draw
void EffectPasses::drawEffects(const glm::mat4 &view, float fovDeg, double seconds)
{
    EffectBuffers &buffers = sources_.buffers;
    const int slot = sources_.viewSlot;
    if (sources_.toggles.spray)
    {
        buffers.writeSpray(slot, sources_.spray, view, sources_.clock);
        if (sources_.toggles.drops)
        {
            buffers.addSprayDrops(slot, sources_.spray, sources_.clock);
        }
    }
    else
    {
        buffers.clearSpray();
    }
    drawSpray();
    buffers.writeTrailSprites(slot, sources_.weaponSprites, view, sources_.clock);
    drawTrailSprites();
    if (sources_.toggles.trackSprites)
    {
        buffers.writeTrackSprites(slot, sources_.track.data.sprites, sources_.camera.eye, view);
    }
    else
    {
        buffers.clearTrackSprites();
    }
    drawTrackSprites();
    drawTrail(seconds);
    const argon::ParticleCamera camera{view, fovDeg, sources_.screenHeight};
    buffers.writeParticles(slot, sources_.particles, camera, sources_.clock);
    drawParticles();
}

// ship shadows: framebuffer - silhouette x height fade
void EffectPasses::drawShadowReceivers()
{
    const ShipShadows &shadows = sources_.shadows;
    if (!shadows.receiverVertices())
    {
        return;
    }
    commands_.bindPipeline(shadows.receiverPipeline());
    scene_.setDepthCull({.test = true, .write = false, .cullBack = false});
    commands_.bindTextures(shadows.receiverTextures());
    scene_.bindVertexBuffer(shadows.receiverBuffer(sources_.frame));
    scene_.pushMatrices(viewProj_, glm::mat4(1));
    commands_.draw({shadows.receiverVertices(), 0});
}

// water_draw 0xce1c0: f = game frames; bump frame (f/2)%60, wave frames (f/8)%30 and +1, w = (f&7)/8. The model
// columns carry the draw parameters: [0] = frame a, frame b, weight, bump frame; [1].x = cube map (box b);
// [3] = patch offset c5 = (x - 4, y, z - 4)
void EffectPasses::drawWater(double seconds)
{
    const Track &track = sources_.track;
    if (!sources_.toggles.water || !track.waterIndexCount)
    {
        return;
    }
    commands_.bindPipeline(sources_.waterPipe);
    scene_.setDepthCull({.test = true, .write = true, .cullBack = false});   // inherited depth test / write (0xbae10), cull none (0xce29d)
    commands_.bindTextures(track.waterSet);
    commands_.bindMaterials(track.waterWaveSet);
    scene_.bindVertexBuffer(track.waterVb);
    commands_.bindIndexBuffer(track.waterIb);
    const long long frame = (long long)(seconds * 60.0);
    const int fa = int((frame >> 3) % 30), fb = (fa + 1) % 30;
    for (const auto &patch : track.waterPatches)
    {
        glm::mat4 params(0.f);
        params[0] = glm::vec4(float(fa), float(fb), float(frame & 7) * 0.125f, float((frame >> 1) % 60));
        params[1] = glm::vec4(float(patch.cube), 0, 0, 0);
        params[3] = glm::vec4(patch.pos.x - 4.f, patch.pos.y, patch.pos.z - 4.f, 1.f);
        scene_.pushMatrices(viewProj_, params);
        commands_.drawIndexed({uint32_t(track.waterIndexCount), uint32_t(0), int32_t(0), 0});
    }
}

// water spray puffs, alpha blended
void EffectPasses::drawSpray()
{
    const EffectBuffers::Spray &spray = sources_.buffers.spray();
    if (!spray.verts)
    {
        return;
    }
    commands_.bindPipeline(spray.pipeline);
    scene_.setDepthCull({.test = true, .write = false, .cullBack = false});
    commands_.bindTextures(spray.set);
    scene_.bindVertexBuffer(spray.stream.buffers[size_t(sources_.viewSlot)]);
    glm::mat4 params(0.f);
    params[0].z = sources_.toggles.sprayVertexAlpha ? 1.f : 0.f;
    scene_.pushMatrices(viewProj_, params);
    commands_.draw({uint32_t(spray.verts), 0});
}

// weapon trail sprites (0xbd8f0): list A alpha blended (the spray's pipeline), list B additive
void EffectPasses::drawTrailSprites()
{
    const EffectBuffers::TwoLists &sprites = sources_.buffers.trailSprites();
    if (!sprites.alphaVerts && !sprites.additiveVerts)
    {
        return;
    }
    scene_.setDepthCull({.test = true, .write = false, .cullBack = false});
    commands_.bindTextures(sources_.buffers.spray().set);
    scene_.bindVertexBuffer(sprites.stream.buffers[size_t(sources_.viewSlot)]);
    glm::mat4 params(0.f);
    params[0].z = sources_.toggles.sprayVertexAlpha ? 1.f : 0.f;
    if (sprites.alphaVerts)
    {
        commands_.bindPipeline(sprites.alphaPipeline);
        scene_.pushMatrices(viewProj_, params);
        commands_.draw({uint32_t(sprites.alphaVerts), 0});
    }
    if (sprites.additiveVerts)
    {
        commands_.bindPipeline(sprites.additivePipeline);
        scene_.pushMatrices(viewProj_, params);
        commands_.draw({uint32_t(sprites.additiveVerts), uint32_t(sprites.alphaVerts)});
    }
}

// sprite.dat's quads (0xbd8f0): depth test without writes, no culling; list A alpha blended, list B additive
void EffectPasses::drawTrackSprites()
{
    const EffectBuffers::TwoLists &sprites = sources_.buffers.trackSprites();
    if (!sprites.alphaVerts && !sprites.additiveVerts)
    {
        return;
    }
    scene_.setDepthCull({.test = true, .write = false, .cullBack = false});
    commands_.bindTextures(sources_.buffers.spray().set);
    scene_.bindVertexBuffer(sprites.stream.buffers[size_t(sources_.viewSlot)]);
    drawTrackSpriteList(sprites.alphaPipeline, {sprites.alphaVerts, 0});
    drawTrackSpriteList(sprites.additivePipeline, {sprites.additiveVerts, sprites.alphaVerts});
}

void EffectPasses::drawTrackSpriteList(argon::gfx::Pipeline pipe, argon::gfx::DrawRange range)
{
    if (!range.count)
    {
        return;
    }
    glm::mat4 params(0.f);
    params[0].z = sources_.toggles.sprayVertexAlpha ? 1.f : 0.f;
    params[0].w = pipe == sources_.buffers.trackSprites().additivePipeline ? 1.f : 0.f;
    commands_.bindPipeline(pipe);
    scene_.pushMatrices(viewProj_, params);
    commands_.draw(range);
}

// ship trail ribbons, additive
void EffectPasses::drawTrail(double seconds)
{
    const EffectBuffers::Trails &trails = sources_.buffers.trails();
    if (!trails.verts)
    {
        return;
    }
    commands_.bindPipeline(trails.pipeline);
    scene_.setDepthCull({.test = true, .write = false, .cullBack = false});
    commands_.bindTextures(trails.set);
    scene_.bindVertexBuffer(trails.stream.buffers[size_t(sources_.frame)]);
    const double gameFrames = seconds * 60.0;
    glm::mat4 params(0.f);
    params[0] = glm::vec4(float(std::fmod(-0.5 * gameFrames, 1024.0)),
                          float(std::fmod(-2.0 * gameFrames, 1024.0)),
                          float(sources_.toggles.trailMode),
                          sources_.toggles.trailGain);
    scene_.pushMatrices(viewProj_, params);
    commands_.draw({uint32_t(trails.verts), 0});
}

// pickup starbursts: additive, texture x vertex colour (trail pipeline, mode 0)
void EffectPasses::drawParticles()
{
    const EffectBuffers::ParticleSystems &systems = sources_.buffers.particles();
    if (!systems.verts)
    {
        return;
    }
    commands_.bindPipeline(sources_.buffers.trails().pipeline);
    scene_.setDepthCull({.test = true, .write = false, .cullBack = false});
    scene_.bindVertexBuffer(systems.stream.buffers[size_t(sources_.viewSlot)]);
    glm::mat4 params(0.f);
    params[0] = glm::vec4(0.f, 0.f, 0.f, 1.f);
    scene_.pushMatrices(viewProj_, params);
    const argon::gfx::TextureSet star = systems.sets[0];
    const argon::gfx::TextureSet sets[3] = {star, systems.sets[1] ? systems.sets[1] : star, systems.sets[2] ? systems.sets[2] : star};
    for (int sys = 0; sys < 3; ++sys)
    {
        if (!systems.ranges[sys].count)
        {
            continue;
        }
        commands_.bindTextures(sets[sys]);
        commands_.draw({uint32_t(systems.ranges[sys].count), uint32_t(systems.ranges[sys].first)});
    }
}
