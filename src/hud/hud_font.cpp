// Argon: the HUD's text fonts (hud_font.hpp).
#include "hud/hud_font.hpp"

#include "original/record_read.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace argon::hud
{
namespace
{
constexpr uint32_t kAtlasWidth = 2048;
constexpr int kMipLevels = 4;            // down to an eighth: oversample 4 drawn at 480 lines
constexpr int kMipSpacing = 1 << kMipLevels;   // texels kept clear between glyphs, so the smaller levels keep them apart
constexpr float kOutlineWidth = 1.5f;     // the black ring round each glyph, in units: the menu panels' outline (App::drawPanel)

// the glyphs the fit and the atlas use: the ones the game's map draws and the font has
struct SourceGlyph
{
    int index = 0;       // in the game's layout
    int outline = 0;     // stbtt glyph
    int advanceUnits = 0;
};

stbtt_fontinfo openFont(const std::vector<uint8_t> &fontFile)
{
    stbtt_fontinfo font{};
    const int offset = fontFile.empty() ? -1 : stbtt_GetFontOffsetForIndex(fontFile.data(), 0);
    if (offset < 0 || !stbtt_InitFont(&font, fontFile.data(), offset))
    {
        throw std::runtime_error("replacement font: not an OpenType / TrueType file");
    }
    return font;
}

std::vector<SourceGlyph> sharedGlyphs(const stbtt_fontinfo &font, const FontLayout &game)
{
    std::vector<SourceGlyph> shared;
    for (size_t i = 0; i < game.glyphs.size(); ++i)
    {
        const int outline = stbtt_FindGlyphIndex(&font, game.codeAt(i));
        if (game.glyphs[i].advance <= 0 || outline == 0)
        {
            continue;
        }
        int advance = 0, bearing = 0;
        stbtt_GetGlyphHMetrics(&font, outline, &advance, &bearing);
        shared.push_back({int(i), outline, advance});
    }
    return shared;
}

// the font units to 640 x 480 units that best match the game's advances (least squares)
float fittedScale(const std::vector<SourceGlyph> &shared, const FontLayout &game)
{
    double gameTimesFont = 0, fontSquared = 0;
    for (const SourceGlyph &glyph : shared)
    {
        gameTimesFont += double(game.glyphs[size_t(glyph.index)].advance) * glyph.advanceUnits;
        fontSquared += double(glyph.advanceUnits) * glyph.advanceUnits;
    }
    if (fontSquared <= 0)
    {
        throw std::runtime_error("replacement font: none of the game's characters are in it");
    }
    return float(gameTimesFont / fontSquared);
}

// the baseline below the line's top: the font's ascent to descent centred on the line
float baselineFromTop(const stbtt_fontinfo &font, float scale, float line)
{
    int ascent = 0, descent = 0, gap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &gap);
    return (line - float(ascent - descent) * scale) * 0.5f + float(ascent) * scale;
}

// glyphs packed in rows left to right, each with its padding
class ShelfPacker
{
  public:
    struct Spot
    {
        uint32_t x, y;
    };
    Spot place(uint32_t width, uint32_t height)
    {
        if (penX + width > kAtlasWidth)
        {
            penX = 0;
            rowTop += rowHeight;
            rowHeight = 0;
        }
        const Spot spot{penX, rowTop};
        penX += width;
        rowHeight = std::max(rowHeight, height);
        return spot;
    }
    uint32_t usedHeight() const
    {
        const uint32_t align = 1u << kMipLevels;
        return std::max((rowTop + rowHeight + align - 1) / align * align, align);
    }

  private:
    uint32_t penX = 0, rowTop = 0, rowHeight = 0;
};

struct PlacedGlyph
{
    SourceGlyph source;
    int x0, y0, width, height;   // the outline's box at the atlas scale, from the pen on the baseline
    ShelfPacker::Spot spot;
};

// the atlas as two planes: the glyph's white and the black ring round it (which also covers the glyph)
struct InkPlanes
{
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> fill, ring;
};

// the ring: the fill grown by a disc of `radius` texels, as horizontal runs of each half width (max of a row's run,
// then of the disc's rows)
void growRing(InkPlanes &planes, const PlacedGlyph &glyph, int padding, int radius)
{
    const int boxWidth = glyph.width + 2 * padding, boxHeight = glyph.height + 2 * padding;
    const size_t origin = size_t(glyph.spot.y) * planes.width + glyph.spot.x;
    std::vector<std::vector<uint8_t>> runs(size_t(radius) + 1, std::vector<uint8_t>(size_t(boxWidth) * boxHeight));
    for (int y = 0; y < boxHeight; ++y)
    {
        for (int x = 0; x < boxWidth; ++x)
        {
            runs[0][size_t(y) * boxWidth + x] = planes.fill[origin + size_t(y) * planes.width + x];
        }
    }
    for (int half = 1; half <= radius; ++half)
    {
        for (int y = 0; y < boxHeight; ++y)
        {
            for (int x = 0; x < boxWidth; ++x)
            {
                const size_t at = size_t(y) * boxWidth + x;
                uint8_t v = runs[size_t(half) - 1][at];
                v = std::max(v, x - half >= 0 ? runs[0][at - half] : uint8_t(0));
                v = std::max(v, x + half < boxWidth ? runs[0][at + half] : uint8_t(0));
                runs[size_t(half)][at] = v;
            }
        }
    }
    for (int y = 0; y < boxHeight; ++y)
    {
        for (int x = 0; x < boxWidth; ++x)
        {
            uint8_t v = 0;
            for (int dy = -radius; dy <= radius; ++dy)
            {
                if (y + dy < 0 || y + dy >= boxHeight)
                {
                    continue;
                }
                const int half = int(std::sqrt(float(radius * radius - dy * dy)) + 0.5f);
                v = std::max(v, runs[size_t(half)][size_t(y + dy) * boxWidth + x]);
            }
            planes.ring[origin + size_t(y) * planes.width + x] = v;
        }
    }
}

// straight alpha: the fill's white over the ring's black
GlyphAtlas::Level whiteOverBlack(const InkPlanes &planes)
{
    GlyphAtlas::Level level{planes.width, planes.height, std::vector<uint8_t>(planes.fill.size() * 4)};
    for (size_t i = 0; i < planes.fill.size(); ++i)
    {
        level.rgba[i * 4 + 0] = level.rgba[i * 4 + 1] = level.rgba[i * 4 + 2] = planes.fill[i];
        level.rgba[i * 4 + 3] = std::max(planes.ring[i], planes.fill[i]);
    }
    return level;
}

std::vector<uint8_t> halved(const std::vector<uint8_t> &plane, uint32_t width, uint32_t height)
{
    const uint32_t halfWidth = width / 2, halfHeight = height / 2;
    std::vector<uint8_t> out(size_t(halfWidth) * halfHeight);
    for (uint32_t y = 0; y < halfHeight; ++y)
    {
        for (uint32_t x = 0; x < halfWidth; ++x)
        {
            const size_t at = size_t(y) * 2 * width + size_t(x) * 2;
            const int sum = plane[at] + plane[at + 1] + plane[at + width] + plane[at + width + 1];
            out[size_t(y) * halfWidth + x] = uint8_t((sum + 2) / 4);
        }
    }
    return out;
}

std::vector<GlyphAtlas::Level> mipChain(InkPlanes planes)
{
    std::vector<GlyphAtlas::Level> levels;
    for (int level = 0; level < kMipLevels; ++level)
    {
        levels.push_back(whiteOverBlack(planes));
        planes.fill = halved(planes.fill, planes.width, planes.height);
        planes.ring = halved(planes.ring, planes.width, planes.height);
        planes.width /= 2;
        planes.height /= 2;
    }
    return levels;
}
}  // namespace

size_t FontLayout::glyphFor(char16_t character) const
{
    if (glyphs.empty())
    {
        return 0;
    }
    if (!codes.empty())
    {
        const auto found = std::lower_bound(codes.begin(), codes.end(), character);
        return found != codes.end() && *found == character ? size_t(found - codes.begin()) : 0;
    }
    int index = int(character) - first;
    if ((index < 0 || index > count) && index != 0x178 - first)
    {
        index = 0xe0;
    }
    return size_t(std::clamp(index, 0, int(glyphs.size()) - 1));
}

char16_t FontLayout::codeAt(size_t glyph) const
{
    return codes.empty() ? char16_t(first + int(glyph)) : codes[glyph];
}

FontLayout layoutFromMap(const original::FontMap &map, float textureSize)
{
    FontLayout layout;
    layout.first = int(map.firstChar);
    layout.count = int(map.charCount);
    layout.line = map.lineHeight;
    layout.glyphs.resize(original::kFontMapChars);
    for (int i = 0; i < original::kFontMapChars; ++i)
    {
        FontGlyph &glyph = layout.glyphs[size_t(i)];
        glyph.u0 = map.glyphs[0][i];
        glyph.v0 = map.glyphs[1][i];
        glyph.u1 = map.glyphs[2][i];
        glyph.v1 = map.glyphs[3][i];
        glyph.advance = map.glyphs[4][i] * textureSize;
        glyph.width = (glyph.u1 - glyph.u0) * textureSize;
        glyph.height = (glyph.v1 - glyph.v0) * textureSize;
        glyph.left = (glyph.advance - glyph.width) * 0.5f;
        glyph.top = (layout.line - glyph.height) * 0.5f;
    }
    return layout;
}

FontLayout layoutFromAbc(const std::vector<uint8_t> &abc, const AbcFit &fit)
{
    if (abc.size() < sizeof(original::AbcFontHeader))
    {
        throw std::runtime_error("font .abc: shorter than its header");
    }
    const auto header = original::readAt<original::AbcFontHeader>(abc, 0);
    if (header.lineHeight <= 0 || abc.size() < sizeof(header) + size_t(header.glyphCount) * sizeof(original::AbcGlyph))
    {
        throw std::runtime_error("font .abc: its glyphs run past the file's end");
    }
    const float scale = fit.glyphHeight / float(header.lineHeight);
    FontLayout layout;
    layout.first = header.firstChar;
    layout.count = int(header.glyphCount);
    layout.line = fit.line;
    for (uint32_t i = 0; i < header.glyphCount; ++i)
    {
        const auto source = original::readAt<original::AbcGlyph>(abc, sizeof(header) + i * sizeof(original::AbcGlyph));
        const float width = float(source.width) * scale;
        layout.glyphs.push_back({source.u0, source.v0, source.u1, source.v1, 0.f, 0.f, width, fit.glyphHeight, width});
        layout.codes.push_back(char16_t(source.code));
    }
    return layout;
}

GlyphAtlas rasterizeFont(const std::vector<uint8_t> &fontFile, const FontLayout &game, int oversample)
{
    const stbtt_fontinfo font = openFont(fontFile);
    const std::vector<SourceGlyph> shared = sharedGlyphs(font, game);
    const float scale = fittedScale(shared, game);
    const float atlasScale = scale * float(oversample), perTexel = 1.f / float(oversample);
    const float baseline = baselineFromTop(font, scale, game.line);
    const int ringRadius = int(std::lround(kOutlineWidth * float(oversample)));
    const int padding = ringRadius + kMipSpacing;

    ShelfPacker packer;
    std::vector<PlacedGlyph> placed;
    for (const SourceGlyph &source : shared)
    {
        PlacedGlyph glyph{source, 0, 0, 0, 0, {}};
        int x1 = 0, y1 = 0;
        stbtt_GetGlyphBitmapBox(&font, source.outline, atlasScale, atlasScale, &glyph.x0, &glyph.y0, &x1, &y1);
        glyph.width = std::max(x1 - glyph.x0, 0);
        glyph.height = std::max(y1 - glyph.y0, 0);
        if (glyph.width > 0 && glyph.height > 0)
        {
            glyph.spot = packer.place(uint32_t(glyph.width + 2 * padding), uint32_t(glyph.height + 2 * padding));
            placed.push_back(glyph);
        }
    }

    InkPlanes planes{kAtlasWidth, packer.usedHeight()};
    planes.fill.resize(size_t(planes.width) * planes.height);
    planes.ring.resize(planes.fill.size());
    GlyphAtlas atlas;
    atlas.layout = game;
    for (FontGlyph &glyph : atlas.layout.glyphs)
    {
        glyph = FontGlyph{.advance = glyph.advance};   // characters the font lacks draw nothing
    }
    for (const PlacedGlyph &placedGlyph : placed)
    {
        const size_t at = size_t(placedGlyph.spot.y + padding) * planes.width + placedGlyph.spot.x + padding;
        stbtt_MakeGlyphBitmap(&font, &planes.fill[at], placedGlyph.width, placedGlyph.height, int(planes.width), atlasScale, atlasScale, placedGlyph.source.outline);
        growRing(planes, placedGlyph, padding, ringRadius);
        FontGlyph &glyph = atlas.layout.glyphs[size_t(placedGlyph.source.index)];
        const float boxWidth = float(placedGlyph.width + 2 * padding), boxHeight = float(placedGlyph.height + 2 * padding);
        glyph.u0 = float(placedGlyph.spot.x) / float(planes.width);
        glyph.v0 = float(placedGlyph.spot.y) / float(planes.height);
        glyph.u1 = (float(placedGlyph.spot.x) + boxWidth) / float(planes.width);
        glyph.v1 = (float(placedGlyph.spot.y) + boxHeight) / float(planes.height);
        glyph.width = boxWidth * perTexel;
        glyph.height = boxHeight * perTexel;
        glyph.left = (glyph.advance - float(placedGlyph.source.advanceUnits) * scale) * 0.5f + float(placedGlyph.x0 - padding) * perTexel;
        glyph.top = baseline + float(placedGlyph.y0 - padding) * perTexel;
    }
    atlas.levels = mipChain(std::move(planes));
    return atlas;
}
}  // namespace argon::hud
