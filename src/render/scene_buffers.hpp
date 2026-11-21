// Argon: the scene constants' buffers: a uniform buffer and its scene set per frame in flight and view.
#pragma once
#include "gfx/device.hpp"
#include "gfx/texture.hpp"
#include "render/scene_constants.hpp"

#include <span>
#include <vector>

// a uniform buffer and its scene set per frame in flight and view (App::viewSlot)
class SceneBuffers
{
  public:
    // white: binding 1 (the frame copy) until the first copy exists; meshDraws: binding 4, one buffer per frame in
    // flight, the slots in frame order (App::viewSlot: frame x views + view)
    void create(argon::gfx::Device &device, const argon::gfx::Texture &white, int slots, std::span<const argon::gfx::BufferRef> meshDraws);
    void destroy(argon::gfx::Device &device);
    void write(int slot, const SceneUbo &constants);
    argon::gfx::SceneSet set(int slot) const;
    std::span<const argon::gfx::SceneSet> sets() const;

  private:
    std::vector<argon::gfx::BufferRef> buffers_;
    std::vector<void *> mapped_;
    std::vector<argon::gfx::SceneSet> sets_;
};
