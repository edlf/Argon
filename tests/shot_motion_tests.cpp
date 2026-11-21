// Argon: tests for a projectile's frame at launch and in flight (logic/shot_motion.hpp: the launch 0x64e20, the
// update 0x66d30). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/shot_motion.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::weapons;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

// ---- the look-at frame (0xb4e70)
void testTheFrameLooksAlongTheDirection()
{
    CHECK(near(frameAlong(glm::vec3(0.f, 0.f, 5.f), ShotFrame{}).forward, glm::vec3(0.f, 0.f, 1.f)));
}

void testTheFramesRightIsWorldUpCrossForward()
{
    CHECK(near(frameAlong(glm::vec3(0.f, 0.f, 1.f), ShotFrame{}).right, glm::vec3(1.f, 0.f, 0.f)));
}

void testTheFramesUpIsForwardCrossRight()
{
    CHECK(near(frameAlong(glm::vec3(0.f, 0.f, 1.f), ShotFrame{}).up, glm::vec3(0.f, 1.f, 0.f)));
}

void testAStraightUpDirectionKeepsTheFrame()
{
    ShotFrame current;
    current.forward = glm::vec3(1.f, 0.f, 0.f);
    CHECK(near(frameAlong(glm::vec3(0.f, 3.f, 0.f), current).forward, glm::vec3(1.f, 0.f, 0.f)));
}

void testAimingLooksFromTheBoneToTheTarget()
{
    const ShotFrame f = frameAimedAt(glm::vec3(1.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 9.f), ShotFrame{});
    CHECK(near(f.forward, glm::vec3(0.f, 0.f, 1.f)));
}

// ---- the pitched launch (0x65b0e)
void testAPitchedLaunchRaisesTheNose()
{
    const ShotFrame f = frameRaisedFrom(ShotFrame{}, 30.f);
    CHECK(near(f.forward, glm::vec3(0.f, 0.5f, std::sqrt(3.f) * 0.5f)));
}

void testAPitchedLaunchKeepsTheRightAxis()
{
    CHECK(near(frameRaisedFrom(ShotFrame{}, 30.f).right, glm::vec3(1.f, 0.f, 0.f)));
}

void testThePitchIsClampedTo90()
{
    CHECK(near(frameRaisedFrom(ShotFrame{}, 120.f).forward, glm::vec3(0.f, 1.f, 0.f)));
}

void testANegativePitchIsClampedToZero()
{
    CHECK(near(frameRaisedFrom(ShotFrame{}, -10.f).forward, glm::vec3(0.f, 0.f, 1.f)));
}

void testThePitchTurnsAboutTheCraftsOwnRight()
{
    ShotFrame craft;
    craft.right = glm::vec3(0.f, 0.f, -1.f);
    craft.forward = glm::vec3(1.f, 0.f, 0.f);
    CHECK(near(frameRaisedFrom(craft, 90.f).forward, glm::vec3(0.f, 1.f, 0.f)));
}

// ---- turning along the path (0x67add)
void testAFastShotTurnsAlongItsPath()
{
    CHECK(turnsAlongPath(300.f, 200.f, false, false));
}

void testASlowShotKeepsItsFrame()
{
    CHECK(!turnsAlongPath(150.f, 200.f, false, false));
}

void testABouncingRecordAlwaysTurns()
{
    CHECK(turnsAlongPath(10.f, 200.f, true, false));
}

void testNoTurnOnTheStepItBounced()
{
    CHECK(!turnsAlongPath(300.f, 200.f, true, true));
}

void testUnhuggedThePathIsTheMove()
{
    CHECK(near(pathDirection(glm::vec3(0.f, -1.f, 10.f), nullptr), glm::vec3(0.f, -1.f, 10.f)));
}

void testHuggedThePathLosesItsPartAlongTheNormal()
{
    const glm::vec3 normal(0.f, 2.f, 0.f);
    CHECK(near(pathDirection(glm::vec3(0.f, -1.f, 1.f), &normal), glm::vec3(0.f, 0.f, std::sqrt(0.5f))));
}

// ---- the speed (0x66f33)
void testTheSpeedGrowsByTheAcceleration()
{
    CHECK(near(acceleratedSpeed(100.f, 600.f, 1000.f, 0.5f, false), 400.f));
}

void testTheSpeedStopsAtItsMaximum()
{
    CHECK(near(acceleratedSpeed(900.f, 600.f, 1000.f, 0.5f, false), 1000.f));
}

void testAHeldSpeedDoesNotGrow()
{
    CHECK(near(acceleratedSpeed(100.f, 600.f, 1000.f, 0.5f, true), 100.f));
}

// ---- the inherited velocity (0x64fae, 0x67c7c)
void testTheInheritedVelocityLeavesOutTheForwardPart()
{
    const glm::vec3 inherited = inheritedVelocity({3.f, 4.f, 50.f}, 50.f, {0.f, 0.f, 1.f}, 100.f);

    CHECK(near(inherited.x, 3.f) && near(inherited.y, 4.f) && near(inherited.z, 0.f));
}

void testTheInheritedVelocityIsItsPercent()
{
    const glm::vec3 inherited = inheritedVelocity({10.f, 0.f, 0.f}, 0.f, {0.f, 0.f, 1.f}, 25.f);

    CHECK(near(inherited.x, 2.5f));
}

void testNoInheritanceGivesNothing()
{
    const glm::vec3 inherited = inheritedVelocity({10.f, 20.f, 30.f}, 30.f, {0.f, 0.f, 1.f}, 0.f);

    CHECK(near(glm::length(inherited), 0.f));
}

void testTheInheritedVelocityDecaysByFortyPercentASecond()
{
    const glm::vec3 decayed = decayedInherited({10.f, 0.f, 0.f}, 0.5f);

    CHECK(near(decayed.x, 8.f));
}
}  // namespace

int main()
{
    testTheFrameLooksAlongTheDirection();
    testTheFramesRightIsWorldUpCrossForward();
    testTheFramesUpIsForwardCrossRight();
    testAStraightUpDirectionKeepsTheFrame();
    testAimingLooksFromTheBoneToTheTarget();
    testAPitchedLaunchRaisesTheNose();
    testAPitchedLaunchKeepsTheRightAxis();
    testThePitchIsClampedTo90();
    testANegativePitchIsClampedToZero();
    testThePitchTurnsAboutTheCraftsOwnRight();
    testAFastShotTurnsAlongItsPath();
    testASlowShotKeepsItsFrame();
    testABouncingRecordAlwaysTurns();
    testNoTurnOnTheStepItBounced();
    testUnhuggedThePathIsTheMove();
    testHuggedThePathLosesItsPartAlongTheNormal();
    testTheSpeedGrowsByTheAcceleration();
    testTheSpeedStopsAtItsMaximum();
    testAHeldSpeedDoesNotGrow();
    testTheInheritedVelocityLeavesOutTheForwardPart();
    testTheInheritedVelocityIsItsPercent();
    testNoInheritanceGivesNothing();
    testTheInheritedVelocityDecaysByFortyPercentASecond();
    std::printf("shot_motion_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
