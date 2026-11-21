#include "render/texture_bind_cache.hpp"

namespace argon
{
void TextureBindCache::bindMesh(const void *model, const MeshTextures &mesh)
{
    for (int stage = 0; stage < kTextureStages; ++stage)
    {
        if (stage < mesh.count)
        {
            bind(stage, StageBind{TextureSlot{model, mesh.texture[size_t(stage)]}, mesh.rate[size_t(stage)]});
        }
        else
        {
            empty(stage);
        }
    }
}

// 0xcbbd2: the same slot already on the stage keeps its bind (and the frame it showed)
void TextureBindCache::bind(int stage, const StageBind &request)
{
    std::optional<StageBind> &current = bound_[size_t(stage)];
    if (current && current->slot == request.slot)
    {
        return;
    }
    current = request;
}

void TextureBindCache::empty(int stage)
{
    bound_[size_t(stage)].reset();
}

void TextureBindCache::emptyAll()
{
    bound_.fill(std::nullopt);
}

std::optional<StageBind> TextureBindCache::bound(int stage) const
{
    return bound_[size_t(stage)];
}
} // namespace argon
