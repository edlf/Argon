// Argon: the 2D layer (hud_painter.hpp).
#include "hud/hud_painter.hpp"

#include "model/cmm.hpp"
#include "original/record_read.hpp"
#include "original/text_records.hpp"
#include "ui/menu_common.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace
{
std::vector<uint8_t> readWhole(const std::string &path)
{
    std::vector<uint8_t> bytes;
    if (!argon::readFile(path, bytes))
    {
        bytes.clear();
    }
    return bytes;
}

// font_draw_text 0xb1a12, flags & 0xf0: the text measured in a first pass (its glyph boxes, here the line), then 0x10
// centres it on y (lifted by half its height) and 0x20 puts its bottom on y
float textBlockLift(int flags, float height)
{
    switch (flags & 0xf0)
    {
    case 0x10:
        return height * 0.5f;
    case 0x20:
        return height;
    default:
        return 0.f;
    }
}
}  // namespace

void HudPainter::attach(argon::gfx::Device &device, argon::gfx::UiRenderer &ui, const std::string &gameFolder)
{
    device_ = &device;
    ui_ = &ui;
    game_ = gameFolder;
}

Found<const HudTexture> HudPainter::texture(const std::string &key) const
{
    const auto found = textures_.find(key);
    return found == textures_.end() ? Found<const HudTexture>() : Found<const HudTexture>(found->second);
}

bool HudPainter::japaneseFonts() const
{
    return japaneseFonts_;
}

const HudFont &HudPainter::font(int slot) const
{
    return fonts_[std::clamp(slot, 0, kFonts - 1)];
}

void HudPainter::blend(ImDrawList *drawList, argon::gfx::UiBlend mode)
{
    drawList->AddCallback(mode == argon::gfx::UiBlend::Additive      ? additiveCallback
                    : mode == argon::gfx::UiBlend::Subtractive ? subtractiveCallback
                                                               : panelCallback,
                    this);
}

namespace
{
constexpr int kReplacementOversample = 4;   // atlas texels per 640 x 480 unit: sharp up to 4x the game's 480 lines

// the game's fonts for a language (0x76d90) and the typeface each is drawn again from
struct FontSet
{
    const char *gameFiles[2];      // fonts\<name>.map / .abc and .dds, slot 0 and 1
    const char *replacementFile;   // in a fonts folder (fonts\README.md)
};
constexpr FontSet kLatinFonts{{"Square721 BdEx BT 18", "Square721 BdEx BT 34"}, "Square 721 Extended Bold.otf"};
constexpr FontSet kJapaneseFonts{{"MSPGoth", "MSPGoth"}, "MS PGothic.otf"};

std::string fontTextureKey(int font)
{
    return "font" + std::to_string(font);
}

// the first fonts folder that has it; empty when none does
std::vector<uint8_t> readReplacement(const std::vector<std::filesystem::path> &folders, const char *fileName)
{
    for (const std::filesystem::path &folder : folders)
    {
        std::vector<uint8_t> file = readWhole((folder / fileName).string());
        if (!file.empty())
        {
            return file;
        }
    }
    return {};
}
}  // namespace

namespace
{
const int kPanelTriangles[12] = {0, 1, 2, 0, 2, 3, 0, 3, 5, 5, 3, 4};

ImVec2 scaledPoint(const glm::vec2 &point, float scale)
{
    return ImVec2(point.x * scale, point.y * scale);
}

// a colour's RGB shifted down (the glow's inner colour), its alpha kept
uint32_t dimmedColour(uint32_t argb, int shift)
{
    const uint32_t red = ((argb >> 16) & 0xff) >> shift, green = ((argb >> 8) & 0xff) >> shift, blue = (argb & 0xff) >> shift;
    return (argb & 0xff000000u) | (red << 16) | (green << 8) | blue;
}

// the panel's four triangles at its six corners' colours; plasma: the overlay's scrolling uv (else the white texel)
struct PanelFan
{
    const glm::vec2 *corners;
    const uint32_t *colours;
    float pixels, plasmaScale;
    bool plasma;
    ImVec2 white;
};

void addPanelFan(ImDrawList *drawList, const PanelFan &fan)
{
    drawList->PrimReserve(12, 12);
    // 0xc16f0: t = (global_frame_counter_2 % 1000) / 1000; that counter is bumped by the vertical blank
    // callback (0xb2670), i.e. 60 a second whatever the frame rate: one loop every 1000 / 60 s
    const int frame = int(ImGui::GetTime() * 60.0) % 1000;
    for (int corner : kPanelTriangles)
    {
        const glm::vec2 &at = fan.corners[corner];
        const ImVec2 uv = fan.plasma ? ImVec2(at.x * 0.005f * fan.plasmaScale + 8.f + 32.f * float(frame), at.y * 0.005f * fan.plasmaScale) : fan.white;
        drawList->PrimVtx(scaledPoint(at, fan.pixels), uv, imColour(fan.colours[corner]));
    }
}
}  // namespace

void HudPainter::additiveCallback(const ImDrawList *, const ImDrawCmd *cmd)
{
    static_cast<HudPainter *>(cmd->UserCallbackData)->ui_->useBlend(argon::gfx::UiBlend::Additive);
}

void HudPainter::panelCallback(const ImDrawList *, const ImDrawCmd *cmd)
{
    static_cast<HudPainter *>(cmd->UserCallbackData)->ui_->useBlend(argon::gfx::UiBlend::Panel);
}

void HudPainter::subtractiveCallback(const ImDrawList *, const ImDrawCmd *cmd)
{
    static_cast<HudPainter *>(cmd->UserCallbackData)->ui_->useBlend(argon::gfx::UiBlend::Subtractive);
}

bool HudPainter::loadTexture(const std::string &key, const std::string &path)
{
    return loadTextureSampled(key, path, argon::gfx::SamplerMode::Clamp);
}

bool HudPainter::loadTiledTexture(const std::string &key, const std::string &path)
{
    return loadTextureSampled(key, path, argon::gfx::SamplerMode::Wrap);
}

bool HudPainter::loadTextureSampled(const std::string &key, const std::string &path, argon::gfx::SamplerMode sampling)
{
    const argon::TextureData td = argon::loadOptionalDds(path);
    if (td.levels.empty())
    {
        return false;
    }
    addTexture(key, td, sampling);
    return true;
}

void HudPainter::addTexture(const std::string &key, const argon::TextureData &td, argon::gfx::SamplerMode sampling)
{
    const auto old = textures_.find(key);
    if (old != textures_.end())
    {
        ui_->removeImage(old->second.image);
        device_->destroyTexture(old->second.tex);
        textures_.erase(old);
    }
    HudTexture texture;
    texture.tex = device_->createTexture(td);
    texture.w = int(td.width);
    texture.h = int(td.height);
    texture.image = ui_->addImage(texture.tex, sampling);
    textures_[key] = texture;
}

// fonts 0 and 1: Square721 BdEx BT 18 / 34 (original::FontMap), or for Japanese MSPGoth in both (original::AbcFontHeader,
// sized as the game does: hud::kAbcFits); each drawn again from its replacement typeface when that file is in a fonts
// folder (docs\argon\differences.md), keeping the game's characters, advances and line height
void HudPainter::loadFonts(bool japanese, const std::vector<std::filesystem::path> &replacementFolders)
{
    japaneseFonts_ = japanese;
    const FontSet &set = japaneseFonts_ ? kJapaneseFonts : kLatinFonts;
    const std::vector<uint8_t> replacement = readReplacement(replacementFolders, set.replacementFile);
    for (int slot = 0; slot < 2; ++slot)
    {
        fonts_[slot] = HudFont{};
        const bool loaded = japaneseFonts_ ? loadAbcFont(slot, set.gameFiles[slot]) : loadMapFont(slot, set.gameFiles[slot]);
        if (!loaded)
        {
            continue;
        }
        fonts_[slot].ok = true;
        if (!replacement.empty())
        {
            loadReplacementFont(slot, replacement);
        }
    }
}

bool HudPainter::loadMapFont(int slot, const std::string &name)
{
    const std::vector<uint8_t> file = readWhole(game_ + "/fonts/" + name + ".map");
    if (file.size() < sizeof(argon::original::FontMap) || !loadTexture(fontTextureKey(slot), game_ + "/fonts/" + name + ".dds"))
    {
        return false;
    }
    const auto map = argon::original::readAt<argon::original::FontMap>(file, 0);
    static_cast<argon::hud::FontLayout &>(fonts_[slot]) = argon::hud::layoutFromMap(map, float(textures_[fontTextureKey(slot)].w));
    return true;
}

bool HudPainter::loadAbcFont(int slot, const std::string &name)
{
    try
    {
        argon::hud::FontLayout layout = argon::hud::layoutFromAbc(readWhole(game_ + "/fonts/" + name + ".abc"), argon::hud::kAbcFits[slot]);
        if (!loadTexture(fontTextureKey(slot), game_ + "/fonts/" + name + ".dds"))
        {
            return false;
        }
        static_cast<argon::hud::FontLayout &>(fonts_[slot]) = std::move(layout);
        return true;
    }
    catch (const std::runtime_error &e)
    {
        std::fprintf(stderr, "fonts\\%s: %s; font %d is not drawn\n", name.c_str(), e.what(), slot);
        return false;
    }
}


void HudPainter::loadReplacementFont(int font, const std::vector<uint8_t> &fontFile)
{
    try
    {
        argon::hud::GlyphAtlas atlas = argon::hud::rasterizeFont(fontFile, fonts_[font], kReplacementOversample);
        argon::TextureData picture;
        picture.width = atlas.levels.front().width;
        picture.height = atlas.levels.front().height;
        picture.format = argon::gfx::TextureFormat::Rgba8;
        picture.hasAlpha = true;
        for (const argon::hud::GlyphAtlas::Level &level : atlas.levels)
        {
            picture.levels.push_back({level.width, level.height, picture.bytes.size(), level.rgba.size()});
            picture.bytes.insert(picture.bytes.end(), level.rgba.begin(), level.rgba.end());
        }
        picture.layerStride = picture.bytes.size();
        addTexture(fontTextureKey(font), picture, argon::gfx::SamplerMode::Clamp);
        static_cast<argon::hud::FontLayout &>(fonts_[font]) = std::move(atlas.layout);
    }
    catch (const std::runtime_error &e)
    {
        std::fprintf(stderr, "%s; the game's font %d is drawn\n", e.what(), font);
    }
}

void HudPainter::freeAll()
{
    for (auto &[key, texture] : textures_)
    {
        ui_->removeImage(texture.image);
        device_->destroyTexture(texture.tex);
    }
    textures_.clear();
    ui_->removeImage(plasma_.image);
    device_->destroyTexture(plasma_.tex);
    plasma_ = {};
}

// the icon tags neon_app_init_load 0x76d60 registers: buttons, character portraits and the logo. Returns the tag length.
size_t HudPainter::iconTag(const std::u16string &text, size_t at, std::string &key, IconBox &box) const
{
    if (text[at] != u'[')
    {
        return 0;
    }
    float size = 0.f;
    size_t tagLength = buttonTag(text, at, key, size);
    if (tagLength == 0)
    {
        tagLength = characterTag(text, at, key, size);
    }
    if (tagLength > 0)
    {
        box = IconBox{size, size, false};
        return tagLength;
    }
    return logoTag(text, at, key, box);
}

// "[QRLOGO]" (0x77272): data\qrlogo.dds at its own size (256 x 128), not scaled with the text
size_t HudPainter::logoTag(const std::u16string &text, size_t at, std::string &key, IconBox &box) const
{
    static const std::u16string kTag = u"[QRLOGO]";
    if (text.compare(at, kTag.size(), kTag) != 0)
    {
        return 0;
    }
    key = "qrlogo";
    const Found<const HudTexture> logo = texture(key);
    box = IconBox{logo ? float(logo->w) : 0.f, logo ? float(logo->h) : 0.f, true};
    return kTag.size();
}

float HudPainter::tallestIcon(const std::u16string &text, float sy) const
{
    float tallest = 0.f;
    for (size_t i = 0; i < text.size(); ++i)
    {
        std::string key;
        IconBox box;
        if (size_t tagLength = iconTag(text, i, key, box))
        {
            tallest = std::max(tallest, box.drawnHeight(sy));
            i += tagLength - 1;
        }
    }
    return tallest;
}

// "[BUTA]" -> icons\a.dds 32 x 32, B / X / Y / L (left) / R (right) / U (up) / D (down), "...FIX" the same icons,
// "[BUTBSMALL]" b.dds 16 x 16
size_t HudPainter::buttonTag(const std::u16string &text, size_t at, std::string &key, float &size)
{
    if (text.compare(at, 4, u"[BUT") != 0)
    {
        return 0;
    }
    size_t close = text.find(u']', at);
    if (close == std::u16string::npos || close - at > 12)
    {
        return 0;
    }
    std::string name;
    for (size_t j = at + 4; j < close; ++j)
    {
        name += char(text[j]);
    }
    if (name.empty())
    {
        return 0;
    }
    size = name == "BSMALL" ? 16.f : 32.f;
    key = std::string("BUT") + name[0];
    return close - at + 1;
}

// "[CHAR<id>]" 32 x 32 and "[FCHAR<id>]" 64 x 64: the character's portrait, the "Char Icons" textures (0x793a0
// loads slot id from data\hud\tex<0x1b93f8[id]>.dds, the position list's pictures)
size_t HudPainter::characterTag(const std::u16string &text, size_t at, std::string &key, float &size)
{
    const bool full = text.compare(at, 6, u"[FCHAR") == 0;
    if (!full && text.compare(at, 5, u"[CHAR") != 0)
    {
        return 0;
    }
    const size_t digits = at + (full ? 6 : 5), e = text.find(u']', digits);
    if (e == std::u16string::npos || e == digits || e - digits > 2)
    {
        return 0;
    }
    int id = 0;
    for (size_t j = digits; j < e; ++j)
    {
        if (text[j] < u'0' || text[j] > u'9')
        {
            return 0;
        }
        id = id * 10 + int(text[j] - u'0');
    }
    char name[16];
    std::snprintf(name, sizeof name, "pilot%02d", menu::kPilotPortrait[id & 15]);
    key = name;
    size = full ? 64.f : 32.f;
    return e - at + 1;
}

float HudPainter::textWidth(const std::u16string &text, int font, float sx) const
{
    float w = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        std::string bk;
        IconBox box;
        if (size_t tagLength = iconTag(text, i, bk, box))
        {
            w += box.drawnWidth(sx);
            i += tagLength - 1;
            continue;
        }
        if (text[i] == u'[' && i + 4 < text.size() && text[i + 1] == u'F' && text[i + 2] == u':' && text[i + 4] == u']')
        {
            font = std::clamp(int(text[i + 3] - u'0'), 0, 1);
            i += 4;
            continue;
        }
        const HudFont &hudFont = fonts_[font];
        if (!hudFont.ok)
        {
            continue;
        }
        w += hudFont.glyphs[hudFont.glyphFor(text[i])].advance * sx;
    }
    return w;
}

void HudPainter::text(ImDrawList *drawList, float pixelsPerUnit, ImVec2 at, const std::u16string &text, const TextStyle &style)
{
    float x = at.x, y = at.y;
    int font = style.font;
    const int flags = style.flags;
    const uint32_t colour = style.colour;
    const float sx = style.scaleX, sy = style.scaleY;
    float w = textWidth(text, font, sx);
    if ((flags & 0xf) == 1)
    {
        x -= w;
    }
    else if ((flags & 0xf) == 2)
    {
        x -= w * 0.5f;
    }
    float pen = x;
    // font_draw_text 0xb1660, flags & 0xf000: 0x1000 centres each glyph's font height in the line's tallest font,
    // 0x2000 aligns them to its bottom (0 = top)
    float lineH = 0;
    {
        int f0 = font;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == u'[' && i + 4 < text.size() && text[i + 1] == u'F' && text[i + 2] == u':' && text[i + 4] == u']')
            {
                f0 = std::clamp(int(text[i + 3] - u'0'), 0, 1);
                i += 4;
                continue;
            }
            if (fonts_[f0].ok)
            {
                lineH = std::max(lineH, fonts_[f0].line);
            }
        }
    }
    y -= textBlockLift(flags, lineH * sy);
    int valign = flags & 0xf000;
    for (size_t i = 0; i < text.size(); ++i)
    {
        std::string bk;
        IconBox box;
        if (size_t tagLength = iconTag(text, i, bk, box))
        {   // a square icon centred on the line, an own-size one from the line's top (neither checked against the game)
            auto bt = textures_.find(bk);
            const float iconW = box.drawnWidth(sx), iconH = box.drawnHeight(sy);
            const float lh = fonts_[font].ok ? fonts_[font].line * sy : iconH;
            const float yo = box.ownSize ? 0.f : (lh - iconH) * 0.5f;
            if (bt != textures_.end())
            {
                drawList->AddImage(bt->second.image,
                             ImVec2(pen * pixelsPerUnit, (y + yo) * pixelsPerUnit),
                             ImVec2((pen + iconW) * pixelsPerUnit, (y + yo + iconH) * pixelsPerUnit),
                             ImVec2(0, 0),
                             ImVec2(1, 1),
                             imColour((colour & 0xff000000u) | 0xffffffu));
            }
            pen += iconW;
            i += tagLength - 1;
            continue;
        }
        if (text[i] == u'[' && i + 4 < text.size() && text[i + 1] == u'F' && text[i + 2] == u':' && text[i + 4] == u']')
        {
            font = std::clamp(int(text[i + 3] - u'0'), 0, 1);
            i += 4;
            continue;
        }
        const HudFont &hudFont = fonts_[font];
        float yo = valign == 0x1000 ? (lineH - hudFont.line) * 0.5f : valign == 0x2000 ? lineH - hudFont.line : 0.f;
        if (!hudFont.ok)
        {
            continue;
        }
        auto it = textures_.find(fontTextureKey(font));
        const argon::hud::FontGlyph &glyph = hudFont.glyphs[hudFont.glyphFor(text[i])];
        // the box placed by hud::layoutFromMap / rasterizeFont: the game's centred on the line (its 2 px of padding
        // above and below, 23 vs 19, 39 vs 35: the ink centred in the 20-high shield / turbo panels) and on the advance
        // (xemu: FE CHARSELECT's "?" centred on its tile)
        const float xo = glyph.left * sx, gw = glyph.width * sx, gh = glyph.height * sy;
        yo = (yo + glyph.top) * sy;
        if (text[i] != u' ' && gw > 0 && it != textures_.end())
        {
            drawList->AddImage(it->second.image,
                         ImVec2((pen + xo) * pixelsPerUnit, (y + yo) * pixelsPerUnit),
                         ImVec2((pen + xo + gw) * pixelsPerUnit, (y + yo + gh) * pixelsPerUnit),
                         ImVec2(glyph.u0, glyph.v0),
                         ImVec2(glyph.u1, glyph.v1),
                         imColour(colour));
        }
        pen += glyph.advance * sx;
    }
}

// data\hud\feplasma0..2.dds (64 x 64, same format) as one 3-layer array for the panel shader (0xc1600 loads them)
void HudPainter::loadPlasma()
{
    if (plasma_.image)
    {
        return;
    }
    argon::TextureData all;
    for (int i = 0; i < 3; ++i)
    {
        const argon::TextureData td = argon::loadOptionalDds(game_ + "/data/hud/feplasma" + std::to_string(i) + ".dds");
        if (td.levels.empty())
        {
            return;
        }
        if (i == 0)
        {
            all = td;
            all.layers = 1;
            all.layerStride = td.bytes.size();
        }
        else
        {
            if (td.format != all.format || td.bytes.size() != all.layerStride)
            {
                return;
            }
            all.bytes.insert(all.bytes.end(), td.bytes.begin(), td.bytes.end());
            all.layers++;
        }
    }
    plasma_.tex = device_->createTexture(all);
    plasma_.w = int(all.width);
    plasma_.h = int(all.height);
    if (plasma_.tex.valid)
    {
        plasma_.image = ui_->addLayeredImage(plasma_.tex, argon::gfx::SamplerMode::Wrap);
    }
}

// 0xc1b40 in the 640 x 480 HUD space. Hexagon with the top-left and bottom-right corners cut by c = 8 (less if the
// panel is narrower than 8): v0 (x, y+c) v1 (x+c, y) v2 (x+w, y) v3 (x+w, y+h-c) v4 (x+w-c, y+h) v5 (x, y+h),
// triangles (0,1,2) (0,2,3) (0,3,5) (5,3,4). Passes, in the game's order:
//  1. fill (if `fill`): untextured, v0..v2 = fillA (the colour argument), v3..v5 = fillB (the caller's EAX)
//  2. glow (if `glow`): ring 6 out, inner colour = `line` with RGB / 2 (highlight) or / 8, outer black, additive
//  3. overlay: plasma (hudpanel.frag), v0 / v1 0xffffff, v3 / v4 0xff8000, rest 0x8080ff, alpha 0x70 (0xb0 if highlight)
//  4. outline: 1 px lines in `line` if highlight, else black
void HudPainter::panel(ImDrawList *drawList, float pixelsPerUnit, const PanelBox &box, const PanelStyle &style)
{
    const float x = box.x, y = box.y, w = box.w, h = box.h;
    const bool highlight = style.highlight, glow = style.glow, fill = style.fill, overlay = style.overlay;
    const uint32_t line = style.line, fillA = style.fillTop, fillB = style.fillBottom;
    const float scale = style.plasma;
    if (w <= 0.f || h <= 0.f)
    {
        return;
    }
    float lc = w < 8.f ? 8.f - w : 0.f;
    float corner = 8.f;
    glm::vec2 v[6] = {{x, y + corner}, {x + corner - lc, y + lc}, {x + w, y + lc}, {x + w, y + h - corner}, {x + w - corner + lc, y + h - lc}, {x, y + h}};
    uint32_t alpha = highlight ? 0xb0000000u : 0x70000000u;
    uint32_t over[6] = {0xffffff, 0xffffff, 0x8080ff, 0xff8000, 0xff8000, 0x8080ff};
    for (auto &colour : over)
    {
        colour = (colour & 0xffffff) | alpha;
    }
    ImVec2 white = drawList->_Data ? ImGui::GetFontTexUvWhitePixel() : ImVec2(0, 0);
    if (fill)
    {   // 0xc1fb0: v0..v2 = the fill colour argument (EBX), v3..v5 = the caller's EAX
        uint32_t fc[6] = {fillA, fillA, fillA, fillB, fillB, fillB};
        addPanelFan(drawList, PanelFan{v, fc, pixelsPerUnit, scale, false, white});
    }
    if (glow)
    {
        const uint32_t in = dimmedColour(line, highlight ? 1 : 3);
        glm::vec2 outline[6] = {{v[0].x - 6.f, v[0].y},
                          {v[1].x, v[1].y - 6.f},
                          {v[2].x + 6.f, v[2].y - 6.f},
                          {v[3].x + 6.f, v[3].y},
                          {v[4].x, v[4].y + 6.f},
                          {v[5].x - 6.f, v[5].y + 6.f}};
        drawList->AddCallback(additiveCallback, this);
        drawList->PrimReserve(36, 36);
        for (int i = 0; i < 6; ++i)
        {
            int next = (i + 1) % 6;
            const ImVec2 a0 = scaledPoint(v[i], pixelsPerUnit), a1 = scaledPoint(v[next], pixelsPerUnit), b0 = scaledPoint(outline[i], pixelsPerUnit), b1 = scaledPoint(outline[next], pixelsPerUnit);
            drawList->PrimVtx(a0, white, imColour(in));
            drawList->PrimVtx(b0, white, imColour(0xff000000u));
            drawList->PrimVtx(a1, white, imColour(in));
            drawList->PrimVtx(a1, white, imColour(in));
            drawList->PrimVtx(b0, white, imColour(0xff000000u));
            drawList->PrimVtx(b1, white, imColour(0xff000000u));
        }
        drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }
    loadPlasma();
    if (plasma_.image)
    {
        drawList->AddCallback(panelCallback, this);
        drawList->PushTextureID(plasma_.image);
        if (overlay)
        {
            addPanelFan(drawList, PanelFan{v, over, pixelsPerUnit, scale, true, white});
        }
        drawList->PopTextureID();
        drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }
    uint32_t lc2 = highlight ? line : 0xff000000u;
    for (int i = 0; i < 6; ++i)
    {
        drawList->AddLine(scaledPoint(v[i], pixelsPerUnit), scaledPoint(v[(i + 1) % 6], pixelsPerUnit), imColour(lc2), pixelsPerUnit);
    }
}
