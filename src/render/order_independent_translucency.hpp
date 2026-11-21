// Argon: Argon Options' order-independent translucency (docs\argon\differences.md "Translucency"). The game draws its
// translucent bucket far to near by each mesh's farthest box edge, and its alpha-blended meshes write depth, so a long
// mesh (Solar's glass fences) can be drawn before what lies behind it and hide it. With this on, the alpha-blended
// translucent meshes' solid pixels are drawn first as opaque (mesh_oit_opaque.frag), their other pixels go into
// weighted blended OIT targets (mesh_oit.frag: no depth write, no order), which are then laid over the scene
// (oit_composite.glsl) and their depth written; the rest of the bucket (additive glows, subtractive meshes, meshes laid
// over others) draws in the game's order after them.
#pragma once
#include "gfx/device.hpp"
#include "gfx/presenter.hpp"

#include <span>

class OrderIndependentTranslucency
{
  public:
    // the scene sets the composite reads the targets through (every view's)
    OrderIndependentTranslucency(argon::gfx::Device &device, argon::gfx::Presenter &presenter, std::span<const argon::gfx::SceneSet> sceneSets);
    OrderIndependentTranslucency(const OrderIndependentTranslucency &) = delete;
    OrderIndependentTranslucency &operator=(const OrderIndependentTranslucency &) = delete;

    // between frames: the targets kept (on) or freed (off), the GPU idle when that changes, and bound to the scene sets
    // whenever they were made again
    void prepare(bool on);
    bool active() const
    {
        return on_;
    }
    // the mesh program into the targets (blends fixed by RenderTarget::Oit; depth tested, not written)
    argon::gfx::Pipeline meshPipeline() const
    {
        return mesh_;
    }
    // the mesh program for the same meshes' solid pixels, into the scene before begin (unblended, depth written)
    argon::gfx::Pipeline solidPipeline() const
    {
        return solid_;
    }
    // the same meshes' depth after composite, every pixel as the game writes it (the colour kept), so the translucent
    // draws after them are hidden behind them as in the game (Draco's sand under its grass)
    argon::gfx::Pipeline depthPipeline() const
    {
        return depth_;
    }
    // inside the scene pass, active: the targets cleared and drawn into until composite
    void begin();
    // the targets laid over the view (its viewport and scene set still bound), the scene pass going on after
    void composite(argon::gfx::CommandList &commands);

  private:
    argon::gfx::Device &device_;
    argon::gfx::Presenter &presenter_;
    std::span<const argon::gfx::SceneSet> sceneSets_;
    argon::gfx::Pipeline mesh_, solid_, depth_, composite_;
    uint64_t boundMade_ = 0;   // the presenter's oitTargetsMade when the scene sets were last bound
    bool on_ = false;
};
