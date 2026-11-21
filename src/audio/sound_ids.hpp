// Argon: the race's sounds by the game's ids (docs\audio\race-sounds.md "Race sounds"): the generic ones (audio\generic\generic.txt,
// each SF_GENERIC_ name's id from the list parser 0xde740) and each craft's ship sounds (its pilot's
// audio\ships\<pilot>\<pilot>.txt, each SF_SHIP_ name's id from 0xdda90).
#pragma once
#include <cstddef>

namespace argon
{

enum class GenericSound : int
{
    None = 0,
    RadialLaunch,
    Wind,
    RainWindshield,
    RainGround,
    TrackRumble,
    Loose,
    Rough,
    Water,
    Incoming,
    FastestLap,
    LightImpactWater,
    LightImpactLoose,
    LightImpactRough,
    LightImpactTrack,
    LightImpactMetal,
    HeavyImpactWater,
    HeavyImpactLoose,
    HeavyImpactRough,
    HeavyImpactTrack,
    HeavyImpactMetal,
    ScrapeMetal,
    ScrapeWall,
    MenuNavigate,
    MenuSelect,
    BadMenuSelect,
    PowerPickup,
    ChipPickup,
    MegaPickup,
    LaserImpact,
    ProjectileImpact,
    MegaImpact,
    PlayerHit,
    Ching,
    PerChing,
    LowEnergy,
    Count
};

constexpr size_t kGenericSoundCount = size_t(GenericSound::Count);

// generic.txt's name of the sound (SF_GENERIC_ + this), in id order
constexpr const char *kGenericSoundNames[kGenericSoundCount] = {
    "",
    "RADIALLAUNCH",
    "WIND",
    "RAINWINDSHIELD",
    "RAINGROUND",
    "TRACKRUMBLE",
    "LOOSE",
    "ROUGH",
    "WATER",
    "INCOMING",
    "FASTESTLAP",
    "LIGHTIMPACTWATER",
    "LIGHTIMPACTLOOSE",
    "LIGHTIMPACTROUGH",
    "LIGHTIMPACTTRACK",
    "LIGHTIMPACTSOLMETAL",
    "HEAVYIMPACTWATER",
    "HEAVYIMPACTLOOSE",
    "HEAVYIMPACTROUGH",
    "HEAVYIMPACTTRACK",
    "HEAVYIMPACTSOLMETAL",
    "SCRAPEMETAL",
    "SCRAPEWALL",
    "MENUNAVIGATE",
    "MENUSELECT",
    "BADMENUSELECT",
    "POWERPICKUP",
    "CHIPPICKUP",
    "MEGAPICKUP",
    "LASERIMPACT",
    "PROJECTILEIMPACT",
    "MEGAIMPACT",
    "PLAYERHIT",
    "CHING",
    "PERCHING",
    "LOWENERGY",
};

enum class ShipSound : int
{
    TravelProjectile,
    TravelLaser,
    TravelSpecial,
    TravelRadial,
    ProjectileFire,
    MegaFire,
    LaserFire,
    TurboFire,
    TurboFail,
    EnginePowerUp,
    EnginePowerDown,
    Count
};

constexpr size_t kShipSoundCount = size_t(ShipSound::Count);

// the pilot list's name of the sound (SF_SHIP_ + this), in id order
constexpr const char *kShipSoundNames[kShipSoundCount] = {
    "WEAPONTRAVELPROJECTILE",
    "WEAPONTRAVELLASER",
    "WEAPONTRAVELSPECIAL",
    "WEAPONTRAVELRADIAL",
    "PROJECTILEFIRE",
    "MEGAFIRE",
    "LASERFIRE",
    "TURBOFIRE",
    "TURBOFAIL",
    "ENGINEPOWERUP",
    "ENGINEPOWERDOWN",
};

}  // namespace argon
