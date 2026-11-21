// Argon: what the front end's and the race menus' 2D drawing draws on: the ImGui draw list, the game's 640 x 480 frame
// scaled to the window (k = window height / 480) and that frame's x offset in a wider window (ox), with the HUD
// painter's text and panels (hud\hud_painter.hpp) in the styles they are drawn in.
#pragma once
#include "hud/hud_painter.hpp"
#include "ui/menu_common.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>

// a menu row (App::drawMenuRow): its text's font and scale, the panel's height, whether it is the selected row
// (highlighted and filled, its text blinking), the text's colour and align (as TextStyle's flags), and whether it has a
// panel while not selected (the choices drawn as plain text)
struct MenuRowLook
{
    int font = 0;
    float scale = 1.f, height = 0.f;
    bool selected = false;
    uint32_t colour = menu::kText;
    int align = 0;
    bool idlePanel = true;

    MenuRowLook alignedTo(int textAlign) const
    {
        MenuRowLook look = *this;
        look.align = textAlign;
        return look;
    }
};

// the bottom hints (App::drawHints): left at x 62 (left-aligned), right at 568 (right-aligned), font 0; an empty one is
// skipped
struct Hints
{
    std::u16string left, right;
    float scale = menu::kHintScale, y = menu::kHintY;
};

// MenuKit::drawGradientFrame's colours: blue (0x102070) with a red (0x702010) bottom-right corner, or the other way round
enum class FrameShade
{
    Blue,
    Red
};

class FeCanvas
{
  public:
    FeCanvas(HudPainter &painter, ImDrawList *drawList, float scale, float originX) : painter_(painter), drawList_(drawList), scale_(scale), originX_(originX)
    {
    }

    ImDrawList *drawList() const
    {
        return drawList_;
    }
    float scale() const
    {
        return scale_;
    }
    float originX() const
    {
        return originX_;
    }
    HudPainter &painter() const
    {
        return painter_;
    }

    // at: in the 640 x 480 frame, the x offset already added
    void text(ImVec2 at, const std::u16string &content, const TextStyle &style) const
    {
        painter_.text(drawList_, scale_, at, content, style);
    }
    void panel(const PanelBox &box, const PanelStyle &style) const
    {
        painter_.panel(drawList_, scale_, box, style);
    }

  private:
    HudPainter &painter_;
    ImDrawList *drawList_;
    float scale_, originX_;
};
