#include "files/dds.hpp"
#include "model/cmm.hpp"
#include "original/dds_records.hpp"
#include "original/record_read.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace argon
{

namespace xbox_format = original::xbox_format;
constexpr size_t kDdsData = sizeof(original::DdsHeader);

static uint8_t chan(uint32_t v, uint32_t mask)
{
    if (!mask)
    {
        return 255;
    }
    unsigned shift = 0;
    while (!((mask >> shift) & 1))
    {
        ++shift;
    }
    uint32_t mx = mask >> shift;
    return uint8_t(((v & mask) >> shift) * 255 / mx);
}

// alpha statistics of level 0 for BC1/BC2/BC3 (only alpha is decoded)
static void bcAlphaStats(TextureData &texture)
{
    const auto &level = texture.levels[0];
    size_t bw = std::max(1u, (level.w + 3) / 4), bh = std::max(1u, (level.h + 3) / 4), blocks = bw * bh;
    size_t clear = 0, partial = 0;
    const uint8_t *blockBytes = texture.bytes.data() + level.offset;
    for (size_t b = 0; b < blocks; ++b)
    {
        uint8_t alphas[16];
        if (texture.format == argon::gfx::TextureFormat::Bc1)
        {
            const uint8_t *block = blockBytes + b * 8;
            uint16_t c0 = uint16_t(block[0] | block[1] << 8), c1 = uint16_t(block[2] | block[3] << 8);
            uint32_t bits = uint32_t(block[4] | block[5] << 8 | block[6] << 16 | uint32_t(block[7]) << 24);
            for (int i = 0; i < 16; ++i)
            {
                alphas[i] = (c0 <= c1 && ((bits >> (2 * i)) & 3) == 3) ? 0 : 255;
            }
        }
        else if (texture.format == argon::gfx::TextureFormat::Bc2)
        {
            const uint8_t *block = blockBytes + b * 16;
            for (int i = 0; i < 16; ++i)
            {
                alphas[i] = uint8_t(((block[i / 2] >> ((i & 1) * 4)) & 15) * 17);
            }
        }
        else
        {
            const uint8_t *block = blockBytes + b * 16;
            uint8_t a0 = block[0], a1 = block[1], pal[8] = {a0, a1};
            if (a0 > a1)
            {
                for (int k = 0; k < 6; ++k)
                {
                    pal[2 + k] = uint8_t(((6 - k) * a0 + (k + 1) * a1) / 7);
                }
            }
            else
            {
                for (int k = 0; k < 4; ++k)
                {
                    pal[2 + k] = uint8_t(((4 - k) * a0 + (k + 1) * a1) / 5);
                }
                pal[6] = 0;
                pal[7] = 255;
            }
            uint64_t bits = 0;
            for (int k = 0; k < 6; ++k)
            {
                bits |= uint64_t(block[2 + k]) << (8 * k);
            }
            for (int i = 0; i < 16; ++i)
            {
                alphas[i] = pal[(bits >> (3 * i)) & 7];
            }
        }
        for (int i = 0; i < 16; ++i)
        {
            if (alphas[i] < 16)
            {
                ++clear;
            }
            else if (alphas[i] < 240)
            {
                ++partial;
            }
        }
    }
    texture.alphaClear = float(clear) / float(blocks * 16);
    texture.alphaPartial = float(partial) / float(blocks * 16);
}

TextureData loadDds(const std::string &path)
{
    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes))
    {
        throw std::runtime_error("cannot read " + path);
    }
    return parseDds(bytes, path);
}

TextureData loadOptionalDds(const std::string &path)
{
    try
    {
        return loadDds(path);
    }
    catch (const std::runtime_error &)
    {
        return {};
    }
}

namespace
{

// the file and its header, as the parts of parseDds read them
struct DdsSource
{
    const std::vector<uint8_t> &file;
    const original::DdsHeader &header;
};

// DXT1 / 3 / 5: the format, whether it has alpha and its block size; 0 for an uncompressed file
uint32_t pickBlockFormat(const original::DdsPixelFormat &pixels, TextureData &texture)
{
    if (pixels.xboxFormat == xbox_format::kDxt1)
    {
        texture.format = argon::gfx::TextureFormat::Bc1;
        texture.hasAlpha = (pixels.flags & original::kDdsAlphaPixels) != 0;
        return 8;
    }
    if (pixels.xboxFormat == xbox_format::kDxt3)
    {
        texture.format = argon::gfx::TextureFormat::Bc2;
        texture.hasAlpha = true;
        return 16;
    }
    if (pixels.xboxFormat == xbox_format::kDxt5)
    {
        texture.format = argon::gfx::TextureFormat::Bc3;
        texture.hasAlpha = true;
        return 16;
    }
    return 0;
}

// the compressed mip chain, as far as the file holds it
void readMipChain(const DdsSource &dds, uint32_t block, TextureData &texture)
{
    const uint32_t mips = std::max(1u, dds.header.mipMapCount);
    size_t offset = kDdsData;
    uint32_t w = texture.width, h = texture.height;
    for (uint32_t i = 0; i < mips; ++i)
    {
        const size_t size = size_t(std::max(1u, (w + 3) / 4)) * std::max(1u, (h + 3) / 4) * block;
        if (offset + size > dds.file.size())
        {
            break;
        }
        texture.levels.push_back({w, h, offset - kDdsData, size});
        offset += size;
        if (w == 1 && h == 1)
        {
            break;
        }
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }
}

// the chain's bytes, six times for a cube map (each face with its own chain), and the alpha it shows
void keepCompressedBytes(const DdsSource &dds, TextureData &texture)
{
    texture.layerStride = texture.levels.back().offset + texture.levels.back().size;
    const bool allFaces = (dds.header.caps2 & original::kDdsCubeMapAllFaces) == original::kDdsCubeMapAllFaces;
    if (allFaces && kDdsData + texture.layerStride * 6 <= dds.file.size() && texture.width == texture.height)
    {
        texture.layers = 6;
        texture.cube = true;
    }
    texture.bytes.assign(dds.file.begin() + kDdsData, dds.file.begin() + kDdsData + texture.layerStride * texture.layers);
    bcAlphaStats(texture);
    if (texture.format == argon::gfx::TextureFormat::Bc1)
    {
        texture.hasAlpha = texture.alphaClear > 0;
    }
}

struct PixelMasks
{
    uint32_t red, green, blue, alpha;
};

// a 32-bit file without colour masks is A8R8G8B8
PixelMasks pixelMasks(const original::DdsPixelFormat &pixels)
{
    if (pixels.rgbBitCount == 32 && !(pixels.redMask | pixels.greenMask | pixels.blueMask))
    {
        return {0xff0000, 0xff00, 0xff, 0xff000000};
    }
    return {pixels.redMask, pixels.greenMask, pixels.blueMask, pixels.alphaMask};
}

// uncompressed, stored linear: level 0 expanded to RGBA8; false when the file is too short
bool expandToRgba(const DdsSource &dds, TextureData &texture)
{
    const PixelMasks masks = pixelMasks(dds.header.pixelFormat);
    const size_t px = size_t(texture.width) * texture.height, bpp = dds.header.pixelFormat.rgbBitCount / 8;
    if (kDdsData + px * bpp > dds.file.size())
    {
        return false;
    }
    texture.format = argon::gfx::TextureFormat::Rgba8;
    texture.hasAlpha = masks.alpha != 0;
    texture.bytes.resize(px * 4);
    for (size_t i = 0; i < px; ++i)
    {
        uint32_t v = 0;
        std::memcpy(&v, dds.file.data() + kDdsData + i * bpp, bpp);
        texture.bytes[i * 4 + 0] = chan(v, masks.red);
        texture.bytes[i * 4 + 1] = chan(v, masks.green);
        texture.bytes[i * 4 + 2] = chan(v, masks.blue);
        const uint8_t alpha = masks.alpha ? chan(v, masks.alpha) : 255;
        texture.bytes[i * 4 + 3] = alpha;
        if (alpha < 16)
        {
            texture.alphaClear += 1;
        }
        else if (alpha < 240)
        {
            texture.alphaPartial += 1;
        }
    }
    texture.alphaClear /= float(px);
    texture.alphaPartial /= float(px);
    texture.levels.push_back({texture.width, texture.height, 0, texture.bytes.size()});
    texture.layerStride = texture.bytes.size();
    return true;
}

}  // namespace

TextureData parseDds(const std::vector<uint8_t> &bytes, const std::string &path)
{
    TextureData texture;
    if (bytes.size() < sizeof(original::DdsHeader) || std::memcmp(bytes.data(), "DDS ", 4) != 0)
    {
        throw std::runtime_error(path + ": not a DDS file");
    }
    const auto header = original::readAt<original::DdsHeader>(bytes, 0);
    texture.height = header.height;
    texture.width = header.width;
    if (!texture.width || !texture.height)
    {
        throw std::runtime_error(path + ": zero size");
    }
    const DdsSource dds{bytes, header};
    if (const uint32_t block = pickBlockFormat(header.pixelFormat, texture))
    {
        readMipChain(dds, block, texture);
        if (texture.levels.empty())
        {
            throw std::runtime_error(path + ": truncated");
        }
        keepCompressedBytes(dds, texture);
        return texture;
    }
    const uint32_t bits = header.pixelFormat.rgbBitCount;
    if (bits == 16 || bits == 32)
    {
        if (!expandToRgba(dds, texture))
        {
            throw std::runtime_error(path + ": truncated");
        }
        return texture;
    }
    throw std::runtime_error(path + ": unsupported format " + std::to_string(header.pixelFormat.xboxFormat) + "/" + std::to_string(bits) + " bits");
}

TextureData loadDdsFrames(const std::string &path, uint32_t frames)
{
    TextureData texture;
    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes))
    {
        throw std::runtime_error("cannot read " + path);
    }
    if (frames <= 1 || bytes.size() % frames)
    {
        return parseDds(bytes, path);
    }
    size_t stride = bytes.size() / frames;
    for (uint32_t f = 0; f < frames; ++f)
    {
        std::vector<uint8_t> part(bytes.begin() + f * stride, bytes.begin() + (f + 1) * stride);
        const TextureData fr = parseDds(part, path);
        if (f == 0)
        {
            texture = fr;
            texture.bytes.clear();
            texture.layers = frames;
            texture.cube = false;
        }
        else if (fr.width != texture.width || fr.height != texture.height || fr.format != texture.format || fr.layerStride != texture.layerStride)
        {
            throw std::runtime_error(path + ": frames differ");
        }
        texture.bytes.insert(texture.bytes.end(), fr.bytes.begin(), fr.bytes.begin() + fr.layerStride);
    }
    return texture;
}

} // namespace argon
