// Argon: tests for a race's points and score (logic\race_points.hpp, logic\race_score.hpp): the HUD's popups and
// count-up, the config files' values and the SCOREMENU sum. No window or GPU: run with test.bat (or ctest in build\).
#include "check.hpp"
#include "logic/race_points.hpp"
#include "logic/race_score.hpp"

#include <cstdio>
#include <sstream>

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr int kAttack = 0, kLap = 4, kTurboUp = 12;

CashModifiers cashOf(const std::string &text)
{
    std::istringstream file(text);
    return CashModifiers::parse(file);
}

void testAwardedPointsRaiseTheTargetAndTheirKind()
{
    RacePoints points;
    points.award(250, kLap, 0, 0.f);
    CHECK(points.target() == 250 && points.ofKind(kLap) == 250);
}

void testSameKindAddsToItsPopup()
{
    RacePoints points;
    points.award(100, kAttack, 0, 0.f);
    points.award(50, kAttack, 0, 0.1f);
    CHECK(points.queuedPopups() == 1 && points.popup().points == 150);
}

void testZeroPointsQueueOnlyMessages()
{
    RacePoints points;
    points.award(0, kAttack, 0, 0.f);
    points.award(0, kTurboUp, 0, 0.f);
    CHECK(points.queuedPopups() == 1 && points.popup().kind == kTurboUp);
}

void testRepeatedShowingPopupIsRetimedToHalfWay()
{
    RacePoints points;
    points.award(100, kAttack, 0, 0.f);
    points.award(100, kAttack, 0, 0.7f);
    CHECK(points.popupShownFor(0.7f) == RacePoints::kPopupSeconds / 2.f);
}

void testPopupGoesAfterItsTimeAndTheNextStarts()
{
    RacePoints points;
    points.award(100, kAttack, 0, 0.f);
    points.award(100, kLap, 0, 0.f);
    points.step(0.85f);
    CHECK(points.queuedPopups() == 1 && points.popup().kind == kLap && points.popupShownFor(0.85f) < 0.1f);
}

void testShownScoreClimbsAQuarterOfTheGap()
{
    RacePoints points;
    points.award(4000, kAttack, 0, 0.f);
    points.step(0.f);
    CHECK(points.shown() == 1000);
}

void testShownScoreClimbsAtLeastAHundredButNotPastTheTarget()
{
    RacePoints points;
    points.award(150, kAttack, 0, 0.f);
    points.step(0.f);
    points.step(0.f);
    CHECK(points.shown() == 150);
}

void testHudResetKeepsTheBreakdown()
{
    RacePoints points;
    points.award(300, kLap, 0, 0.f);
    points.resetHud();
    CHECK(points.target() == 0 && !points.hasPopup() && points.ofKind(kLap) == 300);
}

void testCashModifiersReadNameThenValue()
{
    const CashModifiers cash = cashOf("pickup_bonus\r\n  250 \r\n\r\nlap_time_bonus\n-3\n");
    CHECK(cash.value("pickup_bonus") == 250 && cash.value("lap_time_bonus") == -3 && cash.value("kills_bonus") == 0);
}

void testLapGoalsOfASecondRouteOfBrackenstowe()
{
    std::istringstream file("brackenstowe_lap_times\n1 2 3 4 5\nalt_brackenstowe_lap_times\n61.5 58 55 52 49\n");
    const LapGoals goals = parseLapGoals(file, "alt_bracken");
    CHECK(goals[0] == 61.5f && goals[4] == 49.f);
}

void testLapGoalsOfAMissingTrackAreZero()
{
    std::istringstream file("silver_lap_times\n1 2 3 4 5\n");
    CHECK(parseLapGoals(file, "kalau")[2] == 0.f);
}

void testRaceScoreAddsThePlaceBonusThenTimesTheSpeed()
{
    const CashModifiers cash = cashOf("2nd_bonus\n500\nexpert_speed_modifer\n3\n");
    const RaceScore score = scoreRace(2, 1000, 2, 0, cash);
    CHECK(score.bonus == 500 && score.score == 4500);
}

void testRaceScoreGainsOnlyWhatBeatsTheHighScore()
{
    const CashModifiers cash = cashOf("1st_bonus\n0\nnovice_speed_modifer\n1\n");
    CHECK(scoreRace(1, 800, 0, 1000, cash).gain == 0 && scoreRace(1, 1200, 0, 1000, cash).gain == 200);
}

void testRaceScorePastSixthHasNoPlaceBonus()
{
    const CashModifiers cash = cashOf("6th_bonus\n100\nnovice_speed_modifer\n1\n");
    CHECK(scoreRace(7, 10, 0, 0, cash).bonus == 0);
}
}  // namespace

int main()
{
    testAwardedPointsRaiseTheTargetAndTheirKind();
    testSameKindAddsToItsPopup();
    testZeroPointsQueueOnlyMessages();
    testRepeatedShowingPopupIsRetimedToHalfWay();
    testPopupGoesAfterItsTimeAndTheNextStarts();
    testShownScoreClimbsAQuarterOfTheGap();
    testShownScoreClimbsAtLeastAHundredButNotPastTheTarget();
    testHudResetKeepsTheBreakdown();
    testCashModifiersReadNameThenValue();
    testLapGoalsOfASecondRouteOfBrackenstowe();
    testLapGoalsOfAMissingTrackAreZero();
    testRaceScoreAddsThePlaceBonusThenTimesTheSpeed();
    testRaceScoreGainsOnlyWhatBeatsTheHighScore();
    testRaceScorePastSixthHasNoPlaceBonus();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
