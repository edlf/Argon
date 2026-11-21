// Argon: the weapons' travel loops, see travel_loops.hpp.
#include "logic/travel_loops.hpp"

namespace argon::sound
{
namespace
{
constexpr int kProjectileClass = 0, kBeamClass = 1, kShieldClass = 2;
constexpr float kRadialMinDistAdd = 200.f;   // 0x1a35a0
constexpr float kRadialMaxDistAdd = 500.f;   // 0x1a359c
}  // namespace

bool operator==(const TravelKey &first, const TravelKey &second)
{
    return first.craft == second.craft && first.weapon == second.weapon && first.projectile == second.projectile;
}

ShipSound travelSound(int weaponClass)
{
    switch (weaponClass)
    {
    case kProjectileClass:
        return ShipSound::TravelProjectile;
    case kBeamClass:
        return ShipSound::TravelLaser;
    case kShieldClass:
        return ShipSound::TravelRadial;
    default:
        return ShipSound::TravelSpecial;
    }
}

TravelDistances radialTravelDistances(float sphereRadius, float soundMinDist, float soundMaxDist)
{
    return {sphereRadius + kRadialMinDistAdd, soundMaxDist - soundMinDist + sphereRadius + kRadialMaxDistAdd};
}

std::optional<TravelLaunch> TravelLoops::queue(const TravelLaunch &launch, float now)
{
    std::optional<TravelLaunch> startedEarly;
    int entry = freeEntry();
    if (entry < 0)
    {
        entry = earliestQueued();
        startedEarly = queue_[size_t(entry)].launch;
    }
    queue_[size_t(entry)] = Queued{launch, now, now + kStartDelay, true};
    return startedEarly;
}

std::vector<TravelLaunch> TravelLoops::takeDue(float now)
{
    std::vector<TravelLaunch> due;
    for (Queued &queued : queue_)
    {
        if (queued.taken && queued.due <= now)
        {
            due.push_back(queued.launch);
            queued.taken = false;
        }
    }
    return due;
}

int TravelLoops::holdVoice(const TravelKey &key, int voice, float now)
{
    const int slot = voiceSlotFor();
    const int replaced = held_[size_t(slot)].voice;
    held_[size_t(slot)] = Held{key, voice, now};
    return replaced;
}

std::vector<int> TravelLoops::voicesOf(const TravelKey &key) const
{
    std::vector<int> voices;
    for (const Held &held : held_)
    {
        if (held.voice != kNoVoice && held.key == key)
        {
            voices.push_back(held.voice);
        }
    }
    return voices;
}

std::vector<int> TravelLoops::stop(const TravelKey &key)
{
    std::vector<int> voices = voicesOf(key);
    for (Held &held : held_)
    {
        if (held.key == key)
        {
            held.voice = kNoVoice;
        }
    }
    for (Queued &queued : queue_)
    {
        if (queued.launch.key == key)
        {
            queued.taken = false;
        }
    }
    return voices;
}

std::vector<int> TravelLoops::stopAll()
{
    std::vector<int> voices;
    for (Held &held : held_)
    {
        if (held.voice != kNoVoice)
        {
            voices.push_back(held.voice);
        }
        held.voice = kNoVoice;
    }
    for (Queued &queued : queue_)
    {
        queued.taken = false;
    }
    return voices;
}

int TravelLoops::freeEntry() const
{
    for (int i = 0; i < kSlots; ++i)
    {
        if (!queue_[size_t(i)].taken)
        {
            return i;
        }
    }
    return -1;
}

int TravelLoops::earliestQueued() const
{
    int earliest = 0;
    for (int i = 1; i < kSlots; ++i)
    {
        if (queue_[size_t(i)].queuedAt < queue_[size_t(earliest)].queuedAt)
        {
            earliest = i;
        }
    }
    return earliest;
}

// a slot without a voice, else the earliest started
int TravelLoops::voiceSlotFor() const
{
    int earliest = 0;
    for (int i = 0; i < kSlots; ++i)
    {
        if (held_[size_t(i)].voice == kNoVoice)
        {
            return i;
        }
        if (held_[size_t(i)].startedAt < held_[size_t(earliest)].startedAt)
        {
            earliest = i;
        }
    }
    return earliest;
}

}  // namespace argon::sound
