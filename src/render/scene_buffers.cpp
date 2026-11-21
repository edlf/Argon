// Argon: the scene constants' buffers (scene_buffers.hpp).
#include "render/scene_buffers.hpp"

#include <cstring>

void SceneBuffers::create(argon::gfx::Device &device, const argon::gfx::Texture &white, int slots, std::span<const argon::gfx::BufferRef> meshDraws)
{
    const size_t slotsPerFrame = size_t(slots) / meshDraws.size();
    for (int slot = 0; slot < slots; ++slot)
    {
        buffers_.push_back(device.createStreamBuffer(argon::gfx::BufferUse::Uniform, sizeof(SceneUbo)));
        mapped_.push_back(device.mapped(buffers_.back()));
        sets_.push_back(device.createSceneSet(buffers_.back(), meshDraws[size_t(slot) / slotsPerFrame], white.handle));
    }
}

void SceneBuffers::destroy(argon::gfx::Device &device)
{
    for (argon::gfx::BufferRef &buffer : buffers_)
    {
        device.destroyBuffer(buffer);
    }
}

void SceneBuffers::write(int slot, const SceneUbo &constants)
{
    std::memcpy(mapped_[size_t(slot)], &constants, sizeof constants);
}

argon::gfx::SceneSet SceneBuffers::set(int slot) const
{
    return sets_[size_t(slot)];
}

std::span<const argon::gfx::SceneSet> SceneBuffers::sets() const
{
    return sets_;
}
