// Argon: a texture as the renderer keeps it, the same for every backend: its handle and what the materials read of it.
#pragma once
#include "gfx/gfx_types.hpp"

#include <cstdint>

namespace argon::gfx
{

struct Texture
{
    TextureRef handle;
    bool valid = false;
    bool cube = false;
    uint32_t layers = 1;   // animation frames
    bool alpha = false;    // format has alpha
    bool cutout = false;   // alpha looks like a stencil (used by "Auto")
    bool bump = false;     // texture table type 3: normal map
};

}  // namespace argon::gfx
