// Argon: the weapons' travel loops as the sound manager keeps them (docs\audio\ships.md "Ships"): each launch queues one
// of 10 entries (+0x133c, 0xc9d10) that starts 0.5 s later (+0x9b8) in one of 10 voice slots (+0x1274, 0xc9dc0);
// a projectile's or shield's update moves its voices (0xc9f80), the projectile's end stops them (0xc9f00) and the
// race's end stops them all (0xc9c30). Keys and times only: the voices are the caller's handles.
#pragma once
#include <array>
#include <optional>
#include <vector>

#include "audio/sound_ids.hpp"

namespace argon::sound
{

// 0xc9d10's key: the craft's slot, its weapon (the weapon object +0x14) and the projectile
struct TravelKey
{
    int craft = 0, weapon = 0, projectile = 0;
};
bool operator==(const TravelKey &a, const TravelKey &b);

// a launch's loop: the weapon.nwp class (+8) picks the sound
struct TravelLaunch
{
    TravelKey key;
    int weaponClass = 0;
};

// 0xc9dc0: class 0 / 1 / 2 -> WEAPONTRAVELPROJECTILE / LASER / RADIAL, any other SPECIAL
ShipSound travelSound(int weaponClass);

// 0xc9f80 on a radial loop (class 2): MINDIST = radius + 200, MAXDIST = (MAXDIST - MINDIST of the sound) + radius + 500
struct TravelDistances
{
    float minDist = 0.f, maxDist = 0.f;
};
TravelDistances radialTravelDistances(float sphereRadius, float soundMinDist, float soundMaxDist);

class TravelLoops
{
  public:
    static constexpr int kSlots = 10;
    static constexpr float kStartDelay = 0.5f;   // +0x9b8, set by the defaults 0xc96e0 only
    static constexpr int kNoVoice = -1;

    // 0xc9d10: due kStartDelay after now; with every entry taken the earliest queued starts now (returned) and gives
    // the new one its entry
    std::optional<TravelLaunch> queue(const TravelLaunch &launch, float now);
    std::vector<TravelLaunch> takeDue(float now);   // 0xc9cb0, each sound frame
    // 0xc9dc0: a started loop's voice (kNoVoice when it did not start) takes a free slot, else the earliest started
    // one's; returns the voice it replaced (to stop), kNoVoice when none
    int holdVoice(const TravelKey &key, int voice, float now);
    std::vector<int> voicesOf(const TravelKey &key) const;   // 0xc9f80
    std::vector<int> stop(const TravelKey &key);             // 0xc9f00: its voices to stop; its queued entry goes too
    std::vector<int> stopAll();                              // 0xc9c30

  private:
    struct Queued
    {
        TravelLaunch launch;
        float queuedAt = 0.f, due = 0.f;
        bool taken = false;
    };
    struct Held
    {
        TravelKey key;
        int voice = kNoVoice;
        float startedAt = 0.f;
    };
    int freeEntry() const;
    int earliestQueued() const;
    int voiceSlotFor() const;

    std::array<Queued, kSlots> queue_{};
    std::array<Held, kSlots> held_{};
};

}  // namespace argon::sound
