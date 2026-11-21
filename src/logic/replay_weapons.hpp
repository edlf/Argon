// Argon: the replays' weapon entries ("WeaponEvents", type 2, 0x40 bytes; docs\replays\README.md "Weapon entry"):
// building them as the game writes them (0x641a0 / 0x646e0 / 0x64870 / 0x649f0 / weapon_launch 0x659eb) and reading
// their fields back for playback (craft_weapon 0x641f0, its switch on the signal through 0x646a8). Pure: no window,
// GPU or game state.
#pragma once
#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "logic/replay.hpp"

namespace argon::replay
{

// kind 2's +0x24 and what craft_weapon 0x641f0 does with it
enum Signal : uint32_t
{
    SignalShield = 1,        // 0x6ac90: the craft's shield (weapon +0) = the float +0x2c; at 0 or less knocked out
    SignalLaunch = 2,        // 0x64e20: projectile +0x1c leaves, +0x1f its locked target
    SignalDone = 3,          // 0x65e60: the projectile is gone
    SignalImpact = 4,        // 0x65f80: +0x28 the impact kind (1 craft, 2 world, 3 expired, 4 bounce), +0x2c where
    SignalPowerLevel = 6,    // the craft weapon's +0x9c = +0x28 (not played by the viewer: no HUD in a replay)
    SignalFire = 7,          // 0x6a390: the weapon slot +0x28 fires (a shield goes up)
    SignalPickupTaken = 8,   // +0x2c the pickup, +0x30 the craft that took it
    SignalPickupBack = 9,    // +0x2c the pickup, back on the track
    SignalRetaliate = 0xb,   // 0x6a6c0, +0x1f the attacker (the projectiles come as launches)
    SignalImpactAfter = 0xc  // 0x64b30, then as 4
};

// which projectile an entry is about: +0x1e the shooting craft, +0x1d its weapon slot, +0x1c the projectile
struct ProjectileId
{
    int craft = 0, slot = 0, projectile = 0;
    bool operator==(const ProjectileId &) const = default;
};

constexpr int kNoTarget = 0xff;

WeaponEvent shieldEvent(int craft, float shield, int killer);   // +0x1f the knock-out's killer (0x6a8a0)
WeaponEvent launchEvent(const ProjectileId &id, int target);    // +0x1f the locked target, kNoTarget for none
WeaponEvent doneEvent(const ProjectileId &id);
WeaponEvent impactEvent(const ProjectileId &id, int impactKind, const glm::vec3 &at);
WeaponEvent fireEvent(int craft, int slot);
// kind 1 (0x646e0 / 0x64870): position, velocity and the negated forward as signed bytes x 127 (as the disc's)
WeaponEvent projectileStateEvent(const ProjectileId &id, const glm::vec3 &pos, const glm::vec3 &vel, const glm::vec3 &forward);
WeaponEvent pickupTakenEvent(int pickup, int craft);
WeaponEvent pickupBackEvent(int pickup);

ProjectileId projectileOf(const WeaponEvent &event);
bool isSignal(const WeaponEvent &event, Signal signal);
bool isImpact(const WeaponEvent &event);          // 4 or 0xc
float shieldOf(const WeaponEvent &event);         // +0x2c
int killerOf(const WeaponEvent &event);           // +0x1f
int launchTargetOf(const WeaponEvent &event);     // -1 for none
int impactKindOf(const WeaponEvent &event);       // +0x28
glm::vec3 impactPositionOf(const WeaponEvent &event);   // +0x2c
int fireSlotOf(const WeaponEvent &event);         // +0x28
glm::vec3 statePositionOf(const WeaponEvent &event);    // kind 1 +0x24
glm::vec3 stateVelocityOf(const WeaponEvent &event);    // kind 1 +0x30
glm::vec3 stateForwardOf(const WeaponEvent &event);     // kind 1 +0x3c, x 1 / 127 (0x1a3a0c), negated back
int pickupOf(const WeaponEvent &event);           // +0x2c
int pickupCraftOf(const WeaponEvent &event);      // +0x30

// what a race's weapon entries add up to: recorded in the race and played in its replay, the two should agree
class ShotTally
{
  public:
    void count(const WeaponEvent &event);
    std::string summary() const;   // "<launches> launches, <impacts> impacts, ... (sequence <hash>)"
    bool operator==(const ShotTally &) const = default;

  private:
    int fires = 0, launches = 0, done = 0, impacts = 0, shields = 0, states = 0, pickups = 0;
    uint64_t sequence = 1469598103934665603ull;   // FNV-1a over the signals, their craft, slot, projectile and value
};

}  // namespace argon::replay
