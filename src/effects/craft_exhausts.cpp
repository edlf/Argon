// Argon: the crafts' jets and flaps (effects\craft_exhausts.hpp; 0x50960, 0x50670).
#include "effects/craft_exhausts.hpp"
#include "craft/craftsim.hpp"

#include <algorithm>

namespace argon
{

CraftExhausts::CraftExhausts()
{
    jets_.fill(kIdleJet);
}

// 0x50960: target = (0.5 x throttle + 0.1 x (100 - throttle)) x 0.01, 1.0 in a turbo (+0x84 / +0x80 / +0x88); the
// size eases at 10 / s, drops at once when the target is below it (unless the target is the idle 0.1), at least 0.1
void CraftExhausts::updateJet(int slot, const JetDrive &drive, float dt)
{
    if (!inRange(slot))
    {
        return;
    }
    float &size = jets_[size_t(slot)];
    const float target = drive.turbo ? 1.f : (0.5f * drive.throttle + (100.f - drive.throttle) * 0.1f) * 0.01f;
    size += (target - size) * 10.f * dt;
    if (target < size && target != kIdleJet)
    {
        size = target;
    }
    size = std::max(size, kIdleJet);
}

void CraftExhausts::updateFlaps(int slot, const GameCraft &craft, float dt)
{
    if (!inRange(slot))
    {
        return;
    }
    if (const FlapRule *rule = flapRuleFor(craft.pilot()))
    {
        flaps_[size_t(slot)].update(*rule, flapControls(craft), dt);
    }
}

float CraftExhausts::jetSize(int slot) const
{
    return inRange(slot) ? jets_[size_t(slot)] : kIdleJet;
}

const FlapState &CraftExhausts::flaps(int slot) const
{
    return flaps_[size_t(std::clamp(slot, 0, kEffectSlots - 1))];
}

bool CraftExhausts::inRange(int slot)
{
    return slot >= 0 && slot < kEffectSlots;
}

}  // namespace argon
