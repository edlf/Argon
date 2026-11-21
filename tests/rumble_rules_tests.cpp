// Argon: tests for a craft's controller rumble (logic/rumble_rules.hpp, the game's 0x5b610 / 0x70ac0 / 0xb46d0).
// No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/rumble_rules.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::rumble;

void testMotorWordRoundsToNearest()
{
    CHECK(motorWord(1234.6f) == 1235);
}

void testMotorWordWrapsPast65535()
{
    CHECK(motorWord(85000.f) == 85000 - 65536);
}

void testKnockMotorScalesBy1000()
{
    CHECK(knockMotor(12.5f) == 12500.f);
}

void testKnockMotorStopsAt65000()
{
    CHECK(knockMotor(100.f) == 65000.f);
}

void testCompressionUpTo2Point2AddsNothing()
{
    CHECK(withCompression(1000, 2.2f, 100.f) == 1000);
}

void testCompressionAddsTwentyFiveTimesSpeed()
{
    CHECK(withCompression(1000, 2.5f, 100.f) == 1000 + 2500);
}

void testFadeStartsAt99Steps()
{
    CHECK(knockedOutFade(0.f).first == 99 * 300 && knockedOutFade(0.f).second == 99 * 300);
}

void testFadeEndsAfterThreeSeconds()
{
    CHECK(knockedOutFade(3.f).first == 0 && knockedOutFade(3.f).second == 0);
}

void testFadeHalfway()
{
    CHECK(knockedOutFade(1.5f).first == 50 * 300);
}

void testFirstMotorNeedsMoreThan0x4000()
{
    CHECK(padMotors({0x4000, 0}).first == 0 && padMotors({0x4001, 0}).first == 0x4001);
}

void testSecondMotorNeedsMoreThan0x2000()
{
    CHECK(padMotors({0, 0x2000}).second == 0 && padMotors({0, 0x2001}).second == 0x2001);
}

void testVibrationOffStopsTheFade()
{
    PadState pad;
    pad.vibrationOn = false;
    pad.knockedOut = true;
    CHECK(playerMotors(pad, {40000, 40000}).first == 0);
}

void testKnockedOutPlaysTheFade()
{
    PadState pad;
    pad.knockedOut = true;
    pad.secondsOut = 1.5f;
    CHECK(playerMotors(pad, {40000, 40000}).first == 50 * 300);
}

void testFlyingSendsTheCraftsMotors()
{
    PadState pad;
    pad.flying = true;
    CHECK(playerMotors(pad, {40000, 30000}).second == 30000);
}

void testNotFlyingSendsNothing()
{
    CHECK(playerMotors(PadState{}, {40000, 30000}).first == 0);
}
}  // namespace

int main()
{
    testMotorWordRoundsToNearest();
    testMotorWordWrapsPast65535();
    testKnockMotorScalesBy1000();
    testKnockMotorStopsAt65000();
    testCompressionUpTo2Point2AddsNothing();
    testCompressionAddsTwentyFiveTimesSpeed();
    testFadeStartsAt99Steps();
    testFadeEndsAfterThreeSeconds();
    testFadeHalfway();
    testFirstMotorNeedsMoreThan0x4000();
    testSecondMotorNeedsMoreThan0x2000();
    testVibrationOffStopsTheFade();
    testKnockedOutPlaysTheFade();
    testFlyingSendsTheCraftsMotors();
    testNotFlyingSendsNothing();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
