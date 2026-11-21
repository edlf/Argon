// Argon: a menu page's vertical list of rows (the viewer's; the game's lists have fixed row counts): the rows keep
// the page's own size and step; when they do not all fit above the page's limit only a window of them is shown, and
// that window jumps a row when the cursor passes its edge (as the game's record pages). docs\argon\menu-list-plan.md;
// tests\menu_tests.cpp.
#pragma once
#include <algorithm>
#include <cmath>

namespace menu
{
// a scroll mark (MenuKit::drawScrollMarks): 3 units off the rows, the arrow 6 high
inline constexpr float kScrollMarkGap = 3.f, kScrollMarkHeight = 6.f;
// a left-aligned list's mark: centred this far right of its row's text x
inline constexpr float kScrollMarkIndent = 10.f;

// a list's row: its font, the text's scale (the row is the font's line at that scale high) and the distance from one
// row to the next
struct RowStyle
{
    int font;
    float scale, step;

    constexpr RowStyle spacedBy(float rowStep) const
    {
        return {font, scale, rowStep};
    }
};
// the top-level menus' rows (main menu, Options, pause, the multiplayer end): font 1 at 0.75, FE OPTIONS' step
inline constexpr RowStyle kLargeRows{1, 0.75f, 30.f};
// the lists' and settings' rows: font 0 at 1, 24 apart (the game's 20, with a little room between the panels)
inline constexpr RowStyle kSmallRows{0, 1.f, 24.f};

// where a page puts its rows in the 640 x 480 frame: the first row's top, the distance from one row to the next, each
// row's height, and the y the rows stay above (the ticker panel, the hints)
struct ListLayout
{
    float top, step, rowHeight, limit;

    float rowY(int slot) const
    {
        return top + step * float(slot);
    }
    float rowBottom(int slot) const
    {
        return rowY(slot) + rowHeight;
    }
};

// as many rows as end above the limit (at least one); the scroll mark below them may reach past it
inline int rowsThatFit(const ListLayout &layout, int rowCount)
{
    if (rowCount <= 0)
    {
        return 0;
    }
    const float room = layout.limit - layout.rowBottom(0);
    return std::clamp(int(std::floor(room / layout.step)) + 1, 1, rowCount);
}

// the rows shown of a list: [first, end)
class ScrollWindow
{
  public:
    // the window over rowCount rows, rowsShown at a time, moved the least that keeps `row` (the cursor's) inside it
    void follow(int row, int rowCount, int rowsShown)
    {
        count = std::max(rowCount, 0);
        shown = std::clamp(rowsShown, 1, std::max(count, 1));
        firstShown = std::clamp(firstShown, row - (shown - 1), row);
        firstShown = std::clamp(firstShown, 0, std::max(0, count - shown));
    }
    int first() const
    {
        return firstShown;
    }
    int end() const
    {
        return std::min(count, firstShown + shown);
    }
    bool moreAbove() const
    {
        return firstShown > 0;
    }
    bool moreBelow() const
    {
        return end() < count;
    }

  private:
    int count = 0, shown = 1, firstShown = 0;
};
}  // namespace menu
