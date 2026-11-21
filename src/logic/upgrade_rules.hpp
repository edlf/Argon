// Argon: the characters' upgrades (UPGRADEMENU, ui\upgrade.cpp; docs\frontend\tournament.md): levels 0..7 of Weapon
// Strength, Turbo and Shield (0x1bb0ec + character x 0xb0), their price, and what Weapon Strength and Turbo give in a
// race. Shield's size: WeaponBook::shieldSize (0x64030).
#pragma once
#include "logic/weapon_rules.hpp"

#include <algorithm>
#include <array>

namespace argon::upgrades
{

enum Upgrade
{
    kWeaponStrength = 0,
    kTurbo = 1,
    kShield = 2,
};

constexpr int kMaxLevel = 7;

// the prices 0x82a70 fills 0x1bc518 with at start-up, the same row for all three: the next level's
constexpr int priceOfLevel(int level)
{
    return 50000 + 75000 * level;
}

// race_craft_lap 0x3d5a0: a lap's turbo seconds
constexpr float lapTurboSeconds(int turboLevel)
{
    return 1.5f + 0.5f * float(turboLevel);
}

// 0x3f650: the player's pickup caps (red, blue, yellow, special) by Weapon Strength level
inline weapons::PowerLevels pickupCaps(int weaponStrength)
{
    struct Row
    {
        int red, yellow, blue, special;
    };
    static constexpr std::array<Row, kMaxLevel + 1> kCaps{
        {{1, 1, 1, 0}, {1, 1, 2, 0}, {1, 2, 2, 0}, {2, 2, 2, 0}, {2, 2, 3, 0}, {2, 3, 3, 0}, {3, 3, 3, 0}, {3, 3, 3, 1}}};
    const Row &row = kCaps[size_t(std::clamp(weaponStrength, 0, kMaxLevel))];
    return weapons::PowerLevels{row.red, row.blue, row.yellow, row.special};
}

}  // namespace argon::upgrades
