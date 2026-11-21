// Argon: the texel formats a texture is uploaded in, the same for every backend (each maps them:
// gfx\vulkan\vulkan_formats.hpp, gfx\d3d12\d3d12_device.cpp): the game's DXT1 / DXT3 / DXT5 as BC1 / BC2 / BC3, and
// 8-bit RGBA (decoded pictures) or BGRA (the movies' frames).
#pragma once
#include <cstdint>

namespace argon::gfx
{

enum class TextureFormat : uint8_t
{
    Undefined,
    Bc1,     // DXT1: 4 bits a texel, 1-bit alpha
    Bc2,     // DXT3: explicit 4-bit alpha
    Bc3,     // DXT5: interpolated alpha
    Rgba8,
    Bgra8
};

}  // namespace argon::gfx
