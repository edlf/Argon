// Argon: data\weapon.nwp as the game loads it (0x640c0, docs\weapons\hud.md "Records"): 16 pilot blocks of 11 weapon
// records (0x334 bytes) with the pilot's resistances, then the explosion types (copied to 0x1e8bb8).
#pragma once
#include "logic/explosion.hpp"
#include "logic/trail_sprite.hpp"
#include "logic/weapon_rules.hpp"
#include "original/weapon_records.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace argon::weapons
{

// one weapon record, as the firing, the projectiles, the hits and the shields read it
struct WeaponRec
{
    std::string name;
    int slot = 0, cls = 0, trailType = 0, count = 0, maxBounces = 0;
    bool lockOn = false;   // +0x150: locks targets (crosshairs), one per projectile
    bool radiusFlag = false, homing = false, hugBelow = false, hugSide = false, hugAbove = false, bounces = false;
    bool repeats = false;  // +0x13c: each projectile relaunches every count x stagger until repeatTime
    bool holdsSpeedAfterBounce = false;   // +0x156
    bool aimsAtLock = false;              // +0x167: launched looking at its target
    bool pitchedLaunch = false;           // +0x13f: launched along the craft, nose up by launchPitch degrees
    float launchPitch = 0, reactSpeed = 0;   // +0x18c, +0x148
    float scatterA = 0, scatterB = 0;        // +0xe4 / +0xe8: the bursts' explosions scattered this far
    uint8_t shieldFlags = 0, retaliateSlot = 0;   // +0x1b0 / +0x1b3
    uint32_t trailFrom = 0, trailTo = 0;
    float trailStep = 0, back = 0, trailSpeed = 0, trailJitter = 0, trailLife = 0, life = 0, hugDist = 0, turn = 0, accel = 0, maxSpeed = 0, drop = 0,
          inherit = 0, lockRange = 0, hitRadius = 0, homingDelay = 0, speedPct = 0, speedAdd = 0, stagger = 0, repeatTime = 0;
    original::NwpEffects effects{};       // +0x4c what a hit does
    original::NwpEffects shieldBonus{};   // +0x90 shields: resistances while up
    original::NwpImpactBursts impact{};   // +0xd4
    TrailSpriteStyle sprite{};            // +0xec > 3: trail sprites
    ShieldFade fade{};                    // shields: +0x1a0..+0x1ac
    ShieldSphere sphere{};                // shields: +0x184..+0x190
};

// an explosion type: how it looks and how hard it pushes (+0x18, 0x7e410)
struct ExplosionType
{
    ExplosionLook look;
    float push = 0.f;
};

class WeaponBook
{
  public:
    static constexpr int kPilots = original::kNwpPilotCount;
    static constexpr int kSlots = 11;
    static constexpr int kFirstSpriteTrail = 4;   // trail types 1..3 point particles, 4.. sprites (0x66650)

    // the file's bytes; too short for the pilot blocks leaves no records, too short for the explosion table no types
    void read(const std::vector<uint8_t> &nwp);
    bool hasRecords() const;
    // pilot & 15; slot 0..kSlots - 1
    const WeaponRec &record(int pilot, int slot) const;
    const original::NwpEffects &resistances(int pilot) const;
    // clamped to the table
    const ExplosionType &explosionType(int type) const;
    // 0x6aa00: the shield a pilot starts a race with at an upgrade level
    float shieldSize(int pilot, int level) const;

  private:
    void readExplosionTypes(const std::vector<uint8_t> &nwp);

    std::vector<std::array<WeaponRec, kSlots>> records_;   // [pilot][slot]
    std::array<original::NwpEffects, kPilots> resistances_{};
    std::array<ExplosionType, original::kNwpExplosionTypes> explosionTypes_{};
};

}  // namespace argon::weapons
