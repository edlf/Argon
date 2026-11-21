// Argon: the GPU side of the race's effects (effect_buffers.hpp).
#include "render/effect_buffers.hpp"

#include "model/cmm.hpp"
#include "model/material.hpp"
#include "render/sprite_pool.hpp"

namespace
{
using argon::gfx::SamplerMode;

argon::Vertex spriteVertex(const glm::vec3 &at, const argon::TrackSpriteCorner &corner)
{
    argon::Vertex vertex{};
    vertex.pos[0] = at.x;
    vertex.pos[1] = at.y;
    vertex.pos[2] = at.z;
    vertex.uv[0][0] = corner.uv.x;
    vertex.uv[0][1] = corner.uv.y;
    vertex.rgba[0] = uint8_t(corner.argb >> 16);
    vertex.rgba[1] = uint8_t(corner.argb >> 8);
    vertex.rgba[2] = uint8_t(corner.argb);
    vertex.rgba[3] = uint8_t(corner.argb >> 24);
    return vertex;
}

void writeSpriteQuad(argon::Vertex *out, const argon::TrackSprite &sprite, const argon::SpriteBasis &basis)
{
    for (int k = 0; k < argon::kSpriteTriangleVertices; ++k)
    {
        const int corner = argon::kSpriteTriangleCorners[k];
        out[k] = spriteVertex(argon::spriteCornerPosition(sprite, corner, basis), sprite.corners[size_t(corner)]);
    }
}

argon::Vertex *vertices(void *mapped)
{
    return static_cast<argon::Vertex *>(mapped);
}
}  // namespace

void EffectBuffers::create(argon::gfx::Device &device, const argon::gfx::Texture &white, argon::gfx::SetPool pool, const std::string &gameFolder)
{
    device_ = &device;
    white_ = white;
    pool_ = pool;
    game_ = gameFolder;
    const argon::MeshState additive = argon::MeshState::additive(), alpha = argon::MeshState::alphaBlended();

    const argon::TextureData trail = argon::loadOptionalDds(game_ + "/data/newtrail.dds");
    if (!trail.levels.empty())
    {
        trails_.texture = device.createTexture(trail);
    }
    trails_.pipeline = device.createPipeline({.program = argon::gfx::ShaderProgram::Trail, .blend = &additive});
    trails_.set = setOf(trails_.texture.valid ? trails_.texture : white_, SamplerMode::Wrap);
    createStream(trails_.stream, size_t(argon::EngineTrails::kMaxVertices) * sizeof(argon::Vertex));

    spray_.pipeline = device.createPipeline({.program = argon::gfx::ShaderProgram::Spray, .blend = &alpha});
    createStream(spray_.stream, size_t(argon::WakeSpray::kMaxVertices) * sizeof(argon::Vertex));   // puffs + droplet sheets

    // the trail sprites' alpha list uses the spray's pipeline; the additive list its own (blend one, one)
    trailSprites_.alphaPipeline = spray_.pipeline;
    trailSprites_.additivePipeline = device.createPipeline({.program = argon::gfx::ShaderProgram::Spray, .blend = &additive});
    createStream(trailSprites_.stream, size_t(argon::WeaponSprites::kMaxVertices) * sizeof(argon::Vertex));

    trackSprites_.alphaPipeline = device.createPipeline({.program = argon::gfx::ShaderProgram::QuadSprite, .blend = &alpha});
    trackSprites_.additivePipeline = device.createPipeline({.program = argon::gfx::ShaderProgram::QuadSprite, .blend = &additive});
    createStream(trackSprites_.stream, size_t(kMaxTrackSprites) * argon::kSpriteTriangleVertices * sizeof(argon::Vertex));

    createStream(particles_.stream, size_t(argon::Particles::kMax) * 6 * sizeof(argon::Vertex));
}

void EffectBuffers::destroy()
{
    destroyStream(trails_.stream);
    device_->destroyTexture(trails_.texture);
    destroyStream(spray_.stream);
    device_->destroyTexture(spray_.texture);
    destroyStream(trailSprites_.stream);
    destroyStream(trackSprites_.stream);
    destroyStream(particles_.stream);
    for (argon::gfx::Texture &texture : particles_.textures)
    {
        device_->destroyTexture(texture);
    }
}

// data\hud\starburst.dds, starburstred.dds, bubble.dds: the three particle systems of 0x88660
void EffectBuffers::loadParticleTextures()
{
    static const char *const kFiles[argon::Particles::kSystems] = {"starburst.dds", "starburstred.dds", "bubble.dds"};
    for (int system = 0; system < argon::Particles::kSystems; ++system)
    {
        argon::gfx::Texture &texture = particles_.textures[size_t(system)];
        if (texture.valid)
        {
            continue;
        }
        const argon::TextureData data = argon::loadOptionalDds(game_ + "/data/hud/" + kFiles[system]);
        if (!data.levels.empty())
        {
            texture = device_->createTexture(data);
        }
        if (texture.valid)
        {
            particles_.sets[size_t(system)] = setOf(texture, SamplerMode::Clamp);
        }
    }
}

void EffectBuffers::useSprayTexture(const argon::TextureData &picture, const std::string &track)
{
    spray_.track = track;
    if (spray_.texture.valid)
    {
        device_->waitForIdle();
        device_->destroyTexture(spray_.texture);
    }
    if (!picture.levels.empty())
    {
        spray_.texture = device_->createTexture(picture);
    }
    if (spray_.texture.valid)
    {
        spray_.set = setOf(spray_.texture, SamplerMode::Clamp);
    }
}

bool EffectBuffers::sprayTextureOf(const std::string &track) const
{
    return spray_.track == track;
}

void EffectBuffers::writeTrails(int frame, const argon::EngineTrails &trails)
{
    trails_.verts = trails.writeRibbons(vertices(trails_.stream.mapped[size_t(frame)]));
}

// the puffs, then the droplet sheets
void EffectBuffers::writeSpray(int viewSlot, const argon::WakeSpray &spray, const glm::mat4 &view, float now)
{
    spray_.verts = 0;
    if (spray.empty() || !spray_.texture.valid)
    {
        return;
    }
    spray_.verts = spray.writePuffs(view, now, vertices(spray_.stream.mapped[size_t(viewSlot)]));
}

void EffectBuffers::addSprayDrops(int viewSlot, const argon::WakeSpray &spray, float now)
{
    if (spray.empty() || !spray_.texture.valid)
    {
        return;
    }
    spray_.verts = spray.writeDrops(now, vertices(spray_.stream.mapped[size_t(viewSlot)]), spray_.verts);
}

void EffectBuffers::writeTrailSprites(int viewSlot, const argon::WeaponSprites &sprites, const glm::mat4 &view, float now)
{
    trailSprites_.alphaVerts = trailSprites_.additiveVerts = 0;
    if (sprites.empty() || !spray_.texture.valid || !trailSprites_.stream.mapped[size_t(viewSlot)])
    {
        return;
    }
    const argon::WeaponSprites::Written written = sprites.writeQuads(view, now, vertices(trailSprites_.stream.mapped[size_t(viewSlot)]));
    trailSprites_.alphaVerts = written.alpha;
    trailSprites_.additiveVerts = written.additive;
}

// list A alpha blended far to near, then list B additive (track_sprites.hpp)
void EffectBuffers::writeTrackSprites(int viewSlot, const std::vector<argon::TrackSprite> &sprites, const glm::vec3 &eye, const glm::mat4 &view)
{
    trackSprites_.alphaVerts = trackSprites_.additiveVerts = 0;
    if (sprites.empty() || !spray_.texture.valid || !trackSprites_.stream.mapped[size_t(viewSlot)])
    {
        return;
    }
    argon::Vertex *out = vertices(trackSprites_.stream.mapped[size_t(viewSlot)]);
    trackSprites_.alphaVerts = writeTrackSpriteList(out, sprites, argon::blendedSpriteOrder(sprites, eye), view);
    trackSprites_.additiveVerts = writeTrackSpriteList(out + trackSprites_.alphaVerts, sprites, argon::additiveSpriteOrder(sprites, eye), view);
}

uint32_t EffectBuffers::writeTrackSpriteList(argon::Vertex *out, const std::vector<argon::TrackSprite> &sprites, const std::vector<int> &order,
                                             const glm::mat4 &view) const
{
    const argon::SpriteBasis bases[3] = {argon::spriteBasis(argon::SpriteFacing::World, view),
                                         argon::spriteBasis(argon::SpriteFacing::Upright, view),
                                         argon::spriteBasis(argon::SpriteFacing::Camera, view)};
    uint32_t verts = 0;
    for (int index : order)
    {
        const argon::TrackSprite &sprite = sprites[size_t(index)];
        writeSpriteQuad(out + verts, sprite, bases[int(sprite.facing)]);
        verts += argon::kSpriteTriangleVertices;
    }
    return verts;
}

void EffectBuffers::writeParticles(int viewSlot, const argon::Particles &particles, const argon::ParticleCamera &camera, float now)
{
    particles_.verts = 0;
    particles_.ranges = {};
    if (particles.empty() || !particles_.textures[0].valid)
    {
        return;
    }
    particles_.ranges = particles.writeQuads(camera, now, vertices(particles_.stream.mapped[size_t(viewSlot)]));
    particles_.verts = particles_.ranges.back().first + particles_.ranges.back().count;
}

void EffectBuffers::clearTrails()
{
    trails_.verts = 0;
}

void EffectBuffers::clearSpray()
{
    spray_.verts = 0;
}

void EffectBuffers::clearTrackSprites()
{
    trackSprites_.alphaVerts = trackSprites_.additiveVerts = 0;
}

const EffectBuffers::Trails &EffectBuffers::trails() const
{
    return trails_;
}

const EffectBuffers::Spray &EffectBuffers::spray() const
{
    return spray_;
}

const EffectBuffers::TwoLists &EffectBuffers::trailSprites() const
{
    return trailSprites_;
}

const EffectBuffers::TwoLists &EffectBuffers::trackSprites() const
{
    return trackSprites_;
}

const EffectBuffers::ParticleSystems &EffectBuffers::particles() const
{
    return particles_;
}

// stage 0 the texture, the other three white
argon::gfx::TextureSet EffectBuffers::setOf(const argon::gfx::Texture &texture, argon::gfx::SamplerMode sampling) const
{
    argon::gfx::TextureSetDesc desc;
    desc.textures = {texture.handle, white_.handle, white_.handle, white_.handle};
    desc.samplers = {sampling, SamplerMode::Wrap, SamplerMode::Wrap, SamplerMode::Wrap};
    return device_->createTextureSet(pool_, desc);
}

template <int Slots> void EffectBuffers::createStream(Stream<Slots> &stream, size_t bytes)
{
    for (int slot = 0; slot < Slots; ++slot)
    {
        stream.buffers[size_t(slot)] = device_->createStreamBuffer(argon::gfx::BufferUse::Vertex, bytes);
        stream.mapped[size_t(slot)] = device_->mapped(stream.buffers[size_t(slot)]);
    }
}

template <int Slots> void EffectBuffers::destroyStream(Stream<Slots> &stream)
{
    for (argon::gfx::BufferRef &buffer : stream.buffers)
    {
        device_->destroyBuffer(buffer);
    }
}
