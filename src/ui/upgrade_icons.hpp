// Argon: UPGRADEMENU's weapon icons (draw 0x9f040, 0x9f43e..0x9f68d): the layers drawn 128 x 64 at (430, 150) for a
// Weapon Strength level (docs\frontend\tournament.md "UPGRADEMENU")
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace menu
{
enum class IconBlend
{
    Alpha,      // blend mode 1 of 0xa9530 (SRCALPHA / INVSRCALPHA)
    Additive,   // blend mode 2 (ONE / ONE)
};

struct UpgradeIconLayer
{
    std::string texture;   // data\hud\<texture>.dds
    uint32_t argb;
    IconBlend blend;
};

// 0x9f5b2: 128 + round(96 sin(frame x 10 deg)), frame = the 60 Hz counter 0x439df0 (32..224)
int upgradeIconPulse(int frame);

// level 0..7 and the pulse (upgradeIconPulse): weaponbase three times black then white; at level 7 weaponspecial;
// else each colour's cap after the next upgrade's predecessor, then the next one pulsing (its brightness, additive;
// level 6: weaponspecial's alpha)
std::vector<UpgradeIconLayer> upgradeWeaponIcons(int weaponLevel, int pulse);
}  // namespace menu
