// Argon: the AI's trigger (logic\ai_trigger.hpp; 0x6e9e0, 0x6e8d0 / 0x6e950, 0x6e660, 0x6e380, 0x6e1e0 / 0x6e2b0).
#include "logic/ai_trigger.hpp"

#include <algorithm>
#include <cmath>

namespace argon::weapons
{
namespace
{
const float kLineHalfWidth = 5.f;         // 0x1a3554: |offset along the right axis| < 5
const float kLineRange = 500.f;           // 0x1a359c
const float kShieldBelowPercent = 40.f;   // 0x1a3604
const float kBehindPlayerLimit = -5.f;    // +0x1808 > -0.05 laps
const int kFirstShieldOnLockSpeed = 2;
const int kPlayerSlot = 0;
const float kStepSeconds = 1.f / 60.f;

// 0x6e660: a lock-on weapon is wanted while it has a target
bool wantsLocked(const ArmedColour &colour, const AiSituation &situation)
{
    return colour.armed && colour.locksOn && !colour.inFlight && situation.hasTargets;
}

// 0x6e380: a weapon that neither locks nor shields is wanted with a target in line
bool wantsInLine(const ArmedColour &colour, const AiSituation &situation)
{
    return situation.targetInLine && colour.armed && !colour.locksOn && !colour.shield;
}

bool wants(const ArmedColour &colour, const AiSituation &situation)
{
    const bool aimed = situation.engages && (wantsLocked(colour, situation) || wantsInLine(colour, situation));
    return aimed || (situation.threatened && colour.shield);
}

// the clock counts while the colour is wanted (else it is 0) and fires once past the delay
bool clockFires(float &clock, float delay, bool wanted)
{
    if (!wanted)
    {
        clock = 0.f;
        return false;
    }
    clock += kStepSeconds;
    return clock > delay;
}
}  // namespace

bool aiEngages(bool nemesis, std::optional<float> percentAhead)
{
    return nemesis || (percentAhead && *percentAhead > kBehindPlayerLimit);
}

std::vector<int> aiTargets(const std::vector<int> &listed, bool nemesis)
{
    if (!nemesis)
    {
        return listed;
    }
    const bool playerListed = std::find(listed.begin(), listed.end(), kPlayerSlot) != listed.end();
    return playerListed ? std::vector<int>{kPlayerSlot} : std::vector<int>{};
}

bool targetInLine(int slot, const std::vector<int> &targets, const RaceCrafts &crafts)
{
    const glm::mat4 frame = crafts.frame(slot);
    const glm::vec3 from(frame[3]), right = glm::normalize(glm::vec3(frame[0]));
    for (const int target : targets)
    {
        const glm::vec3 offset = crafts.position(target) - from;
        if (std::abs(glm::dot(offset, right)) < kLineHalfWidth && glm::length(offset) < kLineRange)
        {
            return true;
        }
    }
    return false;
}

bool aiThreatened(const AiThreat &threat)
{
    const bool shieldsOnLock = threat.speedClass >= kFirstShieldOnLockSpeed || threat.nemesis;
    return (shieldsOnLock && threat.lockedOn) || threat.shieldPercent < kShieldBelowPercent;
}

AiWants aiWants(const AiSituation &situation, const AiArmament &armament)
{
    AiWants wanted{wants(armament.red, situation), wants(armament.blue, situation), wants(armament.yellow, situation)};
    if (situation.holdsForFinishedPlayer)
    {
        wanted.red = wanted.blue = false;
    }
    return wanted;
}

AiFire advanceTrigger(AiTrigger &trigger, const AiWants &wanted)
{
    const bool red = clockFires(trigger.redClock, trigger.redDelay, wanted.red);
    const bool blue = clockFires(trigger.blueClock, trigger.blueDelay, wanted.blue);
    return AiFire{red, blue, wanted.yellow};
}

float aiFireDelay(int speedClass, ViewerRand &rand)
{
    const int speed = std::clamp(speedClass, 0, 4);
    const float roll = float(rand.next() % 100u);
    if (speed <= 1)
    {
        return 2.5f + roll * 0.02f;
    }
    const float base[5] = {0.f, 0.f, 2.4f, 2.2f, 2.0f};
    return base[speed] + roll * 0.01f;
}

}  // namespace argon::weapons
