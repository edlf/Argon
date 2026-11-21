// Argon: the weapon rules that depend on who races (docs\weapons\ai.md "Drones and the nemesis race",
// "Shields", "Firing"): the drone kinds (craft setup 0x58e40: +0x9f8 home circuit, +0x9f9 nemesis), the AI's pickup
// caps and shield level (race setup 0x3f3d0), the nemesis race (0x791f0) and its halving, the special's fire rule
// (0x6d000), the power-up levels (0x6ba30, 0x6c33e), the volley's targets (0x6c110) and the shield's colour fade and
// sphere (0x69970).
#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace argon::weapons
{

// craft setup 0x58e40: +0x9f9 nemesis (the player's pilot is this pilot's nemesis, id +- 8), else +0x9f8 home circuit
// (the race's track is the pilot's home, track id = pilot id); 0x3f3d0 checks the nemesis first
enum class DroneKind
{
    General,
    HomeCircuit,
    Nemesis
};
DroneKind droneKind(int pilot, int playerPilot, int trackId);

// 0x3f3d0 for an AI craft: its red / blue / yellow pickup caps (one value for all three, 0x52cd0c / 12 / 18), its
// special cap (0x52cd1e) and its Shield upgrade level (0x52cd24, the shield size's 20 x level)
struct AiWeaponSetup
{
    int levelCap = 0, specialCap = 0, shieldLevel = 0;
};
AiWeaponSetup aiWeaponSetup(int speedClass, DroneKind kind);

// the craft in grid slot 1 (the nemesis, craft id +0x794 = 1) of a nemesis race takes half: hit shield damage
// (0x6b085), corrode (0x6d313) and its shield launch recharge (0x6a439: 5 / 10 / 15)
float nemesisShare(bool nemesisRace, int slot);

// 0x83f90 with 0x64070: the player's hit earns `pointsPerShieldPoint` (weaponhit_bonus) per whole shield point it takes,
// the target's shield size x the % lost before the nemesis race's halving (0x6b135 doubles it back), twice that on the
// player's nemesis (its leftover "awesome" flag; the popup stays "Attack")
int attackPoints(float shieldPercentLost, float shieldSize, bool onPlayersNemesis, int pointsPerShieldPoint);

// 0x6d000: while the craft holds a special any fire request fires it (colour 3); afterwards the yellow colour fires at
// level 3 too (0x6d0b1). Viewer colours: 0 red, 1 yellow, 2 blue, 3 special.
const int kSpecialColour = 3;
const int kYellowColour = 1;
int fireColour(int requested, bool holdsSpecial);

const int kSpecialSlot = 9;   // weapon.nwp record 9

// a craft's power-up levels (weapon object +0xa4 red, +0xa8 blue, +0xac yellow, 0..3) and its specials (+0xb0)
struct PowerLevels
{
    int red = 0, blue = 0, yellow = 0, special = 0;
};

// weapon_pickup 0x6ba30: pickup type 1 red, 0 blue, 2 yellow, 3 mega-up (all three), each capped; with all three at 3
// any pickup adds a special instead (capped by 0x52cd1e)
const int kMegaUp = 3;
PowerLevels raisePowerLevels(PowerLevels levels, int pickupType, const PowerLevels &caps);

// 0x6c33e: after firing weapon slot `slot` its colour's level goes to 0 (slots 0 / 3 / 6 red, 1 / 4 / 7 yellow, 2 / 5
// / 8 blue), the special (slot 9) empties all three; the special count always goes to 0
PowerLevels levelsAfterFiring(PowerLevels levels, int slot);

// 0x6d0b1: after the special's request the yellow level is set to 3, so the level 3 shield (2TSHIELD) fires next
PowerLevels levelsForSpecialsShield(PowerLevels levels);

// weapon_select 0x6c22e..0x6c2f2: every projectile's target is cleared (-1); a lock-on weapon with targets locked
// (+0x58 > 0, nearest first at +0x60) aims every projectile at the nearest, then projectile k at the k-th lock
std::vector<int> volleyTargets(const std::vector<int> &locks, int projectiles);

// player_pad_input 0x70fd2: the held pad bits B (0x20), X (0x40), Y (0x80) -> craft +0x91c / +0x91d / +0x91e; 0x71918
// asks for one colour a frame: B red, else X blue, else Y yellow (-1: none). Holding a button fires as soon as the
// colour is armed and its last shot is gone.
struct FireButtons
{
    bool b = false, x = false, y = false;
};
const int kNoFire = -1;
int padFireColour(const FireButtons &held);

// 0x69970: a shield's model colour (bone tint +0x104, ARGB, alpha forced to 0xff) goes from `from` at age `start` to
// `to` at age `end` (+0x1a0..+0x1ac)
struct ShieldFade
{
    float start = 0, end = 0;
    uint32_t from = 0, to = 0;
};
uint32_t shieldTint(const ShieldFade &fade, float age);

// 0x69970: between ages `start` and `end` a sphere from the shield bone grows from `r0` to `r1` (+0x184..+0x190); each
// craft inside it gets the weapon's impact (kind 1) once
struct ShieldSphere
{
    float start = 0, end = 0, r0 = 0, r1 = 0;
};
std::optional<float> shieldSphereRadius(const ShieldSphere &sphere, float age);

}  // namespace argon::weapons
