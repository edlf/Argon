// Argon: RESET / WRONG WAY (craft_race_info 0x5ce70) and the "Shield Low" popup (0x4eb20); race_warnings.hpp
#include "logic/race_warnings.hpp"

#include <algorithm>

namespace argon
{

void RaceWarnings::step(const Step &step, float dt)
{
    countDownReset(step.respawned, dt);
    judgeHeading(step, dt);
}

// 0x5cb30: a respawn sets +8 to 0.5 s, then every step takes dt off it down to 0
void RaceWarnings::countDownReset(bool respawned, float dt)
{
    if (respawned)
    {
        resetLeft = kResetTime;
    }
    resetLeft = std::max(0.f, resetLeft - dt);
}

// 0x5cf64: no judgement while RESET shows or the craft is dead; otherwise forward . normalize(route) at or under -0.4
// adds dt to +0x8e0 (anything above clears it), and over 1 s sets +0xa73
void RaceWarnings::judgeHeading(const Step &step, float dt)
{
    const float routeLength = glm::length(step.route);
    const bool against = routeLength > 0.f && glm::dot(step.forward, step.route / routeLength) <= kWrongWayDot;
    if (resetLeft > 0.f || !step.alive || !against)
    {
        wrongTime = 0;
        headingWrong = false;
        return;
    }
    wrongTime += dt;
    headingWrong = wrongTime > kWrongWayTime;
}

void ShieldLowWarning::step(float shieldFraction, float dt)
{
    due = false;
    const bool low = shieldFraction < kLow;
    crossedDown = low && previous >= kLow;
    crossedUp = !low && previous < kLow;
    if (crossedDown)
    {
        untilNext = kRepeat;
    }
    previous = shieldFraction;
    if (!low)
    {
        return;
    }
    untilNext -= dt;
    if (untilNext >= 0.f)
    {
        return;
    }
    untilNext = kRepeat;
    due = true;
}

}  // namespace argon
