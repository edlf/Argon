// Argon: tests for the speedometer's second needle (hud/speedometer_rules.hpp). Run with test.bat / test.sh.
#include "hud/speedometer_rules.hpp"
#include "hud/speed_readout.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

using namespace argon::hud;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(float a, float b)
{
    return std::fabs(a - b) < 1e-4f;
}

void equalDrawsLeaveTheNeedleOnTheGauge()
{
    const NeedleJitterDraws draws{100, 100};

    const float angle = secondNeedleTarget(50.f, draws);

    CHECK(near(angle, 72.05f));
}

void aFullRaiseDrawAddsATenthOfTheGauge()
{
    const NeedleJitterDraws draws{255, 0};

    const float angle = secondNeedleTarget(50.f, draws);

    CHECK(near(angle, 72.05f + 5.f));
}

void aFullLowerDrawTakesATenthOfTheGauge()
{
    const NeedleJitterDraws draws{0, 255};

    const float angle = secondNeedleTarget(50.f, draws);

    CHECK(near(angle, 72.05f - 5.f));
}

void anIdleThrottleDoesNotJitter()
{
    const NeedleJitterDraws draws{255, 0};

    const float angle = secondNeedleTarget(0.f, draws);

    CHECK(angle == 0.f);
}

void theNeedleStopsAtTheDialsEnd()
{
    const NeedleJitterDraws draws{255, 0};

    const float angle = secondNeedleTarget(100.f, draws);

    CHECK(angle == 144.1f);
}

void theNeedleNeverTurnsBelowZero()
{
    const NeedleJitterDraws draws{0, 255};

    const float angle = secondNeedleTarget(-10.f, draws);

    CHECK(angle == 0.f);
}

void theReadoutIsTheLastFourSpeedsTimesNineTenths()
{
    SpeedReadout readout;
    for (const float speed : {100.f, 10.f, 20.f, 30.f, 40.f})
    {
        readout.push(speed);
    }
    CHECK(std::abs(readout.kmh() - 90.f) < 1e-4f);
}

void goingBackwardsReadsZero()
{
    SpeedReadout readout;
    readout.push(-50.f);
    CHECK(readout.kmh() == 0.f);
}

}  // namespace

int main()
{
    equalDrawsLeaveTheNeedleOnTheGauge();
    aFullRaiseDrawAddsATenthOfTheGauge();
    aFullLowerDrawTakesATenthOfTheGauge();
    anIdleThrottleDoesNotJitter();
    theNeedleStopsAtTheDialsEnd();
    theNeedleNeverTurnsBelowZero();
    theReadoutIsTheLastFourSpeedsTimesNineTenths();
    goingBackwardsReadsZero();
    std::printf("speedometer_rules_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
