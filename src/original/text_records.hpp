// The game's text files byte for byte: the string tables (Locale\<language>\*.str, string_table_open 0x79030) and the
// fonts' glyph maps (fonts\*.map). Name prefixes as in cmm_records.hpp. Field meanings: docs\physics\README.md (fonts,
// strings), docs\frontend\README.md (the string tables' languages).
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

constexpr int kFontMapChars = 1024;
constexpr int kFontMapColumns = 5;

#pragma pack(push, 1)

// .str: the header, then its blocks. The first block holds the strings: a StringBlockHeader, `count` StringEntry
// records, a terminating entry (id 0xffff), then the UTF-16 strings, each ending in 0
struct StringTableHeader
{
    char magic[4];                // "LSBL"
    uint32_t headerSize;          // 0xc + 4 x blockCount
    uint32_t blockCount;          // 2..4
    uint32_t firstBlock;          // its offset; blockCount - 1 more block offsets follow
};

struct StringBlockHeader
{
    uint32_t size;                // the whole block's, up to the next block
    uint32_t count;
};

struct StringEntry
{
    uint16_t id;
    uint32_t offset;              // in UTF-16 units from the strings' start (after the terminating entry)
};

// .map (u16 first = 32 in both files)
struct FontMap
{
    uint16_t firstChar;
    uint16_t unused02;            // 0xbaad (uninitialised memory)
    uint32_t charCount;           // 344
    float lineHeight;             // 19 / 35 pixels
    float glyphs[kFontMapColumns][kFontMapChars];   // by char - firstChar: u0, v0, u1, v1, advance (texture units)
};

// .abc (fonts\MSPGoth.abc, the Japanese font: both font slots load it when the language is 2, 0x76d90): the header,
// then `glyphCount` AbcGlyph records sorted by code. font_draw_text 0xb1660 reads only the line height, the first
// code, the count and each glyph's box and width; the char lookup 0xb0f00 searches the codes, entry 0 when missing
struct AbcFontHeader
{
    uint32_t kind;                // 0x80000004
    int32_t lineHeight;           // 31 pixels (0xb0f30)
    uint32_t textureWidth;        // 1024: MSPGoth.dds
    uint32_t textureHeight;       // 1024
    uint32_t unknown10;           // 16
    uint16_t firstChar;           // 0x20
    uint16_t lastChar;            // 0xff5e
    uint32_t glyphCount;          // 777
};

struct AbcGlyph
{
    float u0, v0, u1, v1;         // the glyph's cell (texture units), 32 texels high
    int16_t unused10;             // 0..4 (never read)
    int16_t width;                // pixels, the cell's width - 1: the box's width and the pen's advance (0xb1dba)
    int16_t unused14;             // (never read)
    uint16_t code;                // UTF-16; entry 0 is U+25A1, drawn for the characters the font lacks
};

#pragma pack(pop)

static_assert(sizeof(StringTableHeader) == 0x10 && sizeof(StringBlockHeader) == 8 && sizeof(StringEntry) == 6);
static_assert(sizeof(FontMap) == 12 + kFontMapColumns * kFontMapChars * 4 && offsetof(FontMap, glyphs) == 12);
static_assert(sizeof(AbcFontHeader) == 0x1c && offsetof(AbcFontHeader, firstChar) == 0x14 && offsetof(AbcFontHeader, glyphCount) == 0x18);
static_assert(sizeof(AbcGlyph) == 0x18 && offsetof(AbcGlyph, width) == 0x12 && offsetof(AbcGlyph, code) == 0x16);

} // namespace argon::original
