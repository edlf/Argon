// Argon: FE TITLE's three text elements (fe_title_screen_init 0x34460; ui\title.cpp draws them): centred at x 320,
// font 0. Element +0x1a0 is the colour, left at the default 0xff85dde5 (kText) except on "Press START to begin",
// which gets 0xffff8000 (0x34511: shell +0x374 = element +0x1d4 + 0x1a0); +0x204 = 2 on all three.
#pragma once
#include <cstdint>

#include "ui/menu_common.hpp"

namespace menu::fe
{

struct TitleText
{
    int feString;
    const char *fallback;
    float y;
    uint32_t colour;
};

inline constexpr uint32_t kPressStartColour = 0xffff8000u;

// "Press START to begin" (+0x1d4, shown from sequence 8), "Developed by" (+0x3e0, y 421 - 30 = 0x195150 - 0x1a34ac)
// and the copyright (+0x5ec, y 445 - 30 = 0x195154 - 0x1a34ac)
inline constexpr TitleText kPressStart{1, "Press START to begin", 320.f, kPressStartColour};
inline constexpr TitleText kDevelopedBy{0xc, "Developed by Curly Monsters Ltd", 391.f, kText};
inline constexpr TitleText kCopyright{0xd, "(c) 2002 Microsoft Corporation", 415.f, kText};

}  // namespace menu::fe
