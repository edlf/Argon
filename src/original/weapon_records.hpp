// The game's data\weapon.nwp byte for byte (loader 0x640c0): 16 pilot blocks of 11 weapon records, copied to
// 0x1c5270 (then 0x1480 more bytes to 0x1e8bb8). Field meanings: docs\weapons\README.md. The unknown fields hold data in
// most records (tools\cmm\record_survey.py NwpWeapon); notes are from the launch code (0x64e20).
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

constexpr int kNwpPilotCount = 16;
constexpr int kNwpWeaponsPerPilot = 11;

#pragma pack(push, 1)

// The effect channels a hit applies (0x6acf0). The same 17 slots appear three times: a weapon's damage, a shield's
// resistance bonus while it is up, and each pilot's resistances. A hit applies channel k x the area factor x
// (100 - resistance) % (corrode level uses the shield damage resistance, the times their level's), then the shooter
// gets shieldLeech % of the shield damage back (0x6b7e0). The craft's weapon object counts the times down
// (craft_weapon_update 0x6d000).
struct NwpEffects
{
    float shieldDamage;           // % of the shield
    float shieldLeech;
    float corrodeLevel;           // shield lost per second ...
    float corrodeTime;            // ... for this long
    float freezeTime;
    float controlJitter, controlJitterTime;
    float engineJitterLevel, engineJitterTime;
    float engineOffTime;
    float unused28[2];
    uint32_t flags;               // damage only: 0x400 area weapon (no direct hit damage)
    float areaInnerRadius;        // damage only, area weapons: areaInnerPercent within the inner radius, falling
    float areaOuterRadius;        // linearly to areaOuterPercent at the outer one, nothing beyond
    float areaInnerPercent;
    float areaOuterPercent;
};

// The impact bursts (0x65f80): two particle systems A and B, each `count` particles `delay` seconds apart.
struct NwpImpactBursts
{
    uint8_t typeA, typeB;
    uint8_t flagsA, flagsB;       // 2 on hit kind 3, 8 on hit kind 4, 4 an extra flash (0x9d000), 0x10 the
                                  // particles carry no damage (else they carry the weapon's NwpEffects)
    uint8_t countA, countB;
    uint8_t sizePercentA, sizePercentB;
    float delayA, delayB;
};

struct NwpWeapon
{
    uint32_t unknown00[2];
    int32_t weaponClass;          // 0 projectile, 1 beam, 2 shield
    char name[0x20];
    uint8_t unknown2c[0x1f];
    uint8_t radiusFlag;
    NwpEffects damage;
    NwpEffects shieldBonus;       // shields: added to the owner's resistances while up; on launch the owner's
                                  // running corrode / freeze / engine effects are cut by the matching %
    NwpImpactBursts impact;
    float scatterA;               // burst A's explosions land up to this far off the impact per axis (0 in every file)
    float scatterB;               // burst B's (0x66329; 10..50 in 70 records)
    int32_t trailType;
    uint32_t trailFlags;          // sprite trails: 1 random spin direction, 2 alpha blended (else additive),
                                  // 4 fade in over trailFade (0x9d000)
    uint32_t trailFrom, trailTo;
    float trailStep;
    float back;
    float trailSpeed;
    float trailJitter;
    float unknown10c;             // 2 / 10 / 50 in 55 records
    float trailSize, trailSizeRandom;   // sprite trails: size = trailSize + random x trailSizeRandom
    float trailGrow;              // sprite trails: the size x (1 + (trailGrow - 1) x 20 dt^2) a frame (0x9c540)
    float trailSpin, trailSpinRandom;
    float trailFade;              // sprite trails: the colour holds (fades in with flag 4) this long, then blends (0 in every file)
    float trailLife;
    uint8_t unknown12c[0x10];
    uint8_t repeats;              // each projectile relaunches every count x stagger while repeatTime lasts (0x69e7e)
    uint8_t unknown13d;
    uint8_t homing;
    uint8_t pitchedLaunch;        // launched turned about x by launchPitchDegrees from the craft's matrix
    float life;
    float repeatTime;             // seconds after firing that a repeating weapon keeps relaunching
    float surfaceReactSpeed;      // a surface hit is handled above this speed (always with bounces)
    uint32_t unused14c;
    uint8_t lockOn;               // locks targets (crosshairs), one per projectile
    uint8_t unknown151;
    uint8_t hugBelow;
    uint8_t unknown153;
    uint8_t hugSide;
    uint8_t hugAbove;
    uint8_t holdsSpeedAfterBounce;   // with bounces: no more acceleration once it has bounced (0x66f4b)
    uint8_t unknown157[5];
    float hugDistance;
    float turnRate;
    uint8_t unknown164;
    uint8_t bounces;
    uint8_t unknown166;
    uint8_t aimsAtLock;           // with lockOn and a target: launched looking at it (0x65a7d)
    int32_t count;
    int32_t maxBounces;
    float stagger;                // projectile k leaves k x stagger seconds after the shot (0x6a390, 0x69e30)
    float acceleration;
    float maxSpeed;
    float drop;
    float inherit;
    float sphereStart;            // shields (0x69970): from this age to lockRange's (sphereEnd) a sphere grows from
                                  // launchPitchDegrees' (radius r0) to hitRadius' (r1); 0 for projectiles
    float lockRange;              // shields: the sphere's end age
    float launchPitchDegrees;     // clamped to 0..90; shields: the sphere's start radius
    float hitRadius;              // shields: the sphere's end radius
    float homingDelay;
    float speedPercent;
    float speedAdd;
    float shieldFadeFrom;         // shields (0x69970): the model's colour goes from shieldColour at this age ...
    float shieldFadeTo;           // ... to shieldFadeColour at this one (seconds)
    uint32_t shieldColour;        // ARGB
    uint32_t shieldFadeColour;    // ARGB
    uint8_t shieldFlags;          // shields while up: 4 absorb (a hit's shield damage recharges it, 0x6ab60),
                                  // 0x10 retaliate (a hit fires slot retaliateSlot at the attacker, 0x6abf0)
    uint8_t unused1b1[2];
    uint8_t retaliateSlot;        // 10 (RETALIATOR)
    uint8_t unused1b4[0x180];
};

struct NwpPilot
{
    uint32_t weaponCount;         // 11
    NwpEffects resistances;       // % per channel; shieldDamage is combined with the pilot's byte 0x52cd24
    NwpWeapon weapons[kNwpWeaponsPerPilot];
};

// After the pilots: the explosion types (0x1480 bytes copied to 0x1e8bb8), indexed by NwpImpactBursts::typeA / B;
// only 0..15 have a model (objects\explosion_type<n>, 0x7dd40), 0x7e0f0 turns a higher type into 15
constexpr int kNwpExplosionTypes = 32;
struct NwpExplosionType
{
    float animationScale;         // +0 the bones' clock = the explosion's progress x this (0x7e077)
    float durationScale;          // +4 the duration asked for x this (0x7e117)
    float sizeScale;              // +8 the model's scale = the size asked for x this x 0.01 (0x7e125)
    float flashScale;             // +0xc its flash's half size = the scale x this, none when not above 0 (0x7df15)
    uint32_t unknown10;           // not read by the code
    float unknown14;              // not read by the code
    float pushScale;              // +0x18 the push on the crafts around it (0x7e410)
    float pushArgument1c;         // passed to 0x7e410 beside the push, which does not read it (100 in most types)
    uint8_t flatScatter;          // +0x20 an impact burst scatters its explosions in x / z only (0x6601f, burst A's type)
    uint8_t tumbles;              // +0x21 turned at random about all three axes (0x7e247)
    uint8_t turns;                // +0x22 turned at random about y only (0x7e22f)
    uint8_t unknown23[0xa4 - 0x23];
};

#pragma pack(pop)

static_assert(sizeof(NwpExplosionType) == 0xa4 && offsetof(NwpExplosionType, pushScale) == 0x18);
static_assert(offsetof(NwpExplosionType, flatScatter) == 0x20 && offsetof(NwpExplosionType, turns) == 0x22);

static_assert(sizeof(NwpWeapon) == 0x334);
static_assert(offsetof(NwpWeapon, name) == 0x0c && offsetof(NwpWeapon, radiusFlag) == 0x4b && offsetof(NwpWeapon, damage.flags) == 0x7c);
static_assert(offsetof(NwpWeapon, trailType) == 0xec && offsetof(NwpWeapon, trailLife) == 0x128);
static_assert(offsetof(NwpWeapon, homing) == 0x13e && offsetof(NwpWeapon, lockOn) == 0x150);
static_assert(offsetof(NwpWeapon, hugDistance) == 0x15c && offsetof(NwpWeapon, bounces) == 0x165);
static_assert(offsetof(NwpWeapon, acceleration) == 0x174 && offsetof(NwpWeapon, lockRange) == 0x188);
static_assert(offsetof(NwpWeapon, weaponClass) == 0x08 && offsetof(NwpWeapon, damage) == 0x4c);
static_assert(offsetof(NwpWeapon, trailFrom) == 0xf4 && offsetof(NwpWeapon, trailTo) == 0xf8 && offsetof(NwpWeapon, trailStep) == 0xfc);
static_assert(offsetof(NwpWeapon, back) == 0x100 && offsetof(NwpWeapon, trailSpeed) == 0x104 && offsetof(NwpWeapon, trailJitter) == 0x108);
static_assert(offsetof(NwpWeapon, life) == 0x140 && offsetof(NwpWeapon, hugBelow) == 0x152);
static_assert(offsetof(NwpWeapon, hugSide) == 0x154 && offsetof(NwpWeapon, hugAbove) == 0x155 && offsetof(NwpWeapon, turnRate) == 0x160);
static_assert(offsetof(NwpWeapon, count) == 0x168 && offsetof(NwpWeapon, maxBounces) == 0x16c);
static_assert(offsetof(NwpWeapon, maxSpeed) == 0x178 && offsetof(NwpWeapon, drop) == 0x17c && offsetof(NwpWeapon, inherit) == 0x180);
static_assert(offsetof(NwpWeapon, hitRadius) == 0x190 && offsetof(NwpWeapon, homingDelay) == 0x194);
static_assert(offsetof(NwpWeapon, speedPercent) == 0x198 && offsetof(NwpWeapon, speedAdd) == 0x19c);
static_assert(offsetof(NwpPilot, resistances) == 0x04 && sizeof(NwpEffects) == 0x44 && sizeof(NwpImpactBursts) == 0x10);
static_assert(offsetof(NwpWeapon, shieldBonus) == 0x90 && offsetof(NwpWeapon, impact) == 0xd4 && offsetof(NwpWeapon, trailSize) == 0x110);
static_assert(offsetof(NwpWeapon, surfaceReactSpeed) == 0x148 && offsetof(NwpWeapon, aimsAtLock) == 0x167);
static_assert(offsetof(NwpWeapon, shieldFlags) == 0x1b0 && offsetof(NwpWeapon, retaliateSlot) == 0x1b3);
static_assert(offsetof(NwpWeapon, launchPitchDegrees) == 0x18c && offsetof(NwpWeapon, pitchedLaunch) == 0x13f);
static_assert(offsetof(NwpWeapon, repeats) == 0x13c && offsetof(NwpWeapon, repeatTime) == 0x144);
static_assert(sizeof(NwpPilot) == 0x2384 && offsetof(NwpPilot, weapons) == 0x48);
static_assert(offsetof(NwpWeapon, trailFlags) == 0xf0 && offsetof(NwpWeapon, trailGrow) == 0x118 && offsetof(NwpWeapon, trailFade) == 0x124);
static_assert(offsetof(NwpWeapon, sphereStart) == 0x184 && offsetof(NwpWeapon, shieldFadeFrom) == 0x1a0 && offsetof(NwpWeapon, shieldColour) == 0x1a8);

} // namespace argon::original
