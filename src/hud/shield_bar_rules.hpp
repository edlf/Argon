// Argon: the shield bar's fill colour, gamehud_render 0x4b1b4..0x4b1f4. Notes: docs\physics\README.md (HUD, "Shield line").
#pragma once
#include <cstdint>

namespace argon::hud
{

// the fill's ARGB for the shield (huddata +0x44, in %), its flash (huddata +0x70 = craft weapon +0x94, raised by hits
// and the corrode) and the vertical blank count (0x439df0, 60 a second): yellow 0xffffd000; red 0xffff0000 on blanks
// 8..15 of each 16 under 0.33 % (the game compares the % with 0.33, so a real Xbox practically never blinks red);
// white 0xffffffff on blanks 0..7 of each 16 while the flash is above 0
uint32_t shieldFillColour(float shieldPercent, float flashLevel, unsigned vblanks);

}  // namespace argon::hud
