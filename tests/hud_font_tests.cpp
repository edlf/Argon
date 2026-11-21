// Argon: tests for the HUD fonts' glyph boxes and the replacement font's atlas (hud/hud_font.hpp). Run with test.bat /
// test.sh. The replacement font tests read fonts\Square 721 Extended Bold.otf and fonts\MS PGothic.otf, the game font
// ones game_data\fonts\MSPGoth.abc; each is skipped without its file.
#include "hud/hud_font.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>

using namespace argon::hud;

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr float kTextureSize = 512.f;
constexpr int kLetterA = 'A' - 32;

// a map of 19-high lines whose printable ASCII glyphs are 15 x 23 texel boxes advancing 10 texels
std::unique_ptr<argon::original::FontMap> squareGlyphMap()
{
    auto map = std::make_unique<argon::original::FontMap>();
    *map = {};
    map->firstChar = 32;
    map->charCount = 95;
    map->lineHeight = 19.f;
    for (int i = 0; i < 95; ++i)
    {
        map->glyphs[0][i] = 0.f;
        map->glyphs[1][i] = 0.f;
        map->glyphs[2][i] = 15.f / kTextureSize;
        map->glyphs[3][i] = 23.f / kTextureSize;
        map->glyphs[4][i] = 10.f / kTextureSize;
    }
    return map;
}

std::vector<uint8_t> projectFile(const char *path)
{
    std::ifstream in(std::string(QR_SOURCE_DIR "/") + path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::vector<uint8_t> replacementFontFile()
{
    return projectFile("fonts/Square 721 Extended Bold.otf");
}

// an .abc font of 31-high lines: the missing-character box, then "あ" and "い", each `width` pixels wide
std::vector<uint8_t> kanaAbc(int16_t width)
{
    argon::original::AbcFontHeader header{0x80000004u, 31, 1024, 1024, 16, 0x20, 0xff5e, 3};
    const uint16_t codes[3] = {0x25a1, 0x3042, 0x3044};
    std::vector<uint8_t> file(sizeof(header) + 3 * sizeof(argon::original::AbcGlyph));
    std::memcpy(file.data(), &header, sizeof(header));
    for (int i = 0; i < 3; ++i)
    {
        const argon::original::AbcGlyph glyph{0.f, 0.f, 32.f / 1024.f, 32.f / 1024.f, 1, width, 0, codes[i]};
        std::memcpy(file.data() + sizeof(header) + size_t(i) * sizeof(glyph), &glyph, sizeof(glyph));
    }
    return file;
}

void anAbcGlyphAdvancesByItsWidthScaledToTheSlot()
{
    const FontLayout layout = layoutFromAbc(kanaAbc(31), kAbcFits[1]);
    CHECK(std::fabs(layout.glyphs[layout.glyphFor(u'あ')].advance - 35.f) < 1e-4f);
}

void anAbcGlyphIsAsHighAsTheSlotsGlyphs()
{
    const FontLayout layout = layoutFromAbc(kanaAbc(31), kAbcFits[0]);
    CHECK(std::fabs(layout.glyphs[layout.glyphFor(u'い')].height - 19.f * 1.1f) < 1e-4f);
}

void anAbcFontKeepsTheSlotsLine()
{
    CHECK(layoutFromAbc(kanaAbc(31), kAbcFits[0]).line == 19.f);
}

void anAbcFontFindsACharacterByItsCode()
{
    CHECK(layoutFromAbc(kanaAbc(31), kAbcFits[0]).glyphFor(u'い') == 2);
}

void anAbcFontDrawsItsBoxForAMissingCharacter()
{
    CHECK(layoutFromAbc(kanaAbc(31), kAbcFits[0]).glyphFor(u'A') == 0);
}

void aMapFontDrawsItsFallbackGlyphForAnUnknownCharacter()
{
    CHECK(layoutFromMap(*squareGlyphMap(), kTextureSize).glyphFor(u'あ') == 0xe0);
}

void aTruncatedAbcFontIsRefused()
{
    std::vector<uint8_t> file = kanaAbc(31);
    file.resize(file.size() - 1);
    bool refused = false;
    try
    {
        layoutFromAbc(file, kAbcFits[0]);
    }
    catch (const std::runtime_error &)
    {
        refused = true;
    }
    CHECK(refused);
}

void theJapaneseReplacementKeepsTheGameAdvances()
{
    const std::vector<uint8_t> font = projectFile("fonts/MS PGothic.otf");
    if (font.empty())
    {
        return;
    }
    const GlyphAtlas atlas = rasterizeFont(font, layoutFromAbc(kanaAbc(31), kAbcFits[1]), 4);
    CHECK(std::fabs(atlas.layout.glyphs[2].advance - 35.f) < 1e-4f);
}

// the game's whole font in its large slot still fits one texture (16384 texels high at most)
void theJapaneseReplacementAtlasFitsATexture()
{
    const std::vector<uint8_t> font = projectFile("fonts/MS PGothic.otf");
    const std::vector<uint8_t> game = projectFile("game_data/fonts/MSPGoth.abc");
    if (font.empty() || game.empty())
    {
        return;
    }
    const GlyphAtlas atlas = rasterizeFont(font, layoutFromAbc(game, kAbcFits[1]), 4);
    std::printf("hud_font_tests: MS PGothic atlas %ux%u\n", atlas.levels.front().width, atlas.levels.front().height);
    CHECK(atlas.levels.front().height <= 16384);
}

void aGameGlyphIsCentredOnItsAdvance()
{
    const FontLayout layout = layoutFromMap(*squareGlyphMap(), kTextureSize);
    CHECK(layout.glyphs[kLetterA].left == -2.5f);
}

void aGameGlyphIsCentredOnTheLine()
{
    const FontLayout layout = layoutFromMap(*squareGlyphMap(), kTextureSize);
    CHECK(layout.glyphs[kLetterA].top == -2.f);
}

void aGameGlyphAdvancesInScreenUnits()
{
    const FontLayout layout = layoutFromMap(*squareGlyphMap(), kTextureSize);
    CHECK(layout.glyphs[kLetterA].advance == 10.f);
}

void aFileThatIsNoFontIsRefused()
{
    bool refused = false;
    try
    {
        rasterizeFont({1, 2, 3, 4}, layoutFromMap(*squareGlyphMap(), kTextureSize), 4);
    }
    catch (const std::runtime_error &)
    {
        refused = true;
    }
    CHECK(refused);
}

void theReplacementKeepsTheGameAdvances()
{
    const std::vector<uint8_t> font = replacementFontFile();
    if (font.empty())
    {
        return;
    }
    const GlyphAtlas atlas = rasterizeFont(font, layoutFromMap(*squareGlyphMap(), kTextureSize), 4);
    CHECK(atlas.layout.glyphs[kLetterA].advance == 10.f);
}

void theReplacementLetterInkSitsInsideTheLine()
{
    const std::vector<uint8_t> font = replacementFontFile();
    if (font.empty())
    {
        return;
    }
    const FontGlyph letter = rasterizeFont(font, layoutFromMap(*squareGlyphMap(), kTextureSize), 4).layout.glyphs[kLetterA];
    CHECK(letter.top > -5.f && letter.top + letter.height < 24.f);
}

void theReplacementAtlasHasItsMipChain()
{
    const std::vector<uint8_t> font = replacementFontFile();
    if (font.empty())
    {
        return;
    }
    const GlyphAtlas atlas = rasterizeFont(font, layoutFromMap(*squareGlyphMap(), kTextureSize), 4);
    CHECK(atlas.levels.size() == 4 && atlas.levels[1].width * 2 == atlas.levels[0].width);
}

bool hasOpaqueBlack(const GlyphAtlas::Level &level)
{
    for (size_t i = 0; i < level.rgba.size(); i += 4)
    {
        if (level.rgba[i] == 0 && level.rgba[i + 3] == 255)
        {
            return true;
        }
    }
    return false;
}

void theReplacementGlyphsHaveTheGamesBlackOutline()
{
    const std::vector<uint8_t> font = replacementFontFile();
    if (font.empty())
    {
        return;
    }
    const GlyphAtlas atlas = rasterizeFont(font, layoutFromMap(*squareGlyphMap(), kTextureSize), 4);
    CHECK(hasOpaqueBlack(atlas.levels.front()));
}
}  // namespace

int main()
{
    aGameGlyphIsCentredOnItsAdvance();
    aGameGlyphIsCentredOnTheLine();
    aGameGlyphAdvancesInScreenUnits();
    aFileThatIsNoFontIsRefused();
    theReplacementKeepsTheGameAdvances();
    theReplacementLetterInkSitsInsideTheLine();
    theReplacementAtlasHasItsMipChain();
    theReplacementGlyphsHaveTheGamesBlackOutline();
    anAbcGlyphAdvancesByItsWidthScaledToTheSlot();
    anAbcGlyphIsAsHighAsTheSlotsGlyphs();
    anAbcFontKeepsTheSlotsLine();
    anAbcFontFindsACharacterByItsCode();
    anAbcFontDrawsItsBoxForAMissingCharacter();
    aMapFontDrawsItsFallbackGlyphForAnUnknownCharacter();
    aTruncatedAbcFontIsRefused();
    theJapaneseReplacementKeepsTheGameAdvances();
    theJapaneseReplacementAtlasFitsATexture();
    std::printf("hud_font_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
