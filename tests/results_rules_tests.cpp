// Argon: tests for the race's results fixed at the player's finish (logic/results_rules.hpp). No window or GPU: run
// with test.bat / test.sh (or ctest in build).
#include "logic/results_rules.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::results::RacerProgress;
using argon::results::RowTimes;

constexpr int kLaps = 3;

bool near(float a, float b)
{
    return std::fabs(a - b) < 1e-3f;
}

RacerProgress racer(int lapsDone, float fraction, float lapTimeSoFar, float finishedLapsTime, float bestLap)
{
    RacerProgress r;
    r.lapsDone = lapsDone;
    r.lapFraction = fraction;
    r.lapTimeSoFar = lapTimeSoFar;
    r.finishedLapsTime = finishedLapsTime;
    r.bestLap = bestLap;
    return r;
}

void testFinishedRacerShowsItsOwnRaceTime()
{
    const RacerProgress finished = racer(kLaps, 0.2f, 12.f, 180.f, 58.f);

    const RowTimes times = argon::results::rowTimes(finished, kLaps);

    CHECK(near(times.raceTime, 180.f));
}

void testFinishedRacerShowsItsBestLap()
{
    const RacerProgress finished = racer(kLaps, 0.2f, 12.f, 180.f, 58.f);

    const RowTimes times = argon::results::rowTimes(finished, kLaps);

    CHECK(near(times.lapTime, 58.f));
}

void testRacingRacerCarriesTheLapsPaceOverTheLapsLeft()
{
    const RacerProgress racing = racer(2, 0.5f, 30.f, 120.f, 59.f);

    const RowTimes times = argon::results::rowTimes(racing, kLaps);

    CHECK(near(times.raceTime, 120.f + 60.f * 1.f));
}

void testRacingRacerKeepsItsBestLap()
{
    const RacerProgress racing = racer(2, 0.5f, 30.f, 120.f, 59.f);

    const RowTimes times = argon::results::rowTimes(racing, kLaps);

    CHECK(near(times.lapTime, 59.f));
}

void testFirstLapRacerProjectsTheWholeRace()
{
    const RacerProgress firstLap = racer(0, 0.25f, 15.f, 0.f, argon::results::kNoTime);

    const RowTimes times = argon::results::rowTimes(firstLap, kLaps);

    CHECK(near(times.raceTime, 60.f * kLaps));
}

void testFirstLapRacerShowsTheLapsPaceAsItsLapTime()
{
    const RacerProgress firstLap = racer(0, 0.25f, 15.f, 0.f, argon::results::kNoTime);

    const RowTimes times = argon::results::rowTimes(firstLap, kLaps);

    CHECK(near(times.lapTime, 60.f));
}

void testRacerWithoutProgressShowsTheTimeSoFar()
{
    const RacerProgress justCrossed = racer(1, 0.f, 0.f, 61.f, 61.f);

    const RowTimes times = argon::results::rowTimes(justCrossed, kLaps);

    CHECK(near(times.raceTime, 61.f));
}

void testSlowerTimeBehindIsKept()
{
    std::vector<float> times = {180.f, 185.f};

    argon::results::keepPlacesInOrder(times);

    CHECK(near(times[1], 185.f));
}

void testFasterEstimateBehindBecomesTheOneAheadsPlusTheGap()
{
    std::vector<float> times = {180.f, 175.f};

    argon::results::keepPlacesInOrder(times);

    CHECK(near(times[1], 180.f + argon::results::kPlaceGap));
}

void testRaisedTimeCarriesDownTheOrder()
{
    std::vector<float> times = {180.f, 175.f, 181.f};

    argon::results::keepPlacesInOrder(times);

    CHECK(near(times[2], 180.f + 2.f * argon::results::kPlaceGap));
}

void testFinishersComeBeforeTheRacersByPlace()
{
    const std::vector<int> order = argon::results::finishOrder({2, 0}, {1, 3}, {});

    CHECK((order == std::vector<int>{2, 0, 1, 3}));
}

void testKnockedOutFillFromTheBackFirstOutLast()
{
    const std::vector<int> order = argon::results::finishOrder({0}, {1}, {3, 2});

    CHECK((order == std::vector<int>{0, 1, 2, 3}));
}

void testRacerKnockedOutAfterJoiningKeepsItsPlace()
{
    const std::vector<int> order = argon::results::finishOrder({0, 1, 2}, {}, {1});

    CHECK((order == std::vector<int>{0, 1, 2}));
}

void testPlaceNumberShownForARacerInTheRace()
{
    CHECK(argon::results::showsPlaceNumber(false, false));
}

void testPlaceNumberHiddenForARacerOutBeforeTheLine()
{
    CHECK(!argon::results::showsPlaceNumber(true, false));
}

void testPlaceNumberShownForARacerOutAfterTheLine()
{
    CHECK(argon::results::showsPlaceNumber(true, true));
}
}  // namespace

int main()
{
    testFinishedRacerShowsItsOwnRaceTime();
    testFinishedRacerShowsItsBestLap();
    testRacingRacerCarriesTheLapsPaceOverTheLapsLeft();
    testRacingRacerKeepsItsBestLap();
    testFirstLapRacerProjectsTheWholeRace();
    testFirstLapRacerShowsTheLapsPaceAsItsLapTime();
    testRacerWithoutProgressShowsTheTimeSoFar();
    testSlowerTimeBehindIsKept();
    testFasterEstimateBehindBecomesTheOneAheadsPlusTheGap();
    testRaisedTimeCarriesDownTheOrder();
    testFinishersComeBeforeTheRacersByPlace();
    testKnockedOutFillFromTheBackFirstOutLast();
    testRacerKnockedOutAfterJoiningKeepsItsPlace();
    testPlaceNumberShownForARacerInTheRace();
    testPlaceNumberHiddenForARacerOutBeforeTheLine();
    testPlaceNumberShownForARacerOutAfterTheLine();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
