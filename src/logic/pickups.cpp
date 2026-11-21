// Argon: the track's pickups (logic\pickups.hpp; pickups_collide 0x8fcd0, the loader 0x78380).
#include "logic/pickups.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{
namespace
{
const float kReach = 14.f;          // 0x6d000 passes 14 as the ship's radius
const float kRespawnSeconds = 0.4f;

// the per-type lift the pickup loader (0x78380, floats 0x1a3970..0x1a3980) adds to the pup.txt Y
float liftOf(PickupType type)
{
    static const float lift[5] = {3.84f, 3.08f, 3.93f, 5.97f, 5.74f};
    return lift[int(type)];
}

// the box test then the sphere, as 0x8fcd0
bool withinReach(const glm::vec3 &ship, const glm::vec3 &pickup)
{
    const glm::vec3 offset = ship - pickup;
    const bool inBox = std::abs(offset.x) < kReach && std::abs(offset.y) < kReach && std::abs(offset.z) < kReach;
    return inBox && glm::length(offset) < kReach;
}
}  // namespace

PickupType pickupTypeOf(const std::string &name)
{
    if (name == "POWERUPRED")
    {
        return PickupType::Red;
    }
    if (name == "POWERUPYELLOW")
    {
        return PickupType::Yellow;
    }
    if (name == "MEGAUP")
    {
        return PickupType::MegaUp;
    }
    if (name == "POWERCHIP")
    {
        return PickupType::PowerChip;
    }
    return PickupType::Blue;
}

uint32_t pickupBurstColour(PickupType type)
{
    static const uint32_t colour[4] = {0xff3030ffu, 0xffff3030u, 0xffffff30u, 0u};
    return type == PickupType::PowerChip ? 0u : colour[int(type)];
}

void Pickups::place(const std::vector<Pickup> &spots)
{
    spots_.clear();
    for (const Pickup &spot : spots)
    {
        const PickupType type = pickupTypeOf(spot.type);
        spots_.push_back(Spot{spot.type, type, glm::vec3(spot.pos[0], spot.pos[1] + liftOf(type), spot.pos[2])});
    }
    lap_ = 0;
}

void Pickups::startLap(int lap)
{
    if (lap == lap_)
    {
        return;
    }
    lap_ = lap;
    for (Spot &spot : spots_)
    {
        spot.takenThisLap = false;
    }
}

std::vector<PickupChange> Pickups::stepPlayer(const glm::vec3 &ship, float now)
{
    std::vector<PickupChange> changes;
    for (size_t i = 0; i < spots_.size(); ++i)
    {
        Spot &spot = spots_[i];
        if (!spot.onTrack)
        {
            if (spot.type != PickupType::PowerChip && spot.takenAt + kRespawnSeconds < now)
            {
                spot.onTrack = true;
                changes.push_back(changeOf(PickupChange::Kind::Back, i));
            }
            continue;
        }
        if (!withinReach(ship, spot.pos) || spot.takenThisLap)
        {
            continue;
        }
        spot.takenThisLap = true;
        changes.push_back(take(i, now));
    }
    return changes;
}

std::vector<PickupChange> Pickups::takeByAi(const glm::vec3 &ship, float now)
{
    std::vector<PickupChange> changes;
    for (size_t i = 0; i < spots_.size(); ++i)
    {
        const Spot &spot = spots_[i];
        if (spot.onTrack && spot.type != PickupType::PowerChip && withinReach(ship, spot.pos))
        {
            changes.push_back(take(i, now));
        }
    }
    return changes;
}

std::optional<PickupChange> Pickups::takeReplayed(int index, float now)
{
    if (!known(index))
    {
        return std::nullopt;
    }
    return take(size_t(index), now);
}

std::optional<PickupChange> Pickups::bringBack(int index)
{
    if (!known(index))
    {
        return std::nullopt;
    }
    spots_[size_t(index)].onTrack = true;
    return changeOf(PickupChange::Kind::Back, size_t(index));
}

size_t Pickups::count() const
{
    return spots_.size();
}

bool Pickups::onTrack(size_t index) const
{
    return spots_[index].onTrack;
}

bool Pickups::takenThisLap(size_t index) const
{
    return spots_[index].takenThisLap;
}

int Pickups::takenThisLapCount() const
{
    int taken = 0;
    for (const Spot &spot : spots_)
    {
        taken += spot.takenThisLap;
    }
    return taken;
}

const std::string &Pickups::typeName(size_t index) const
{
    return spots_[index].typeName;
}

glm::vec3 Pickups::position(size_t index) const
{
    return spots_[index].pos;
}

PickupChange Pickups::changeOf(PickupChange::Kind kind, size_t index) const
{
    return PickupChange{kind, int(index), spots_[index].type, spots_[index].pos};
}

PickupChange Pickups::take(size_t index, float now)
{
    spots_[index].onTrack = false;
    spots_[index].takenAt = now;
    return changeOf(PickupChange::Kind::Taken, index);
}

bool Pickups::known(int index) const
{
    return index >= 0 && index < int(spots_.size());
}

}  // namespace argon
