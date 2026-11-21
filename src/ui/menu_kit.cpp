// Argon: the pieces the menu pages share (ui\menu_kit.hpp). Constants: ui\menu_common.hpp.
#include "ui/menu_kit.hpp"

#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"

#include <cstdio>

using namespace menu;

MenuKit::MenuKit(const HudPainter &painter, const GameStrings &strings, FeShell &shell) : painter_(painter), strings_(strings), shell_(shell)
{
}

float MenuKit::menuBlink()
{
    return blinkFactor(ImGui::GetTime());
}

float MenuKit::fontLine(int font) const
{
    const HudFont &hudFont = painter_.font(font);
    return hudFont.ok ? hudFont.line : font == 1 ? 35.f : 20.f;
}

float MenuKit::drawMenuRow(const FeCanvas &canvas, ImVec2 at, const std::u16string &text, const MenuRowLook &look) const
{
    RowPanel panel = rowPanel(at.x, painter_.textWidth(text, look.font, look.scale), look.height, look.align);
    if (look.selected || look.idlePanel)
    {
        canvas.panel({panel.x, at.y, panel.w, look.height}, {look.selected, true, 0xffffff00u, look.selected, 0xffa08040u, 0xffa08040u});
    }
    const float ty = rowTextY(at.y, look.height, fontLine(look.font), look.scale);
    const uint32_t colour = look.selected ? menu::blinkColour(look.colour, menuBlink()) : look.colour;
    canvas.text({at.x, ty}, text, {look.font, colour, look.align, look.scale, look.scale});
    return ty;
}

MenuRowLook MenuKit::listRowLook(const menu::RowStyle &style, bool selected, uint32_t colour) const
{
    return MenuRowLook{style.font, style.scale, rowHeight(style), selected, colour};
}

float MenuKit::rowHeight(const menu::RowStyle &style) const
{
    return fontLine(style.font) * style.scale;
}

menu::ListLayout MenuKit::listLayout(const menu::RowStyle &style, float top, float limit) const
{
    return {top, style.step, rowHeight(style), limit};
}

bool MenuKit::stepCursor(int &sel, int count, const MenuInput &in, menu::ListOrder order)
{
    return menu::stepCursor(sel, count, in.up, in.down, order);
}

bool MenuKit::menuCursor(int &sel, int count, const MenuInput &in, menu::ListOrder order) const
{
    if (!stepCursor(sel, count, in, order))
    {
        return false;
    }
    shell_.restartTicker();
    return true;
}

// FE GHOSTMANAGE's track name (0x223f0; its element and font are not decoded: font 1 at 0.6 here, centred in the frame's
// height); FE GHOSTSELECT leaves the frame empty, the viewer shows it there too (docs\argon\differences.md)
void MenuKit::drawGhostTrackName(const FeCanvas &canvas, int track) const
{
    const float originX = canvas.originX();
    const float scale = 0.6f * menu::fe::kFe;
    const float y = rowTextY(100.f, 40.f, fontLine(1), scale);
    canvas.text({atLeft(originX, kRowX), y}, strings_.info(300 + track, ""), {1, menu::fe::kText, 0, scale, scale});
}

// the text blocks (biography, track info) widen with a widescreen window
float MenuKit::wrapWidth() const
{
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float pixelsPerUnit = ds.y / 480.f, originX = (ds.x / pixelsPerUnit - 640.f) * 0.5f;
    return spanWidth(originX, 516.f);
}

void MenuKit::drawHints(const FeCanvas &canvas, const Hints &hints) const
{
    const float originX = canvas.originX();
    if (!hints.left.empty())
    {
        canvas.text({atLeft(originX, kRowX) - hintOutset(hints.left, hints.scale), hints.y}, hints.left, {0, kText, 0, hints.scale, hints.scale});
    }
    if (!hints.right.empty())
    {
        canvas.text({atRight(originX, kRight) + hintOutset(hints.right, hints.scale), hints.y}, hints.right, {0, kText, 1, hints.scale, hints.scale});
    }
}

float MenuKit::hintOutset(const std::u16string &hint, float scale) const
{
    if (!shell_.isOpen())
    {
        return 0.f;
    }
    return shell_.hintOutset(painter_.textWidth(hint, 0, scale));
}

void MenuKit::drawScrollArrow(const FeCanvas &canvas, const char *icon, float x, float y) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    if (const Found<const HudTexture> texture = painter_.texture(icon))
    {
        drawList->AddImage(texture->image, ImVec2(x * pixelsPerUnit, y * pixelsPerUnit), ImVec2((x + 40.f) * pixelsPerUnit, (y + 20.f) * pixelsPerUnit));
    }
}

namespace
{
constexpr float kScrollMarkHalfWidth = 6.f;

// a filled triangle in the 640 frame with a 1 unit black outline (as the row panels'): its base at baseY, its tip at
// tipY (above the base: pointing up)
void drawScrollMark(const FeCanvas &canvas, float x, float baseY, float tipY)
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const ImVec2 left((x - kScrollMarkHalfWidth) * pixelsPerUnit, baseY * pixelsPerUnit), right((x + kScrollMarkHalfWidth) * pixelsPerUnit, baseY * pixelsPerUnit), tip(x * pixelsPerUnit, tipY * pixelsPerUnit);
    drawList->AddTriangleFilled(left, right, tip, imColour(menu::kText));
    drawList->AddTriangle(left, right, tip, imColour(0xff000000u), pixelsPerUnit);
}
}  // namespace

void MenuKit::drawScrollMarks(const FeCanvas &canvas, const ScrollMarkXs &xs, const menu::ListLayout &layout, const menu::ScrollWindow &window)
{
    if (window.moreAbove())
    {
        const float base = layout.rowY(0) - kScrollMarkGap;
        drawScrollMark(canvas, xs.above, base, base - kScrollMarkHeight);
    }
    if (window.moreBelow())
    {
        const float base = layout.rowBottom(window.end() - window.first() - 1) + kScrollMarkGap;
        drawScrollMark(canvas, xs.below, base, base + kScrollMarkHeight);
    }
}

void MenuKit::drawGradientFrame(const FeCanvas &canvas, const menu::Box &box, FrameShade shade)
{
    const bool swapped = shade == FrameShade::Red;
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    uint32_t topColour = swapped ? 0xb0702010u : 0xb0102070u, bottomColour = swapped ? 0xb0102070u : 0xb0702010u;
    ImVec2 p0(box.x * pixelsPerUnit, box.y * pixelsPerUnit), p1((box.x + box.w) * pixelsPerUnit, (box.y + box.h) * pixelsPerUnit);
    drawList->AddRectFilledMultiColor(p0, p1, imColour(topColour), imColour(topColour), imColour(bottomColour), imColour(topColour));
    drawList->AddRect(p0, p1, imColour(0xff000000u), 0.f, 0, pixelsPerUnit);
}

// 0x528ea8 + id x 0x50 holds data\hud\tex<0x1b93f8[id]>.dds (0x793a0); the box in the 640 frame, ox included
void MenuKit::drawPilotPortrait(const FeCanvas &canvas, int pilot, const menu::Box &box) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    char name[16];
    std::snprintf(name, sizeof name, "pilot%02d", menu::kPilotPortrait[pilot & 15]);
    if (const Found<const HudTexture> texture = painter_.texture(name))
    {
        drawList->AddImage(texture->image, ImVec2(box.x * pixelsPerUnit, box.y * pixelsPerUnit), ImVec2((box.x + box.w) * pixelsPerUnit, (box.y + box.h) * pixelsPerUnit));
    }
}

namespace
{
// words joined into lines no wider than a width (HudPainter::textWidth at the font and scale)
class LineWrap
{
  public:
    LineWrap(const HudPainter &painter, int font, float scale, float width) : painter_(painter), font_(font), scale_(scale), width_(width)
    {
    }
    void add(char16_t character)
    {
        if (character == u' ')
        {
            flushWord();
        }
        else if (character == u'\n')
        {
            flushWord();
            lines_.push_back(line_);
            line_.clear();
        }
        else
        {
            word_ += character;
        }
    }
    std::vector<std::u16string> finish()
    {
        flushWord();
        if (!line_.empty())
        {
            lines_.push_back(line_);
        }
        return std::move(lines_);
    }

  private:
    void flushWord()
    {
        if (word_.empty())
        {
            return;
        }
        const std::u16string joined = line_.empty() ? word_ : line_ + u" " + word_;
        if (!line_.empty() && painter_.textWidth(joined, font_, scale_) > width_)
        {
            lines_.push_back(line_);
            line_ = word_;
        }
        else
        {
            line_ = joined;
        }
        word_.clear();
    }

    const HudPainter &painter_;
    int font_;
    float scale_, width_;
    std::vector<std::u16string> lines_;
    std::u16string line_, word_;
};
}  // namespace

std::vector<std::u16string> MenuKit::wrapText(const std::u16string &text, int font, float scale, float width) const
{
    LineWrap wrap(painter_, font, scale, width);
    for (char16_t character : text)
    {
        wrap.add(character);
    }
    return wrap.finish();
}
