#include "logic/weapon_rules.hpp"

#include <algorithm>
#include <cmath>

namespace argon::weapons
{
namespace
{
const int kNemesisOffset = 8;
const int kNemesisSlot = 1;
const int kSpeedClasses = 5;
const float kNemesisShare = 0.5f;

// 0x3f3d0's jump table per speed class: the nemesis, home circuit and general drones
const AiWeaponSetup kAiSetup[kSpeedClasses][3] = {
    {{1, 0, 1}, {0, 0, 0}, {0, 0, 0}},
    {{1, 0, 3}, {1, 0, 2}, {0, 0, 1}},
    {{3, 0, 5}, {2, 0, 4}, {1, 0, 3}},
    {{3, 1, 7}, {3, 0, 6}, {2, 0, 5}},
    {{3, 1, 7}, {3, 1, 7}, {3, 0, 6}},
};

int kindColumn(DroneKind kind)
{
    if (kind == DroneKind::Nemesis)
    {
        return 0;
    }
    return kind == DroneKind::HomeCircuit ? 1 : 2;
}

uint8_t channel(uint32_t argb, int shift)
{
    return uint8_t((argb >> shift) & 0xffu);
}

uint32_t blendChannel(uint32_t from, uint32_t to, int shift, float share)
{
    const float v = float(channel(from, shift)) * (1.f - share) + float(channel(to, shift)) * share;
    return uint32_t(std::nearbyint(std::min(v, 255.f))) << shift;   // fistp: to even
}
}  // namespace

DroneKind droneKind(int pilot, int playerPilot, int trackId)
{
    if (playerPilot == (pilot ^ kNemesisOffset))
    {
        return DroneKind::Nemesis;
    }
    return trackId == pilot ? DroneKind::HomeCircuit : DroneKind::General;
}

AiWeaponSetup aiWeaponSetup(int speedClass, DroneKind kind)
{
    return kAiSetup[std::clamp(speedClass, 0, kSpeedClasses - 1)][kindColumn(kind)];
}

float nemesisShare(bool nemesisRace, int slot)
{
    return nemesisRace && slot == kNemesisSlot ? kNemesisShare : 1.f;
}

int attackPoints(float shieldPercentLost, float shieldSize, bool onPlayersNemesis, int pointsPerShieldPoint)
{
    const double percentToFraction = 0.01f;   // 0x1a355c
    // x87: the product keeps its precision until 0x6b15d stores it as the float 0x83f90 takes
    float shieldPoints = float(double(shieldSize) * (double(shieldPercentLost) * percentToFraction));
    if (onPlayersNemesis)
    {
        shieldPoints += shieldPoints;
    }
    return int(shieldPoints) * pointsPerShieldPoint;   // 0xb5130: the float's integer part (modf, fistp)
}

int fireColour(int requested, bool holdsSpecial)
{
    return holdsSpecial ? kSpecialColour : requested;
}

PowerLevels raisePowerLevels(PowerLevels levels, int pickupType, const PowerLevels &caps)
{
    const int red = 1, blue = 0, yellow = 2, top = 3;
    if (levels.red == top && levels.blue == top && levels.yellow == top)
    {
        levels.special = std::min(levels.special + 1, caps.special);
        return levels;
    }
    if (pickupType == red || pickupType == kMegaUp)
    {
        levels.red = std::min(levels.red + 1, caps.red);
    }
    if (pickupType == blue || pickupType == kMegaUp)
    {
        levels.blue = std::min(levels.blue + 1, caps.blue);
    }
    if (pickupType == yellow || pickupType == kMegaUp)
    {
        levels.yellow = std::min(levels.yellow + 1, caps.yellow);
    }
    return levels;
}

PowerLevels levelsAfterFiring(PowerLevels levels, int slot)
{
    const int colours = 3, red = 0, yellow = 1, blue = 2;
    const bool special = slot == kSpecialSlot;
    levels.red = special || slot % colours == red ? 0 : levels.red;
    levels.yellow = special || slot % colours == yellow ? 0 : levels.yellow;
    levels.blue = special || slot % colours == blue ? 0 : levels.blue;
    levels.special = 0;
    return levels;
}

PowerLevels levelsForSpecialsShield(PowerLevels levels)
{
    levels.yellow = 3;
    return levels;
}

std::vector<int> volleyTargets(const std::vector<int> &locks, int projectiles)
{
    const int none = -1;
    std::vector<int> targets(size_t(std::max(projectiles, 0)), locks.empty() ? none : locks.front());
    for (size_t k = 0; k < targets.size() && k < locks.size(); ++k)
    {
        targets[k] = locks[k];
    }
    return targets;
}

int padFireColour(const FireButtons &held)
{
    const int red = 0, blue = 2;
    if (held.b)
    {
        return red;
    }
    if (held.x)
    {
        return blue;
    }
    return held.y ? kYellowColour : kNoFire;
}

uint32_t shieldTint(const ShieldFade &fade, float age)
{
    const uint32_t opaque = 0xff000000u;
    if (!(age > fade.start))
    {
        return fade.from | opaque;
    }
    if (age >= fade.end)
    {
        return fade.to | opaque;
    }
    const float share = (age - fade.start) / (fade.end - fade.start);
    return opaque | blendChannel(fade.from, fade.to, 16, share) | blendChannel(fade.from, fade.to, 8, share) | blendChannel(fade.from, fade.to, 0, share);
}

std::optional<float> shieldSphereRadius(const ShieldSphere &sphere, float age)
{
    if (!(age > sphere.start) || !(age < sphere.end))
    {
        return std::nullopt;
    }
    const float share = (age - sphere.start) / (sphere.end - sphere.start);
    return sphere.r0 + (sphere.r1 - sphere.r0) * share;
}

}  // namespace argon::weapons
