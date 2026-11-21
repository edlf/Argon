// Argon: tests for the race start's clocks (logic\race_start.hpp) and the race's clock, time limit and the player's
// knock-out (logic\race_clock.hpp). No window or GPU: run with test.bat (or ctest in build\).
#include "check.hpp"
#include "logic/race_clock.hpp"
#include "logic/race_start.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr float kStep = 1.f / 60.f;
constexpr int kRacers = 6;

// steps the start for `seconds`, counting the countdown starts and the digit sounds
struct Run
{
    int countdownStarts = 0, digitSounds = 0;
};

Run runFor(RaceStart &start, float seconds, int racers = kRacers)
{
    Run run;
    for (float t = 0.f; t < seconds; t += kStep)
    {
        const RaceStart::Step step = start.step(kStep, racers, false);
        run.countdownStarts += step.countdownStarted ? 1 : 0;
        run.digitSounds += step.digits;
    }
    return run;
}

void testIntroShowsEachCraftTwoSecondsFromTheLast()
{
    RaceStart start;
    start.startIntro();
    runFor(start, 2.5f);
    CHECK(start.introCraft(kRacers) == kRacers - 2);
}

void testIntroEndsAfterTheLastCraftIntoTheCountdown()
{
    RaceStart start;
    start.startIntro();
    const Run run = runFor(start, 2.f * kRacers + 0.1f);
    CHECK(!start.introRunning() && run.countdownStarts == 1 && start.countdownClock() >= 0.f);
}

void testSkippedIntroStartsTheCountdown()
{
    RaceStart start;
    start.startIntro();
    const RaceStart::Step step = start.step(kStep, kRacers, true);
    CHECK(step.countdownStarted && !start.introRunning());
}

void testCountdownSoundsEachDigitOnce()
{
    RaceStart start;
    start.startCountdown();
    CHECK(runFor(start, 6.f).digitSounds == RaceStart::kDigits);
}

void testCraftsAreHeldUntilGo()
{
    RaceStart start;
    start.startCountdown();
    runFor(start, 3.9f);
    const bool heldBefore = start.held();
    runFor(start, 0.2f);
    CHECK(heldBefore && !start.held());
}

void testDigitsGoOffPastTenSeconds()
{
    RaceStart start;
    start.startCountdown();
    runFor(start, 9.f);
    const bool shownAtNine = start.countdownShown();
    runFor(start, 1.1f);
    CHECK(shownAtNine && !start.countdownShown());
}

void testCountdownShownAtAMomentHasSoundedItsDigits()
{
    RaceStart start;
    start.showCountdownAt(2.5f);
    CHECK(runFor(start, 3.f).digitSounds == 2);
}

void testStoppedStartHoldsNothing()
{
    RaceStart start;
    start.startIntro();
    start.stop();
    CHECK(!start.held() && !start.countdownShown());
}

void testRaceClockCountsFromItsStart()
{
    RaceClock clock;
    clock.start(30.f);
    CHECK(clock.elapsed(45.f) == 15.f && !clock.pastLimit(45.f));
}

void testRaceClockReachesItsLimit()
{
    RaceClock clock;
    clock.setLimit(60.f);
    clock.start(10.f);
    CHECK(clock.pastLimit(70.f) && !clock.pastLimit(69.f));
}

void testRaceClockStartForgetsTheTimeOut()
{
    RaceClock clock;
    clock.endByTime();
    clock.start(0.f);
    CHECK(!clock.endedByTime());
}

void testKnockOutKeepsWhoAndWhen()
{
    PlayerKnockOut knockOut;
    knockOut.knockedOutBy(3, 100.f);
    CHECK(knockOut.by() == 3 && knockOut.secondsSince(104.f) == 4.f);
}

void testClearedKnockOutIsNobodys()
{
    PlayerKnockOut knockOut;
    knockOut.knockedOutBy(3, 100.f);
    knockOut.clear();
    CHECK(knockOut.by() == PlayerKnockOut::kNobody);
}
}  // namespace

int main()
{
    testIntroShowsEachCraftTwoSecondsFromTheLast();
    testIntroEndsAfterTheLastCraftIntoTheCountdown();
    testSkippedIntroStartsTheCountdown();
    testCountdownSoundsEachDigitOnce();
    testCraftsAreHeldUntilGo();
    testDigitsGoOffPastTenSeconds();
    testCountdownShownAtAMomentHasSoundedItsDigits();
    testStoppedStartHoldsNothing();
    testRaceClockCountsFromItsStart();
    testRaceClockReachesItsLimit();
    testRaceClockStartForgetsTheTimeOut();
    testKnockOutKeepsWhoAndWhen();
    testClearedKnockOutIsNobodys();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
