// Argon: the HUD's text fonts as glyph boxes (hud_font.cpp): the game's bitmap fonts (fonts\*.map / .dds,
// original::FontMap; the Japanese fonts\MSPGoth.abc / .dds, original::AbcFontHeader) and their replacement, an OpenType
// font drawn into a sharper atlas (docs\argon\differences.md).
// Positions are in the game's 640 x 480 units.
#pragma once
#include <cstdint>
#include <vector>

#include "original/text_records.hpp"

namespace argon::hud
{
// one glyph: its texture rectangle, its box from the pen (x) and the line's top (y), and the pen's advance
struct FontGlyph
{
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    float left = 0, top = 0, width = 0, height = 0;
    float advance = 0;
};

// a font's characters: glyph n draws character first + n, or codes[n] when the font lists them (.abc)
struct FontLayout
{
    int first = 32, count = 0;
    float line = 0;   // the line height
    std::vector<FontGlyph> glyphs;
    std::vector<char16_t> codes;   // sorted; empty for a .map font

    // the glyph that draws c: a .map font's fallback glyph 0xe0 (font_draw_text 0xb1dcf; 0x178 kept), an .abc font's
    // entry 0 (0xb0f00) when it lacks c
    size_t glyphFor(char16_t character) const;
    char16_t codeAt(size_t glyph) const;
};

// how the game sizes the .abc font in a font slot (0x76dc0): the slot's line (the Square721 fonts': 19, 35) and its
// glyphs' height (font 0's 1.1 x its line)
struct AbcFit
{
    float line, glyphHeight;
};
inline constexpr AbcFit kAbcFits[2] = {{19.f, 19.f * 1.1f}, {35.f, 35.f}};

// the game's glyphs (texture units, one square texture of `textureSize` texels): each box centred on its advance and
// on the line (the game's boxes carry 2 px above and below the line height, 5 px beside the advance)
FontLayout layoutFromMap(const original::FontMap &map, float textureSize);

// the .abc font's glyphs, each box from the pen on the line's top, `width` pixels wide and the font's line high, scaled
// so the line is fit.glyphHeight (font_draw_text 0xb1dba, 0xb23df). Throws std::runtime_error when the file is too short.
FontLayout layoutFromAbc(const std::vector<uint8_t> &abc, const AbcFit &fit);

// an RGBA8 picture (white, the coverage in alpha) with its mip chain, level 0 first
struct GlyphAtlas
{
    struct Level
    {
        uint32_t width = 0, height = 0;
        std::vector<uint8_t> rgba;
    };
    std::vector<Level> levels;
    FontLayout layout;
};

// the game's font drawn again from an OpenType / TrueType file: the game's characters, advances and line height, the
// outlines scaled to fit those advances and drawn `oversample` texels per unit. Throws std::runtime_error when the file
// is no font.
GlyphAtlas rasterizeFont(const std::vector<uint8_t> &fontFile, const FontLayout &game, int oversample);
}  // namespace argon::hud
