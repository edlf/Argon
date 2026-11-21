// Argon: lock-on and the HUD's crosshairs (logic\lock_on.hpp; 0x6c420, 0x4e7f0).
#include "logic/lock_on.hpp"

#include <algorithm>
#include <utility>

namespace argon::weapons
{
namespace
{
const size_t kMaxLockTargets = 6;
const float kLockRange = 500.f;
const float kLockCone = 0.85f;
const float kNewCrosshairGap = 0.25f, kAppearSeconds = 0.5f, kReleaseSeconds = 0.25f;
enum CrosshairState
{
    kNone = 0,
    kAppearing = 1,
    kHeld = 2,
    kReleased = 3
};

bool following(const Crosshair &crosshair)
{
    return crosshair.state == kAppearing || crosshair.state == kHeld;
}
}  // namespace

std::vector<int> lockCandidates(int slot, const RaceCrafts &crafts)
{
    if (!crafts.flies(slot))
    {
        return {};
    }
    const glm::vec3 forward = glm::normalize(glm::vec3(crafts.frame(slot)[2]));
    const glm::vec3 from = crafts.position(slot);
    std::vector<std::pair<float, int>> near;
    for (int racer = 0; racer < crafts.racerCount(); ++racer)
    {
        const int other = crafts.slotOf(racer);
        if (other == slot || !crafts.targetable(other))
        {
            continue;
        }
        const glm::vec3 offset = crafts.position(other) - from;
        const float distance = glm::length(offset);
        if (distance > 0.f && distance < kLockRange && glm::dot(offset / distance, forward) > kLockCone)
        {
            near.push_back({distance, other});
        }
    }
    std::sort(near.begin(), near.end());
    std::vector<int> targets;
    for (size_t i = 0; i < near.size() && i < kMaxLockTargets; ++i)
    {
        targets.push_back(near[i].second);
    }
    return targets;
}

void PlayerLocks::update(const Lists &lists, float now)
{
    lists_ = lists;
    for (int list = 0; list < kLists; ++list)
    {
        age(list, follow(list, now), now);
    }
}

// the listed targets: a crosshair already on one stays; a new one takes a free crosshair, one per 0.25 s
std::array<bool, PlayerLocks::kCrosshairs> PlayerLocks::follow(int list, float now)
{
    auto &crosshairs = crosshairs_[size_t(list)];
    std::array<bool, kCrosshairs> seen{};
    for (const int target : lists_[size_t(list)])
    {
        bool found = false;
        for (int i = 0; i < kCrosshairs; ++i)
        {
            if (crosshairs[size_t(i)].target == target && following(crosshairs[size_t(i)]))
            {
                seen[size_t(i)] = true;
                found = true;
            }
        }
        if (found || lastNew_[size_t(list)] + kNewCrosshairGap > now)
        {
            continue;
        }
        for (int i = 0; i < kCrosshairs; ++i)
        {
            if (crosshairs[size_t(i)].state == kNone)
            {
                crosshairs[size_t(i)] = Crosshair{kAppearing, target, now};
                lastNew_[size_t(list)] = now;
                seen[size_t(i)] = true;
                break;
            }
        }
    }
    return seen;
}

// the unlisted ones are released; appearing ones are held after 0.5 s, released ones gone after 0.25 s
void PlayerLocks::age(int list, const std::array<bool, kCrosshairs> &seen, float now)
{
    for (int i = 0; i < kCrosshairs; ++i)
    {
        Crosshair &crosshair = crosshairs_[size_t(list)][size_t(i)];
        if (following(crosshair) && !seen[size_t(i)])
        {
            crosshair.state = kReleased;
            crosshair.stateSince = now;
        }
        if (crosshair.state == kAppearing && now - crosshair.stateSince >= kAppearSeconds)
        {
            crosshair.state = kHeld;
        }
        if (crosshair.state == kReleased && now - crosshair.stateSince >= kReleaseSeconds)
        {
            crosshair = Crosshair{};
        }
    }
}

const PlayerLocks::Lists &PlayerLocks::lists() const
{
    return lists_;
}

const PlayerLocks::Crosshairs &PlayerLocks::crosshairs() const
{
    return crosshairs_;
}

bool PlayerLocks::holds(int slot) const
{
    for (const std::vector<int> &list : lists_)
    {
        if (std::find(list.begin(), list.end(), slot) != list.end())
        {
            return true;
        }
    }
    return false;
}

}  // namespace argon::weapons
