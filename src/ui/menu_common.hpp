// Argon: constants shared by the menu pages (front end ui\menu.cpp ... and the race's menus
// ui\pause.cpp / app_results.cpp / app_score.cpp / ui\upgrade.cpp); docs\frontend\README.md.
// A front-end page does `using namespace menu::fe;`, a race menu `using namespace menu::race;`: both bring the
// colours and layout below, and kFe, the page's text factor.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace menu
{
// the buttons pressed this frame (PlayerInput::readMenu: any pad or the keyboard)
struct MenuInput
{
    bool up = false, down = false, left = false, right = false, a = false, b = false, x = false, y = false;
};

// each character's portrait, data\hud pilotNN (the loading screen's, the credits' pictures)
inline constexpr int kPilotPortrait[16] = {12, 14, 7, 2, 4, 11, 9, 6, 1, 13, 0, 15, 5, 3, 10, 8};

// a rectangle in the 640 x 480 frame
struct Box
{
    float x, y, w, h;
};

inline constexpr uint32_t kText = 0xff85dde5u;     // hud_text_ctor default colour
inline constexpr uint32_t kGrey = 0xff555555u;     // locked / unavailable rows, the options not chosen
inline constexpr uint32_t kOrange = 0xffffbe64u;   // -0x419c: the info values
inline constexpr uint32_t kYellow = 0xffffff00u;
inline constexpr uint32_t kHead = 0xffff8000u;     // the result pages' headings

inline constexpr float kRowX = 62.f, kRowFrom = 640.f;   // 52 + 10 (4:3 safe area); rows slide in from x 640 over 10 frames
inline constexpr float kRight = 568.f;                   // right-aligned texts: 630 - 52 - 10
inline constexpr float kHintY = 397.f, kHintScale = 1.f;   // the "[BUTB] - Back" / "Select - [BUTA]" hints (xemu)

// the safe margin (g_ui_safe_margin 52); with the widescreen flag 0.75 x 52 - 80 = -41 in the centred 640 frame
// (docs\frontend\README.md "Widescreen layout"). The layouts above are written for 52: safeShift moves them, for a window
// whose 640 frame starts at ox: 0 at 4:3, the game's widescreen margin (-93) at 16:9 and wider, in between the
// margin stays 52 from the window's edge. With the widescreen flag off (g_widescreen 0x298ecc, the viewer's Game
// Settings / settings.json) the 4:3 margin stays in the centred frame whatever the window
inline constexpr float kSafeMargin = 52.f, kSafeMarginWide = 0.75f * 52.f - 80.f;
inline bool widescreenLayout = true;   // App::storeArgonSettings keeps it
inline float safeShift(float originX)
{
    return widescreenLayout ? std::max(kSafeMarginWide, kSafeMargin - originX) - kSafeMargin : 0.f;
}
inline float atLeft(float originX, float x)
{
    return originX + safeShift(originX) + x;
}     // a position measured from the left margin
inline float atRight(float originX, float x)
{
    return originX - safeShift(originX) + x;
}    // ... from the right margin
inline float spanWidth(float originX, float w)
{
    return w - 2.f * safeShift(originX);
} // a width from margin to margin
// a box (x, w) of the 4:3 layout: from margin to margin it widens, else it follows the nearer margin (centred stays)
inline bool spansMargins(float x, float w)
{
    return x <= kSafeMargin + 0.5f && x + w >= 640.f - kSafeMargin - 0.5f;
}
inline float boxLeft(float originX, float x, float w)
{
    float centre = x + w * 0.5f;
    if (spansMargins(x, w) || centre < 300.f)
    {
        return atLeft(originX, x);
    }
    return centre > 340.f ? atRight(originX, x) : originX + x;
}
inline float boxWidth(float originX, float x, float w)
{
    return spansMargins(x, w) ? spanWidth(originX, w) : w;
}

// the front end's pages draw their texts at the elements' scale (xemu, widescreen on and off: an earlier 0.75 here
// made them 1 / 0.75 small)
namespace fe
{
using namespace menu;
inline constexpr float kFe = 1.f;
} // namespace fe
// the race's menus (PAUSEMENU, UPGRADEMENU, the result pages: game-logic hud groups) at the elements' own scale
// (compared with the game: 1.33 x the front-end size, checked against xemu)
namespace race
{
using namespace menu;
inline constexpr float kFe = 1.f;
} // namespace race

// ---- the menu helpers' arithmetic (App's wrappers in ui\menukit.cpp; tests\menu_tests.cpp)

// 0xad620: the selected text's RGB factor at `seconds`: sin((vblanks % 360) x 10 deg) x 0.25 + 0.75, vblanks at 60 a
// second (one cycle per 36 vblanks = 0.6 s), so 0.5..1
inline float blinkFactor(double seconds)
{
    int vb = int(seconds * 60.0) % 360;
    return std::sin(float(vb) * 10.f * 3.14159265f / 180.f) * 0.25f + 0.75f;
}

// RGB x blink, each channel clamped to 0..255; alpha kept
inline uint32_t blinkChannel(uint32_t colour, float blink, int shift)
{
    return uint32_t(std::clamp(int(float((colour >> shift) & 255u) * blink), 0, 255)) << shift;
}

inline uint32_t blinkColour(uint32_t colour, float blink)
{
    return (colour & 0xff000000u) | blinkChannel(colour, blink, 16) | blinkChannel(colour, blink, 8) | blinkChannel(colour, blink, 0);
}

// how a list is drawn: its first row at the top, or at the bottom (up then moves to the next row)
enum class ListOrder
{
    TopDown,
    BottomUp
};

// up / down through n rows, wrapping: up = the previous row (bottom-up: the next). Returns whether up or down was
// pressed; n <= 0 leaves sel alone
inline bool stepCursor(int &sel, int count, bool up, bool down, ListOrder order = ListOrder::TopDown)
{
    if (count <= 0)
    {
        return false;
    }
    int prev = count - 1, next = 1;
    if (order == ListOrder::BottomUp)
    {
        std::swap(prev, next);
    }
    if (up)
    {
        sel = (sel + prev) % count;
    }
    if (down)
    {
        sel = (sel + next) % count;
    }
    return up || down;
}

// one 60 Hz step of rows sliding from x `from` to `to`: (to - from) / 10 a step, stopping at `to`
inline void slideRows(std::span<float> xs, float from, float to)
{
    float step = (to - from) / 10.f;
    for (float &x : xs)
    {
        x = step < 0 ? std::max(to, x + step) : std::min(to, x + step);
    }
}

// a flashing grey (FE MESSAGEBOX's controller message 0x2ad70, FE FASTESTLAP's new records 0x206c0): 16 darker a
// frame, back to 255 below 64
constexpr uint8_t stepFlash(uint8_t grey)
{
    return grey < 64 + 16 ? 255 : uint8_t(grey - 16);
}

// the model fade's alpha (shell draw 0x30303 -> 0xc2810, shell +0xa238): 1 - clamp(seconds since the model showed,
// 0, 1), none without a fade (sinceShown < 0); a locked character's black silhouette (+0xa23c, 0x304fb) keeps at
// least 0.2 (0x1a358c)
inline float modelFade(double sinceShown, bool lockedCharacter)
{
    const float fade = sinceShown < 0.0 ? 0.f : 1.f - std::clamp(float(sinceShown), 0.f, 1.f);
    return lockedCharacter ? std::max(fade, 0.2f) : fade;
}

// the description ticker (0x2ffa0): 2 px a frame at the game's 60 Hz = 120 px a second, moved by the frame time
// (smooth at any refresh rate, the game's speed); past -(width + 19) it starts again at x 640
inline constexpr float kTickerSpeed = 2.f * 60.f;
inline constexpr float kTickerX = 62.f, kTickerY = 370.f, kTickerWidth = 506.f, kTickerHeight = 20.f;
// the width scrollTicker wraps at: the text's, plus the widening of the panel from margin to margin, so in a widescreen
// window (text drawn from the right margin) it still runs fully past the panel's left end (docs\argon\differences.md)
inline float tickerWrapWidth(float textWidth, float originX)
{
    return textWidth + spanWidth(originX, kTickerWidth) - kTickerWidth;
}
inline float scrollTicker(float x, float dt, float width)
{
    x -= kTickerSpeed * dt;
    return x < -(width + 19.f) ? kRowFrom : x;
}

// font 0's capitals (Square721 BdEx BT 18): ink from 1 to 16 (the baseline) below HudPainter::text's y at scale 1, measured
// from the font texture (glyph boxes 23 high, drawn 2 above the 19-high line). inkCentredY: the y that centres that
// band in a box at y, h high, at text scale s (lowercase sits on the same baseline)
inline constexpr float kFont0CapTop = 1.f, kFont0Baseline = 16.f;
inline float inkCentredY(float y, float h, float scale)
{
    return y + h * 0.5f - (kFont0CapTop + kFont0Baseline) * 0.5f * scale;
}

// a row's panel around text w wide at x (HudPainter::text's align: 0 left, 1 right, 2 centre), h high: h / 2 margin each side
struct RowPanel
{
    float x, w;
};
inline RowPanel rowPanel(float x, float w, float h, int align)
{
    float left = align == 2 ? x - w * 0.5f : align == 1 ? x - w : x;
    return {left - h * 0.5f, w + h};
}

// the text's y centred in a panel at y, h high: the font's line x the text scale
inline float rowTextY(float y, float h, float line, float scale)
{
    return y + (h - line * scale) * 0.5f;
}
}  // namespace menu
