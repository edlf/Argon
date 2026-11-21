// The game's .dds textures byte for byte: the standard 128-byte DDS header (magic + DDS_HEADER), but the FourCC slot holds
// an Xbox D3DFORMAT; the data that follows is linear (not swizzled). Field meanings: docs\models\README.md "Textures".
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

namespace xbox_format
{
constexpr uint32_t kDxt1 = 0x0c;
constexpr uint32_t kDxt3 = 0x0e;
constexpr uint32_t kDxt5 = 0x0f;
} // namespace xbox_format

constexpr uint32_t kDdsAlphaPixels = 0x1;        // DdsPixelFormat::flags
constexpr uint32_t kDdsCubeMapAllFaces = 0xfe00;   // DdsHeader::caps2: the cube map flag and all six faces

#pragma pack(push, 1)

struct DdsPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t xboxFormat;          // the FourCC slot: an xbox_format, or 0 with the masks
    uint32_t rgbBitCount;
    uint32_t redMask, greenMask, blueMask, alphaMask;
};

struct DdsHeader
{
    char magic[4];                // "DDS "
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    DdsPixelFormat pixelFormat;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3, caps4;
    uint32_t reserved2;
};

#pragma pack(pop)

static_assert(sizeof(DdsPixelFormat) == 0x20 && sizeof(DdsHeader) == 0x80);
static_assert(offsetof(DdsHeader, height) == 12 && offsetof(DdsHeader, mipMapCount) == 28 && offsetof(DdsHeader, pixelFormat) == 76);
static_assert(offsetof(DdsHeader, pixelFormat.xboxFormat) == 84 && offsetof(DdsHeader, pixelFormat.alphaMask) == 104);
static_assert(offsetof(DdsHeader, caps2) == 112);

} // namespace argon::original
