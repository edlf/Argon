// Argon: the pieces the menu pages share (front end and the race's menus): the selected row's pulse, the row panel
// with its text (text element draw 0xad620), the lists' layout and cursor, the bottom hints, the scroll arrows and
// marks, the gradient frame, the pilot portraits and word wrapping. The arithmetic is in ui\menu_common.hpp (tested by
// tests\menu_tests.cpp); this binds it to the clock, the painter's fonts and the shell's ticker and slide. The shell
// owns one (FeShell::menus).
#pragma once
#include "ui/fe_canvas.hpp"
#include "ui/menu_common.hpp"
#include "ui/menu_list.hpp"

#include <span>
#include <string>
#include <vector>

class FeShell;
class GameStrings;
class HudPainter;

class MenuKit
{
  public:
    MenuKit(const HudPainter &painter, const GameStrings &strings, FeShell &shell);

    static float menuBlink();   // 0xad620: the selected text's pulse, 0.5..1
    float fontLine(int font) const;   // the font's line height (20 / 35 before it loads)

    // a row: its panel (highlighted and filled while selected) around the text, the text blinking while selected.
    // x is the text's x as HudPainter::text's align (0 left, 1 right, 2 centre); h the panel height; idlePanel false: no
    // panel while not selected (the choices drawn as plain text). Returns the text's y.
    float drawMenuRow(const FeCanvas &canvas, ImVec2 at, const std::u16string &text, const MenuRowLook &look) const;
    // a row in a row style (ui\menu_list.hpp); its height, and a list's layout in it from top, kept above limit
    MenuRowLook listRowLook(const menu::RowStyle &style, bool selected, uint32_t colour) const;
    float rowHeight(const menu::RowStyle &style) const;
    menu::ListLayout listLayout(const menu::RowStyle &style, float top, float limit) const;

    // up / down through n rows, wrapping (up = the previous row; in a bottom-up list, menu::ListOrder, the next).
    // stepCursor returns whether up or down was pressed; menuCursor also restarts the ticker then
    static bool stepCursor(int &sel, int count, const menu::MenuInput &in, menu::ListOrder order = menu::ListOrder::TopDown);
    bool menuCursor(int &sel, int n, const menu::MenuInput &in, menu::ListOrder order = menu::ListOrder::TopDown) const;

    // the bottom hints: left at x 62 (left-aligned), right at 568 (right-aligned), font 0; an empty one is skipped. In the
    // front end they slide with the shell's title (0x316e0 states 1 / 4)
    void drawHints(const FeCanvas &canvas, const Hints &hints) const;
    // a list's scroll arrow, icons\up.dds ("BUTU") or icons\down.dds ("BUTD"), 40 x 20 at (x, y) in the 640 frame (ox included)
    void drawScrollArrow(const FeCanvas &canvas, const char *icon, float x, float y) const;
    // the viewer's scroll marks of a list that does not fit (ui\menu_list.hpp): a small outlined arrow above the first
    // row shown while rows are hidden above, one below the last row shown while rows are hidden below, each centred on
    // its x (the rows' own, so the marks slide in and out with them)
    struct ScrollMarkXs
    {
        float above, below;
    };
    static void drawScrollMarks(const FeCanvas &canvas, const ScrollMarkXs &xs, const menu::ListLayout &layout, const menu::ScrollWindow &window);
    // the gradient frame 0xc2310 (the record pages, Track Data, Edit Playlist, Select Ghost): blue on top, red at the
    // bottom right (FrameShade::Red: the other way round), a black outline; the box in the 640 x 480 frame, ox included
    static void drawGradientFrame(const FeCanvas &canvas, const menu::Box &box, FrameShade shade = FrameShade::Blue);
    // the ghost pages' top frame (52, 100) 536 x 40: the track's name (infotext 300 + track), font 1 at 0.6
    void drawGhostTrackName(const FeCanvas &canvas, int track) const;
    // data/hud pilotNN in the box (640 frame, ox included)
    void drawPilotPortrait(const FeCanvas &canvas, int pilot, const menu::Box &box) const;

    float wrapWidth() const;   // 516, widened from margin to margin
    // the words of s in lines at most width wide (font, scale); newlines break
    std::vector<std::u16string> wrapText(const std::u16string &s, int font, float scale, float width) const;

  private:
    float hintOutset(const std::u16string &hint, float s) const;   // the front end's hint slide (ui\shell_slide.hpp)

    const HudPainter &painter_;
    const GameStrings &strings_;
    FeShell &shell_;
};
