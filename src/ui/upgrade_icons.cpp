// Argon: UPGRADEMENU's weapon icons (draw 0x9f040). u = level + 3 is the weapon the next upgrade gives (infotext
// character x 10 + u); the caps rise blue, yellow, red, so u 3 / 6 is blue 2 / 3, 4 / 7 yellow, 5 / 8 red, 9 the special
#include "ui/upgrade_icons.hpp"

#include <cmath>

namespace menu
{
namespace
{
constexpr uint32_t kWhite = 0xffffffffu, kBlack = 0xff000000u;
constexpr int kSpecialNext = 9, kAllCaps = 10;

std::string weaponTexture(const char *colour, int level)
{
    return std::string("weapon") + colour + char('0' + level);
}

// 0x9f507..0x9f5ad: red 0x597ca8 + 0x28 x (1 + (u > 5) + (u > 8)), blue 0x597d20 + .. (1 + (u > 3) + (u > 6)),
// yellow 0x597d98 + .. (1 + (u > 4) + (u > 7)): weaponred1 sits at 0x597cd0, nine sprites 0x28 apart
void addCaps(std::vector<UpgradeIconLayer> &layers, int u)
{
    layers.push_back({weaponTexture("red", 1 + (u > 5) + (u > 8)), kWhite, IconBlend::Additive});
    layers.push_back({weaponTexture("blue", 1 + (u > 3) + (u > 6)), kWhite, IconBlend::Additive});
    layers.push_back({weaponTexture("yellow", 1 + (u > 4) + (u > 7)), kWhite, IconBlend::Additive});
}

// 0x9f60b..0x9f64c: u 3 blue2 (0x597d70), 4 yellow2 (0x597de8), 5 red2 (0x597cf8), 6 blue3, 7 yellow3, 8 red3
std::string nextCap(int u)
{
    static const char *const kOrder[3] = {"blue", "yellow", "red"};
    return weaponTexture(kOrder[(u - 3) % 3], u <= 5 ? 2 : 3);
}

// 0x9f600: pulse x 0x10101 - 0x1000000 = opaque grey; 0x9f667: (pulse << 24) + 0xffffff = white at that alpha
void addPulsingNext(std::vector<UpgradeIconLayer> &layers, int u, int pulse)
{
    const uint32_t pulseLevel = uint32_t(pulse);
    if (u == kSpecialNext)
    {
        layers.push_back({"weaponspecial", (pulseLevel << 24) | 0xffffffu, IconBlend::Alpha});
        return;
    }
    layers.push_back({nextCap(u), kBlack | (pulseLevel * 0x10101u), IconBlend::Additive});
}
}  // namespace

int upgradeIconPulse(int frame)
{
    const double radians = double(frame) * 10.0 * 3.14159265358979 / 180.0;   // 0x5a211c (pi / 180) x 10 (0x1a35a4)
    return 128 - int(std::lround(std::sin(radians) * -96.0));                  // 0x1a3900 = -96.0, fistp rounds
}

// 0x9f43e: blend mode 1, weaponbase (0x1be970) at 0xff000000 three times, then 0xffffffff
std::vector<UpgradeIconLayer> upgradeWeaponIcons(int weaponLevel, int pulse)
{
    const int u = weaponLevel + 3;
    std::vector<UpgradeIconLayer> layers(3, UpgradeIconLayer{"weaponbase", kBlack, IconBlend::Alpha});
    layers.push_back({"weaponbase", kWhite, IconBlend::Alpha});
    if (u >= kAllCaps)
    {
        layers.push_back({"weaponspecial", kWhite, IconBlend::Alpha});   // 0x9f4e6: mode 1, 0x1be998, no caps
        return layers;
    }
    addCaps(layers, u);
    addPulsingNext(layers, u, pulse);
    return layers;
}
}  // namespace menu
