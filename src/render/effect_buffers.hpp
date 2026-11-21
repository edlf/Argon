// Argon: the GPU side of the race's effects (docs\effects\README.md): for the engine trails, the water spray, the
// weapons' trail sprites, sprite.dat's track sprites and the pickup particles, the pipeline, the textures, a vertex
// buffer per frame in flight (and view, for the camera-facing ones) and what this frame wrote into it. The effects
// themselves (src\effects, logic\particles.hpp) write the vertices; render\effect_passes.cpp draws them.
#pragma once
#include "effects/engine_trails.hpp"
#include "effects/wake_spray.hpp"
#include "effects/weapon_sprites.hpp"
#include "files/dds.hpp"
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "gfx/texture.hpp"
#include "logic/particles.hpp"
#include "track/track_sprites.hpp"

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

class EffectBuffers
{
  public:
    static constexpr int kFrames = argon::gfx::kFramesInFlight;
    static constexpr int kViewSlots = kFrames * 4;   // per frame in flight and split-screen view
    static constexpr int kMaxTrackSprites = 4048;    // the pool: 2000 blended + 2048 additive

    // a stream buffer per slot and what this slot's frame wrote
    template <int Slots> struct Stream
    {
        std::array<argon::gfx::BufferRef, Slots> buffers{};
        std::array<void *, Slots> mapped{};
    };
    struct Trails
    {
        argon::gfx::Pipeline pipeline;   // additive, data\newtrail.dds (the particles draw with it too)
        argon::gfx::Texture texture;
        argon::gfx::TextureSet set;
        Stream<kFrames> stream;
        uint32_t verts = 0;
    };
    struct Spray
    {
        argon::gfx::Pipeline pipeline;   // alpha blended, the track's envdata\quadsprite (the sprites' atlas too)
        argon::gfx::Texture texture;
        argon::gfx::TextureSet set;
        std::string track;   // the track the texture is from
        Stream<kViewSlots> stream;
        uint32_t verts = 0;
    };
    struct TwoLists   // alpha quads first, then the additive ones
    {
        argon::gfx::Pipeline alphaPipeline, additivePipeline;
        Stream<kViewSlots> stream;
        uint32_t alphaVerts = 0, additiveVerts = 0;
    };
    struct ParticleSystems   // starburst, starburstred, bubble (0x88660)
    {
        std::array<argon::gfx::Texture, argon::Particles::kSystems> textures{};
        std::array<argon::gfx::TextureSet, argon::Particles::kSystems> sets{};
        argon::Particles::Ranges ranges{};   // per system, in the vertex buffer
        Stream<kViewSlots> stream;
        uint32_t verts = 0;
    };

    // white: the texture of the unused stages; pool: where the texture sets live
    void create(argon::gfx::Device &device, const argon::gfx::Texture &white, argon::gfx::SetPool pool, const std::string &gameFolder);
    void destroy();
    void loadParticleTextures();   // data\hud's atlases, once
    void useSprayTexture(const argon::TextureData &picture, const std::string &track);   // no levels: none
    bool sprayTextureOf(const std::string &track) const;

    // this frame's vertices
    void writeTrails(int frame, const argon::EngineTrails &trails);
    void writeSpray(int viewSlot, const argon::WakeSpray &spray, const glm::mat4 &view, float now);
    void addSprayDrops(int viewSlot, const argon::WakeSpray &spray, float now);   // after writeSpray: the droplet sheets
    void writeTrailSprites(int viewSlot, const argon::WeaponSprites &sprites, const glm::mat4 &view, float now);
    void writeTrackSprites(int viewSlot, const std::vector<argon::TrackSprite> &sprites, const glm::vec3 &eye, const glm::mat4 &view);
    void writeParticles(int viewSlot, const argon::Particles &particles, const argon::ParticleCamera &camera, float now);
    void clearTrails();
    void clearSpray();
    void clearTrackSprites();

    const Trails &trails() const;
    const Spray &spray() const;
    const TwoLists &trailSprites() const;
    const TwoLists &trackSprites() const;
    const ParticleSystems &particles() const;

  private:
    argon::gfx::TextureSet setOf(const argon::gfx::Texture &texture, argon::gfx::SamplerMode sampling) const;
    template <int Slots> void createStream(Stream<Slots> &stream, size_t bytes);
    template <int Slots> void destroyStream(Stream<Slots> &stream);
    uint32_t writeTrackSpriteList(argon::Vertex *out, const std::vector<argon::TrackSprite> &sprites, const std::vector<int> &order,
                                  const glm::mat4 &view) const;

    argon::gfx::Device *device_ = nullptr;
    argon::gfx::Texture white_;
    argon::gfx::SetPool pool_;
    std::string game_;
    Trails trails_;
    Spray spray_;
    TwoLists trailSprites_, trackSprites_;
    ParticleSystems particles_;
};
