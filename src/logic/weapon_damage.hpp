// Argon: the damage arithmetic of a weapon hit (weapon_damage 0x6acf0, resistances 0x6aa00), without the
// race around it: the area falloff, the shield's resistance, what an active TSHIELD does to a hit, and the single
// player's knock-out mercy. docs\weapons\README.md "Damage". Weapons::applyHit (weapon_hits.cpp) applies the result.
#pragma once
#include <optional>
#include <vector>

#include "original/weapon_records.hpp"

namespace argon::weapons
{

// 0x6acf0: area weapons (flags 0x400) scale every channel by the inner % within the inner radius, falling linearly to
// the outer % at the outer radius; at or beyond it the hit does nothing. Other weapons: 1.
std::optional<float> areaFactor(const original::NwpEffects &damage, float distance);

// 0x7e410: an explosion that carries a weapon's effects pushes every craft but the shooter's away from it, area weapon or
// not: the inner % within the inner radius, from 0.01 past it linear to the outer % at the outer radius, nothing from
// there on, x the explosion type's push scale (weapon.nwp's explosion table +0x18) x 0.01
float blastPush(const original::NwpEffects &damage, float distance, float typeScale);

// 0x6aa00: the shield-damage resistance = 100 - 10000 / shield size (so a hit takes `damage` points of the shield
// whatever its size), plus each active shield's bonus (+0x90), capped at 100 after each one.
float shieldResistance(float shieldSize, const std::vector<const original::NwpEffects *> &activeShields);

// what an active shield (class 2) does to a hit: blocked (a shield with a shield bonus > 0, 0x6aba0) zeroes the hit
// unless it heals; absorb (flag 4, 0x6ab60) turns the shield damage into a recharge
struct ShieldGuard
{
    bool blocked = false;
    bool absorbs = false;
};

// a hit scaled for its target, in % of the target's shield
struct ScaledHit
{
    float shieldPercent = 0;   // > 0 damage, < 0 recharge
    float leechPercent = 0;    // the shooter gets this % of the shield damage (0x6b7e0)
};

struct HitResistance
{
    float shield = 0;          // shieldResistance
    float leech = 0;           // the pilot's channel 1 (+ shield bonuses +0x94)
};

ScaledHit scaleHit(const original::NwpEffects &damage, const HitResistance &resistance, float areaScale, const ShieldGuard &guard);

// 0x6b6ee: in a single player race a hit that empties the player's shield from 33 % or more leaves it at 1..6 %
constexpr float kMercyFromPercent = 33.f;
bool sparesThePlayer(float percentBeforeHit);
float mercyPercent(unsigned random10Bits);   // 1 + (rand & 0x3ff) x 5 / 1023

}  // namespace argon::weapons
