// Argon: the 2D layer the HUD (hud\) and the menus (ui\) draw with, in the game's 640 x 480 frame scaled by k: the HUD
// textures registered with ImGui, the two fonts (the game's, or drawn again from a replacement typeface:
// hud\hud_font.hpp), the text renderer (font_draw_text 0xb1660, its icon tags), the panels (0xc1b40) and their plasma,
// and ImGui draw callbacks for the HUD's additive / subtractive / panel blends. The menus' rows and hints on top of it:
// ui\menukit.cpp.
#pragma once
#include "files/dds.hpp"
#include "gfx/device.hpp"
#include "gfx/texture.hpp"
#include "gfx/ui_renderer.hpp"
#include "hud/hud_font.hpp"
#include "util/found.hpp"

#include <imgui.h>

#include <cstdint>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

// a texture as ImGui draws it
struct HudTexture
{
    argon::gfx::Texture tex;
    ImTextureID image{};   // UiRenderer::addImage's (the plasma: addLayeredImage's)
    int w = 0, h = 0;
};

struct HudFont : argon::hud::FontLayout
{
    bool ok = false;
};

// a panel's place in the 640 x 480 frame: its top left, width and height
struct PanelBox
{
    float x, y, w, h;
};

// HudPainter::text's look: the font (0 small, 1 big), the colour (ARGB), the flags (0 left, 1 right, 2 centre; and the
// painter's other bits) and the scale along x and y
struct TextStyle
{
    int font = 0;
    uint32_t colour = 0xff85dde5u;   // hud_text_ctor's default
    int flags = 0;
    float scaleX = 1.f, scaleY = 1.f;
};

// HudPainter::panel's look (0xc1b40): the outline in `line` (highlight), the glow ring, the fill (fillTop on v0..v2,
// fillBottom on v3..v5), the plasma's uv scale and the overlay pass
struct PanelStyle
{
    bool highlight = false, glow = false;
    uint32_t line = 0xffffff00u;
    bool fill = false;
    uint32_t fillTop = 0, fillBottom = 0;
    float plasma = 1.f;
    bool overlay = true;
};

// an 0xAARRGGBB colour as ImGui packs it
inline ImU32 imColour(uint32_t argb)
{
    return IM_COL32((argb >> 16) & 255, (argb >> 8) & 255, argb & 255, argb >> 24);
}

class HudPainter
{
  public:
    static constexpr int kFonts = 2;

    void attach(argon::gfx::Device &device, argon::gfx::UiRenderer &ui, const std::string &gameFolder);

    // ---- textures (data\hud and the front end's), by key
    bool loadTexture(const std::string &key, const std::string &path);
    bool loadTiledTexture(const std::string &key, const std::string &path);   // sampled with wrap (scrolled uv)
    Found<const HudTexture> texture(const std::string &key) const;   // none before it loads
    void freeAll();

    // ---- fonts 0 and 1: Square721 BdEx BT 18 / 34, or for Japanese MSPGoth in both, each drawn again from its
    // replacement typeface when the first of `replacementFolders` that has it does
    void loadFonts(bool japanese, const std::vector<std::filesystem::path> &replacementFolders);
    bool japaneseFonts() const;
    const HudFont &font(int slot) const;

    // ---- text (0xb1660): flags & 0xf: 0 left, 1 right, 2 centre; & 0xf0: 0x10 centred on y, 0x20 its bottom on y;
    // & 0xf000: each glyph centred (0x1000) or bottom-aligned (0x2000) in the line's tallest font. "[F:n]" switches the
    // font inside the text; "[BUTx]", "[CHARn]", "[FCHARn]", "[QRLOGO]" are pictures
    float textWidth(const std::u16string &text, int font, float sx = 1.f) const;
    float tallestIcon(const std::u16string &s, float sy) const;   // the drawn height of a text's tallest icon, 0 without
    void text(ImDrawList *drawList, float pixelsPerUnit, ImVec2 at, const std::u16string &text, const TextStyle &style);

    // ---- the game's HUD panel 0xc1b40: x, y, w, h; highlight (outline in `line` colour, alpha 0xb0), glow ring, fill
    // (fillA on v0..v2, fillB on v3..v5); scale: the plasma's uv
    void panel(ImDrawList *drawList, float pixelsPerUnit, const PanelBox &box, const PanelStyle &style);

    // ---- what the next primitives are blended with (until ImDrawCallback_ResetRenderState)
    void blend(ImDrawList *drawList, argon::gfx::UiBlend mode);

  private:
    // an icon tag's picture: square ones scale with the text; ownSize ones (the tag table's +0x44, 0x39bda4) keep their
    // texture's size whatever the text's scale (0xb1ce7)
    struct IconBox
    {
        float width = 0.f, height = 0.f;
        bool ownSize = false;
        float drawnWidth(float sx) const
        {
            return ownSize ? width : width * sx;
        }
        float drawnHeight(float sy) const
        {
            return ownSize ? height : height * sy;
        }
    };

    bool loadTextureSampled(const std::string &key, const std::string &path, argon::gfx::SamplerMode sampling);
    void addTexture(const std::string &key, const argon::TextureData &picture, argon::gfx::SamplerMode sampling);
    bool loadMapFont(int slot, const std::string &name);
    bool loadAbcFont(int slot, const std::string &name);
    void loadReplacementFont(int font, const std::vector<uint8_t> &fontFile);
    void loadPlasma();
    size_t iconTag(const std::u16string &text, size_t at, std::string &key, IconBox &box) const;
    size_t logoTag(const std::u16string &s, size_t i, std::string &key, IconBox &box) const;
    static size_t buttonTag(const std::u16string &text, size_t at, std::string &key, float &size);
    static size_t characterTag(const std::u16string &text, size_t at, std::string &key, float &size);
    static void additiveCallback(const ImDrawList *, const ImDrawCmd *cmd);
    static void subtractiveCallback(const ImDrawList *, const ImDrawCmd *cmd);
    static void panelCallback(const ImDrawList *, const ImDrawCmd *cmd);

    argon::gfx::Device *device_ = nullptr;
    argon::gfx::UiRenderer *ui_ = nullptr;
    std::string game_;
    std::map<std::string, HudTexture> textures_;
    HudFont fonts_[kFonts];
    bool japaneseFonts_ = false;
    HudTexture plasma_;   // feplasma0..2 as a 3-layer array (the panels' overlay)
};
