// Argon: the rules of the race's event sounds (docs\audio\race-sounds.md "Race sounds"): which generic sound a craft's world
// contacts, a craft pair or a splash plays (0xc2b50, craft_water 0x61340), how loud (0xcabc0 with the forces of
// audio\f_settings.txt, sound_load_f_settings 0xc9890), the rolloff of a positional sound (the curve 0x194f5c) and
// the score page's tally (0x95b00).
#pragma once
#include <glm/glm.hpp>
#include <string>

#include "audio/sound_ids.hpp"

namespace argon::sound
{

// the forces where a hit turns light, heavy and loudest (f_settings.txt "<light / heavy / max> impact <kind>")
struct ForceRange
{
    float light, heavy, max;
};

// f_settings.txt; the defaults are 0xc96e0's. The game reads the file line by line in a fixed order and stops at the
// shipped "scrape trail 0.1" (an integer read stopped by the fraction), so its ship and water lines never apply. Water
// forces are downward speeds (the file stores them negative).
struct ImpactSettings
{
    ForceRange world{20.f, 60.f, 150.f};
    ForceRange ship{10.f, 60.f, 100.f};
    ForceRange water{1.f, 10.f, 20.f};
    float impactDelay = 0.25f;   // "impact time delay": a craft's same hit sound does not start again sooner
};
ImpactSettings parseImpactSettings(const std::string &text);

// a sound folder's MINVOL / MAXVOL (mB)
struct VolumeRange
{
    float minMb = -10000.f, maxMb = 0.f;
};
// the folder parser (0xc6ea6..) reads LOOPS, MINVOL and MAXVOL, the record's shorts +0x98 / +0x9a / +0x9c, with
// sscanf("%d") into an int: each also writes its value's high half over the next short. So LOOPS 1 leaves MINVOL 0
// (the scrapes, rain, wind and terrain loops play at full whatever the force) and a negative MINVOL leaves MAXVOL -1.
void scanLoopsInto(int loops, VolumeRange &volume);
void scanMinVolumeInto(int minMb, VolumeRange &volume);
// 0xcabc0: a light hit (up to `heavy`) runs from MINVOL to MAXVOL over light..heavy, a heavy one again over heavy..max,
// in whole percent steps; -10000..0 mB
float hitVolumeMb(float force, const ForceRange &forces, const VolumeRange &volume);

// a craft's world contacts this frame, summed as the game's contact record 0x442e94 sums them (physics_substep 0xcffb0);
// 0xd0a30 then divides the dot sum by the count before 0xc2b50 reads it
struct CraftContact
{
    int count = 0;                    // contacts that pushed the craft back
    glm::vec3 velocityChange{0.f};    // what the responses changed the velocity by
    float normalDotSum = 0.f;         // the contacts' normal . the velocity's direction (1: head on), summed
    int surface = 0;                  // the last contact's surface (0 track, 1 rough, 2 loose, 3 water, 4 slippery)
};

struct ContactSound
{
    GenericSound sound = GenericSound::None;
    bool scrape = false;        // a loop that holds until the scrapes stop
    bool stopScrapes = false;   // the craft's ScrapeMetal / ScrapeWall loops end first
    float force = 0.f;          // for hitVolumeMb with the world forces
};
// 0xc2b50: a glancing contact (|dot| < 0.35) scrapes (from |dot| 0.1), anything else hits by dot x |change|
ContactSound worldContactSound(const CraftContact &contact, const ImpactSettings &settings);
// 0xc2b50: two crafts touching, by how much the first one's velocity changed
GenericSound shipImpactSound(float velocityChange, const ImpactSettings &settings);

struct Splash
{
    GenericSound sound = GenericSound::None;
    GenericSound unlessPlaying = GenericSound::None;   // the craft's other splash, still playing, holds this one
};
// craft_water 0x61340: a corner under water at this vertical speed
Splash waterSplash(float verticalSpeed, const ImpactSettings &settings);

// the rolloff curve 0x194f5c (1, 1/2, 1/4, 1/8, 0) as DirectSound spreads it: 1 at MINDIST, the five points at
// the ends of five equal steps up to MAXDIST (the first, 1, keeps the full volume over the first step)
float rolloffGain(float distance, float minDist, float maxDist);

// weapon_launch 0x65351: a shield's (class 2) launch plays SF_GENERIC_RADIALLAUNCH for craft 0 only, for every craft
// in split screen (game mode 4)
bool shieldLaunchSounds(int slot, bool splitScreen);

// 0x95b00, once a frame from SCOREMENU's opening: the shown score climbs by a step that grows each frame
class ScoreTally
{
  public:
    void start(int target);
    bool step();                 // true when the score is reached (the tally ends)
    void finish();
    bool counting() const
    {
        return counting_;
    }
    int shown() const
    {
        return shown_;
    }
    float chingPitch() const;    // each step's SF_GENERIC_CHING plays at 22050 + 2 x step Hz

  private:
    int target_ = 0, shown_ = 0, step_ = 0;
    bool counting_ = false;
};

}  // namespace argon::sound
