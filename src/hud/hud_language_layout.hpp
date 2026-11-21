// Argon: the race HUD positions that depend on the language (0x5293b0, XGetLanguage: 2 = Japanese), from
// gamehud_layout 0x45270 and gamehud_render 0x4a100. Every other language takes the same values. Notes:
// docs\physics\README.md (HUD, "Japanese layout").
#pragma once
#include "saves/viewer_settings.hpp"

namespace argon::hud
{

// offsets in 640 x 480 units from the single-player safe area (T, R, B) or from the bar / box they sit in
struct HudLanguageLayout
{
    float timeRowStep;        // time / prev / best rows from T + 52 (0x4578a): 14, Japanese 19
    float scoreValueTop;      // the score's value below T (0x456f5): 13, Japanese 15
    float speedValueLeft;     // the speed read-out left of R (0x455f0): 55, Japanese 58
    float speedValueAbove;    // and above B: 40, Japanese 36
    float speedUnitAbove;     // "km/h" / "mph" above B (0x454aa): 28, Japanese 26
    float shieldBarTop;       // single player: the shield line below T (0x4b0ce): 61, Japanese 79 ("Shield" 19 above)
    float shieldBarDrop;      // the bar below that line, split screen too (0x4b1f7): 0, Japanese 3
    float shieldValueDrop;    // the "%d" below that line (0x4b268): 2, Japanese 3.5
    float turboTextDrop;      // single player: "Turbo: %.1fs" below its box's top (0x4aa1e): 1, Japanese 0.5
    float splitTurboTextDrop; // split screen: "%.1fs" below its box's top (0x4ab6e / 0x4ac84): 1, Japanese 3.5
};

HudLanguageLayout hudLanguageLayout(Language language);

}  // namespace argon::hud
