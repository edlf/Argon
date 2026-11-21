// Argon: which map the HUD minimap shows for a track and at what scale (gamehud_setup 0x4725e, hud_minimap_draw
// 0x4f145). Notes: docs\physics\README.md (HUD, "Minimap").
#pragma once
#include <string>

namespace argon::hud
{

// 0x4727a: the map's name in data\hud\minimap (no extension) for the game's track id (0x52ccf8, App::courseFolder).
// Only alt_silver, alt_bracken and alt_mount have their own map; the other second routes show their first route's
// (jump table 0x49318) and every id past 0xf (Mars and alt_mars) shows mars.
std::string minimapName(int trackId);

// 0x4f145: world units per map texel: 9.9, Fuusoku and alt_fuusoku (2, 0xa) 11.88
float minimapWorldPerTexel(int trackId);

}  // namespace argon::hud
