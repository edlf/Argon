// Argon: tests for lock-on and the HUD's crosshairs (logic/lock_on.hpp: 0x6c420, 0x4e7f0) and the AI's trigger
// (logic/ai_trigger.hpp: 0x6e9e0, 0x6e660, 0x6e380, 0x6e1e0 / 0x6e2b0), on a race of crafts given through
// logic/race_crafts.hpp. No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/ai_trigger.hpp"
#include "logic/lock_on.hpp"
#include "check.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::weapons;

// crafts in slots 0..n - 1 at the given places, all facing +z
class FakeCrafts final : public RaceCrafts
{
  public:
    explicit FakeCrafts(std::vector<glm::vec3> places) : places_(std::move(places))
    {
    }
    void knockOut(int slot)
    {
        out_.push_back(slot);
    }
    int racerCount() const override
    {
        return int(places_.size());
    }
    int slotOf(int racer) const override
    {
        return racer;
    }
    bool flies(int slot) const override
    {
        return slot >= 0 && slot < racerCount();
    }
    glm::vec3 position(int slot) const override
    {
        return places_[size_t(slot)];
    }
    glm::mat4 frame(int slot) const override
    {
        glm::mat4 frame(1.f);
        frame[3] = glm::vec4(places_[size_t(slot)], 1.f);
        return frame;
    }
    bool targetable(int slot) const override
    {
        return std::find(out_.begin(), out_.end(), slot) == out_.end();
    }

  private:
    std::vector<glm::vec3> places_;
    std::vector<int> out_;
};

PlayerLocks::Lists redListOf(std::vector<int> targets)
{
    PlayerLocks::Lists lists;
    lists[0] = std::move(targets);
    return lists;
}

void testCraftsAheadAreCandidatesNearestFirst()
{
    const FakeCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 200.f), glm::vec3(0.f, 0.f, 100.f)});
    CHECK(lockCandidates(0, crafts) == std::vector<int>({2, 1}));
}

void testACraftBehindIsNoCandidate()
{
    const FakeCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, -50.f)});
    CHECK(lockCandidates(0, crafts).empty());
}

void testACraftBeyond500mIsNoCandidate()
{
    const FakeCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 501.f)});
    CHECK(lockCandidates(0, crafts).empty());
}

void testACraftOutsideTheConeIsNoCandidate()
{
    const FakeCrafts crafts({glm::vec3(0.f), glm::vec3(100.f, 0.f, 100.f)});   // 0.707 < 0.85
    CHECK(lockCandidates(0, crafts).empty());
}

void testAKnockedOutCraftIsNoCandidate()
{
    FakeCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 100.f)});
    crafts.knockOut(1);
    CHECK(lockCandidates(0, crafts).empty());
}

void testAtMostSixCandidates()
{
    std::vector<glm::vec3> places{glm::vec3(0.f)};
    for (int i = 1; i <= 8; ++i)
    {
        places.push_back(glm::vec3(0.f, 0.f, 10.f * float(i)));
    }
    CHECK(lockCandidates(0, FakeCrafts(places)).size() == 6);
}

void testAnEmptySlotHasNoCandidates()
{
    CHECK(lockCandidates(5, FakeCrafts({glm::vec3(0.f)})).empty());
}

void testAListedTargetGetsAnAppearingCrosshair()
{
    PlayerLocks locks;
    locks.update(redListOf({3}), 1.f);
    const Crosshair &first = locks.crosshairs()[0][0];
    CHECK(first.state == 1 && first.target == 3);
}

void testOnlyOneNewCrosshairPerQuarterSecond()
{
    PlayerLocks locks;
    locks.update(redListOf({3, 4}), 1.f);
    CHECK(locks.crosshairs()[0][1].state == 0);
}

void testAnAppearingCrosshairIsHeldAfterHalfASecond()
{
    PlayerLocks locks;
    locks.update(redListOf({3}), 1.f);
    locks.update(redListOf({3}), 1.5f);
    CHECK(locks.crosshairs()[0][0].state == 2);
}

void testAnUnlistedTargetsCrosshairIsReleasedThenGone()
{
    PlayerLocks locks;
    locks.update(redListOf({3}), 1.f);
    locks.update(redListOf({}), 1.1f);
    const bool released = locks.crosshairs()[0][0].state == 3;
    locks.update(redListOf({}), 1.4f);
    CHECK(released && locks.crosshairs()[0][0].state == 0);
}

void testTheLocksSayWhichCraftTheyHold()
{
    PlayerLocks locks;
    locks.update(redListOf({3}), 1.f);
    CHECK(locks.holds(3) && !locks.holds(4));
}

ArmedColour armedWith(bool locksOn, bool shield)
{
    ArmedColour colour;
    colour.armed = true;
    colour.locksOn = locksOn;
    colour.shield = shield;
    return colour;
}

void testANemesisTargetsOnlyTheListedPlayer()
{
    CHECK(aiTargets({2, 0, 3}, true) == std::vector<int>({0}));
}

void testANemesisWithoutThePlayerListedHasNoTargets()
{
    CHECK(aiTargets({2, 3}, true).empty());
}

void testADroneMoreThan5PercentBehindDoesNotEngage()
{
    CHECK(!aiEngages(false, -6.f) && aiEngages(false, -4.f) && aiEngages(true, std::nullopt));
}

void testATargetWithin5mOfTheLineAheadIsInLine()
{
    const FakeCrafts crafts({glm::vec3(0.f), glm::vec3(4.f, 0.f, 300.f)});
    CHECK(targetInLine(0, {1}, crafts));
}

void testATargetWideOfTheLineIsNotInLine()
{
    const FakeCrafts crafts({glm::vec3(0.f), glm::vec3(6.f, 0.f, 300.f)});
    CHECK(!targetInLine(0, {1}, crafts));
}

void testShieldsGoUpWhenLockedOnFromSpeed3OrBelow40Percent()
{
    CHECK(aiThreatened(AiThreat{2, false, true, 100.f}) && !aiThreatened(AiThreat{1, false, true, 100.f}) &&
          aiThreatened(AiThreat{0, false, false, 39.f}));
}

void testALockOnWeaponIsWantedWithATarget()
{
    AiSituation situation;
    situation.engages = situation.hasTargets = true;
    AiArmament armament;
    armament.red = armedWith(true, false);
    CHECK(aiWants(situation, armament).red);
}

void testALockOnWeaponWithItsShotOutIsNotWanted()
{
    AiSituation situation;
    situation.engages = situation.hasTargets = true;
    AiArmament armament;
    armament.red = armedWith(true, false);
    armament.red.inFlight = true;
    CHECK(!aiWants(situation, armament).red);
}

void testAShieldIsWantedOnlyWhenThreatened()
{
    AiSituation situation;
    situation.engages = situation.hasTargets = situation.targetInLine = true;
    AiArmament armament;
    armament.blue = armedWith(false, true);
    const bool calm = aiWants(situation, armament).blue;
    situation.threatened = true;
    CHECK(!calm && aiWants(situation, armament).blue);
}

void testAFinishedListedPlayerHoldsRedAndBlue()
{
    AiSituation situation;
    situation.engages = situation.targetInLine = situation.holdsForFinishedPlayer = true;
    AiArmament armament;
    armament.red = armament.blue = armament.yellow = armedWith(false, false);
    const AiWants wanted = aiWants(situation, armament);
    CHECK(!wanted.red && !wanted.blue && wanted.yellow);
}

void testRedFiresOnceItsClockPassesTheDelay()
{
    AiTrigger trigger;
    trigger.redDelay = 0.05f;
    AiWants wanted;
    wanted.red = true;
    const bool first = advanceTrigger(trigger, wanted).red;
    advanceTrigger(trigger, wanted);
    CHECK(!first && advanceTrigger(trigger, wanted).red);   // 3 / 60 s > 0.05 s
}

void testAnUnwantedColoursClockStartsAgain()
{
    AiTrigger trigger;
    trigger.redClock = 1.f;
    advanceTrigger(trigger, AiWants{});
    CHECK(trigger.redClock == 0.f);
}

void testTheFireDelayStaysInItsSpeedClassRange()
{
    // speed 1 / 2 (classes 0 / 1): 2.5 .. 4.48 s; speed 3 / 4 / 5: 2.4 / 2.2 / 2.0 plus 0 .. 0.99 s
    const float low[5] = {2.5f, 2.5f, 2.4f, 2.2f, 2.0f}, high[5] = {4.48f, 4.48f, 3.39f, 3.19f, 2.99f};
    argon::ViewerRand rand;
    bool inRange = true;
    for (int roll = 0; roll < 200; ++roll)
    {
        const int speed = roll % 5;
        const float delay = aiFireDelay(speed, rand);
        inRange = inRange && delay >= low[speed] && delay <= high[speed] + 1e-4f;
    }
    CHECK(inRange);
}
}  // namespace

int main()
{
    testCraftsAheadAreCandidatesNearestFirst();
    testACraftBehindIsNoCandidate();
    testACraftBeyond500mIsNoCandidate();
    testACraftOutsideTheConeIsNoCandidate();
    testAKnockedOutCraftIsNoCandidate();
    testAtMostSixCandidates();
    testAnEmptySlotHasNoCandidates();
    testAListedTargetGetsAnAppearingCrosshair();
    testOnlyOneNewCrosshairPerQuarterSecond();
    testAnAppearingCrosshairIsHeldAfterHalfASecond();
    testAnUnlistedTargetsCrosshairIsReleasedThenGone();
    testTheLocksSayWhichCraftTheyHold();
    testANemesisTargetsOnlyTheListedPlayer();
    testANemesisWithoutThePlayerListedHasNoTargets();
    testADroneMoreThan5PercentBehindDoesNotEngage();
    testATargetWithin5mOfTheLineAheadIsInLine();
    testATargetWideOfTheLineIsNotInLine();
    testShieldsGoUpWhenLockedOnFromSpeed3OrBelow40Percent();
    testALockOnWeaponIsWantedWithATarget();
    testALockOnWeaponWithItsShotOutIsNotWanted();
    testAShieldIsWantedOnlyWhenThreatened();
    testAFinishedListedPlayerHoldsRedAndBlue();
    testRedFiresOnceItsClockPassesTheDelay();
    testAnUnwantedColoursClockStartsAgain();
    testTheFireDelayStaysInItsSpeedClassRange();
    std::printf("lock_on_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
