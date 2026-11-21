// Xbox-style .dds used by Quantum Redshift: normal DDS header, but the FourCC
// slot holds an Xbox D3DFORMAT (0x0C DXT1, 0x0E DXT3, 0x0F DXT5) or 0 with RGB masks.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "gfx/texture_format.hpp"

namespace argon
{

struct TextureData
{
    uint32_t width = 0, height = 0;
    gfx::TextureFormat format = gfx::TextureFormat::Undefined;   // BC1/BC2/BC3 or R8G8B8A8_UNORM
    bool hasAlpha = false;                   // format carries alpha
    float alphaClear = 0, alphaPartial = 0;  // fraction of level-0 texels with a<16 / 16<=a<240
    // "Auto" cutout: alpha looks like a stencil (mostly 0 or 255), not a gloss/blend mask
    bool looksLikeCutout() const
    {
        return hasAlpha && alphaClear > 0.05f && alphaPartial < 0.2f;
    }
    struct Level
    {
        uint32_t w, h;
        size_t offset, size;
    };
    std::vector<Level> levels;               // mip chain of layer 0
    uint32_t layers = 1;                     // cube faces (+X,-X,+Y,-Y,+Z,-Z) or animation frames
    bool cube = false;
    size_t layerStride = 0;                  // bytes between faces
    std::vector<uint8_t> bytes;              // all layers/levels, tightly packed
};

// a DDS file's texture; throws std::runtime_error naming the file and what is wrong (unreadable, not a DDS,
// zero size, truncated, an unsupported format)
TextureData loadDds(const std::string &path);
TextureData parseDds(const std::vector<uint8_t> &file, const std::string &name);
// Animated textures (texture table type 4/6/8): `frames` complete DDS files back to back.
// Result is a 2D array with one layer per frame. frames <= 1 behaves like loadDds.
TextureData loadDdsFrames(const std::string &path, uint32_t frames);
// a texture the game may do without: none (no levels) when its file is missing or unreadable
TextureData loadOptionalDds(const std::string &path);

} // namespace argon
