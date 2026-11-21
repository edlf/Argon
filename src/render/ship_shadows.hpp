// Argon: the ship shadows (0x9b820 / 0xbe230 / 0x9c3a0): per ship an orthographic 13.5 x 13.5 silhouette seen from
// 100 units up the light direction, projected onto up to 64 nearby light-facing collision triangles whose vertex grey
// fades with the height difference (clamp(35 - |dy|, 0, 30) / 255), then subtracted from the framebuffer (blend ONE /
// ONE, REVERSE_SUBTRACT; render\effect_passes.cpp draws the receivers).
#pragma once
#include "gfx/command_list.hpp"
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"
#include "render/gpu_model.hpp"
#include "render/mesh_draw_records.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace argon
{
class CollisionGrid;
}

// a ship as drawn this frame: its model and placement
struct ShadowedShip
{
    Track *ship;
    glm::mat4 placement;
};

class ShipShadows
{
  public:
    static const int kMaxShadows = 16;

    // what a frame's shadows are cast from and onto; solid: the editor's solid silhouettes
    struct Frame
    {
        int frame;
        glm::vec3 sun;
        const argon::CollisionGrid &receivers;
        argon::gfx::SceneSet scene;
        MeshDrawRecords &meshDraws;   // the frame's (the scene set reads them)
        bool solid;
    };

    void create(argon::gfx::Device &device, const Texture &white, argon::gfx::SetPool pool);
    void destroy();
    // the shadow-map passes (outside the main render pass) for the first kMaxShadows ships, and their receivers
    void record(argon::gfx::CommandList &commands, const std::vector<ShadowedShip> &ships, const Frame &frame);
    void clear();

    // the receivers' draw (this frame's vertices)
    uint32_t receiverVertices() const;
    argon::gfx::Pipeline receiverPipeline() const;
    argon::gfx::TextureSet receiverTextures() const;
    argon::gfx::BufferRef receiverBuffer(int frame) const;

  private:
    static const int kShadowSize = 256, kShadowTris = 64;
    // a shadowed ship: where it is, the light's view of it and its layer of the shadow maps
    struct Caster
    {
        const Track *ship;
        glm::mat4 model, lightViewProjection;
        glm::vec3 light;   // the sun's direction
        uint32_t layer;
    };
    void drawCaster(argon::gfx::CommandList &commands, const Caster &caster, const Frame &frame) const;
    void collectReceivers(const Caster &caster, const argon::CollisionGrid &grid, argon::Vertex *out);

    argon::gfx::Device *device_ = nullptr;
    argon::gfx::LayerTarget maps_;   // a layer per shadowed ship
    argon::gfx::Pipeline castPipeline_, solidPipeline_, receivePipeline_;
    argon::gfx::TextureSet mapSet_;
    std::array<argon::gfx::BufferRef, argon::gfx::kFramesInFlight> buffers_;
    std::array<void *, argon::gfx::kFramesInFlight> mapped_{};
    uint32_t vertices_ = 0;
    std::vector<uint32_t> triangles_;   // collectReceivers' grid query, its storage kept between frames
};
