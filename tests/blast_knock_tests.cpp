// Argon: tests for an explosion's push on a craft (logic/blast_push.hpp, the game's 0x5dee0) and the internal
// camera's shake and contact jolt (logic/camera_shake.hpp, 0x5b610 / 0x5ba10).
// No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/blast_push.hpp"
#include "check.hpp"
#include "logic/camera_shake.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
}

using namespace argon::blast;
using namespace argon::shake;

void testPushNeverPointsDown()
{
    CHECK(upward({1.f, -2.f, 3.f}) == glm::vec3(1.f, 2.f, 3.f));
}

void testPushAlongTheVelocityDoesNotBrake()
{
    CHECK(brakingFraction({0.f, 0.f, 5.f}, {0.f, 0.f, 80.f}) == 0.f);
}

void testPushAcrossTheVelocityBrakesTwoPercentPerUnit()
{
    CHECK(near(brakingFraction({3.f, 0.f, 7.f}, {0.f, 0.f, 80.f}), 0.06f));
}

void testBrakingStopsAtFortyPercent()
{
    CHECK(near(brakingFraction({50.f, 0.f, 0.f}, {0.f, 0.f, 80.f}), 0.4f));
}

void testStillCraftTakesXAsItsHeading()
{
    CHECK(near(brakingFraction({5.f, 0.f, 0.f}, glm::vec3(0.f)), 0.f));
}

void testDoubledBrakingCanReachEightyPercent()
{
    CHECK(near(doubledBraking(0.4f), 0.8f));
}

void testEvenRandShovesRight()
{
    CHECK(near(sidewaysForce(2.f, {1.f, 0.f, 0.f}, kDroneSideScale, 4), {40.f, 0.f, 0.f}));
}

void testOddRandShovesLeft()
{
    CHECK(near(sidewaysForce(2.f, {1.f, 0.f, 0.f}, kPlayerSideScale, 7), {-20.f, 0.f, 0.f}));
}

void testRandomAxisIsZeroAtTheMiddle()
{
    CHECK(randomAxis(0x3fff) == 0.f);
}

void testRandomAxisReachesAboutPlusMinusFiveHundredths()
{
    CHECK(near(randomAxis(0x7fff), 0x4000 / 32767.f * 0.1f) && near(randomAxis(0), -0x3fff / 32767.f * 0.1f));
}

void testFirstFrameArmsTheShake()
{
    CameraShake shake;
    shake.step({1.f / 60.f, 2.f, 5.f, {0.01f, 0.02f, -0.03f}});
    CHECK(near(shake.offset(), {0.02f, 0.04f, -0.06f}));
}

void testShakeFadesUntilTheTimerRunsOut()
{
    CameraShake shake;
    shake.step({0.1f, 1.f, 4.f, {0.05f, 0.f, 0.f}});
    shake.step({0.1f, 1.f, 4.f, {-0.05f, 0.f, 0.f}});
    CHECK(near(shake.offset().x, 0.05f * 0.8f));
}

void testShakeRearmsAfterOneOverHertz()
{
    CameraShake shake;
    shake.step({0.1f, 1.f, 4.f, {0.05f, 0.f, 0.f}});
    shake.step({0.1f, 1.f, 4.f, {0.f, 0.f, 0.f}});
    shake.step({0.1f, 1.f, 4.f, {-0.05f, 0.f, 0.f}});
    CHECK(near(shake.offset().x, -0.05f));
}

void testZeroHertzNeverRearms()
{
    CameraShake shake;
    shake.step({0.1f, 1.f, 0.f, {0.05f, 0.f, 0.f}});
    shake.step({0.1f, 1.f, 0.f, {-0.05f, 0.f, 0.f}});
    CHECK(shake.offset().x > 0.f);
}

void testKnockKicksAHundredthOfItsSize()
{
    CameraShake shake;
    shake.kick({0.05f, -0.02f, 0.01f}, 10.f);
    CHECK(near(shake.offset(), {0.005f, -0.002f, 0.001f}));
}

void testNoKnockNoKick()
{
    CameraShake shake;
    shake.kick({0.05f, 0.f, 0.f}, 0.f);
    CHECK(shake.offset() == glm::vec3(0.f));
}

void testShakeTiltsUpByXAndZAndLiftsTheLookByY()
{
    const ShakenView view = shaken({{0.f, 1.f, 0.f}, {0.f, 0.f, 10.f}}, {0.1f, 0.2f, 0.3f});
    CHECK(near(view.up, {0.1f, 1.f, 0.3f}) && near(view.look, {0.f, 0.2f, 10.f}));
}

void testNoPushNoTurboCooldown()
{
    CHECK(TurboCooldown().over());
}

void testPushHoldsTheTurboForASecond()
{
    TurboCooldown cooldown;
    cooldown.start();
    cooldown.countDown(0.99f);
    CHECK(!cooldown.over());
}

void testTurboCooldownStopsAtZero()
{
    TurboCooldown cooldown;
    cooldown.start();
    cooldown.countDown(0.6f);
    cooldown.countDown(0.6f);
    CHECK(cooldown.over());
}

SubstepContacts substepContacts(const glm::vec3 &momentumChange, float normalDot)
{
    SubstepContacts contacts;
    contacts.add(momentumChange, normalDot);
    return contacts;
}

void testSubstepAveragesTheNormalDot()
{
    SubstepContacts contacts = substepContacts({1.f, 0.f, 0.f}, 0.2f);
    contacts.add({2.f, 0.f, 0.f}, 0.4f);
    CHECK(near(contacts.averageNormalDot(), 0.3f));
}

void testSubstepSumsTheMomentumChange()
{
    SubstepContacts contacts = substepContacts({1.f, 0.f, 0.f}, 0.2f);
    contacts.add({2.f, 1.f, 0.f}, 0.4f);
    CHECK(near(contacts.momentumChange(), {3.f, 1.f, 0.f}));
}

void testHeadOnContactAddsHalfItsMomentumChange()
{
    ContactJolt jolt;
    jolt.addSubstep(substepContacts({0.f, 0.f, -40.f}, 0.9f));
    CHECK(near(jolt.vector(), {0.f, 0.f, -20.f}));
}

void testGlancingContactDoesNotJolt()
{
    ContactJolt jolt;
    jolt.addSubstep(substepContacts({0.f, 0.f, -40.f}, 0.25f));
    CHECK(near(jolt.vector(), glm::vec3(0.f)));
}

void testJoltKeepsNinetyFivePercentAFrame()
{
    ContactJolt jolt;
    jolt.addSubstep(substepContacts({10.f, 0.f, 0.f}, 1.f));
    jolt.beginFrame();
    CHECK(near(jolt.vector(), {4.75f, 0.f, 0.f}));
}

void testOnlyTwentyFourSubstepsAFrameCount()
{
    ContactJolt jolt;
    for (int i = 0; i < 30; ++i)
    {
        jolt.addSubstep(substepContacts({2.f, 0.f, 0.f}, 1.f));
    }
    CHECK(near(jolt.vector(), {24.f, 0.f, 0.f}));
}

void testSubstepWithoutContactsTakesNoRecord()
{
    ContactJolt jolt;
    for (int i = 0; i < 30; ++i)
    {
        jolt.addSubstep(SubstepContacts());
    }
    jolt.addSubstep(substepContacts({2.f, 0.f, 0.f}, 1.f));
    CHECK(near(jolt.vector(), {1.f, 0.f, 0.f}));
}

void testJoltAlongRightTiltsTheUpAgainstIt()
{
    const ShakenView view = jolted({{0.f, 1.f, 0.f}, {0.f, 0.f, 10.f}}, {-20.f, 0.f, 0.f}, CraftAxes());
    CHECK(near(view.up, {0.06f, 1.f, 0.f}) && near(view.look, {0.f, 0.f, 10.f}));
}

void testJoltAlongForwardMovesTheLookAlongUp()
{
    const ShakenView view = jolted({{0.f, 1.f, 0.f}, {0.f, 0.f, 10.f}}, {0.f, 0.f, -5.f}, CraftAxes());
    CHECK(near(view.up, {0.f, 1.f, 0.f}) && near(view.look, {0.f, -0.05f, 10.f}));
}

void testJoltTermsStopAtATenth()
{
    const ShakenView view = jolted({{0.f, 1.f, 0.f}, {0.f, 0.f, 10.f}}, {1000.f, 0.f, 1000.f}, CraftAxes());
    CHECK(near(view.up, {-0.1f, 1.f, 0.f}) && near(view.look, {0.f, 0.1f, 10.f}));
}
}  // namespace

int main()
{
    testPushNeverPointsDown();
    testPushAlongTheVelocityDoesNotBrake();
    testPushAcrossTheVelocityBrakesTwoPercentPerUnit();
    testBrakingStopsAtFortyPercent();
    testStillCraftTakesXAsItsHeading();
    testDoubledBrakingCanReachEightyPercent();
    testEvenRandShovesRight();
    testOddRandShovesLeft();
    testRandomAxisIsZeroAtTheMiddle();
    testRandomAxisReachesAboutPlusMinusFiveHundredths();
    testFirstFrameArmsTheShake();
    testShakeFadesUntilTheTimerRunsOut();
    testShakeRearmsAfterOneOverHertz();
    testZeroHertzNeverRearms();
    testKnockKicksAHundredthOfItsSize();
    testNoKnockNoKick();
    testShakeTiltsUpByXAndZAndLiftsTheLookByY();
    testNoPushNoTurboCooldown();
    testPushHoldsTheTurboForASecond();
    testTurboCooldownStopsAtZero();
    testSubstepAveragesTheNormalDot();
    testSubstepSumsTheMomentumChange();
    testHeadOnContactAddsHalfItsMomentumChange();
    testGlancingContactDoesNotJolt();
    testJoltKeepsNinetyFivePercentAFrame();
    testOnlyTwentyFourSubstepsAFrameCount();
    testSubstepWithoutContactsTakesNoRecord();
    testJoltAlongRightTiltsTheUpAgainstIt();
    testJoltAlongForwardMovesTheLookAlongUp();
    testJoltTermsStopAtATenth();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
