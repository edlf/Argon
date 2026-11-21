// Argon: tests for the race's laps (logic/race_laps.hpp) and the multiplayer race's rules (logic/multiplayer_rules.hpp).
// No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/multiplayer_rules.hpp"
#include "check.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/race_laps.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::multiplayer::Standing;

void testDefaultLapsKeepTheRacesOwn()
{
    CHECK(argon::multiplayer::lapsForOption(0, 3) == 3);
}

void testLapOptionsAreThreeToNine()
{
    CHECK(argon::multiplayer::lapsForOption(1, 2) == 3 && argon::multiplayer::lapsForOption(4, 2) == 9);
}

void testRaceGoesOnWhileOneOfTwoHasFinished()
{
    CHECK(!argon::multiplayer::raceOver(Standing{2, 1, 1, false}));
}

void testRaceEndsWhenEveryoneHasFinished()
{
    CHECK(argon::multiplayer::raceOver(Standing{3, 3, 0, false}));
}

void testKnockOutLeavingOneEndsTheRace()
{
    const Standing s{3, 2, 1, true};
    CHECK(argon::multiplayer::raceOver(s) && argon::multiplayer::winsByDefault(s));
}

void testKnockOutLeavingTwoRacingGoesOn()
{
    CHECK(!argon::multiplayer::raceOver(Standing{3, 1, 2, true}));
}
}  // namespace

void testNoviceAndAmateurRaceTwoLaps()
{
    CHECK(argon::race::lapsForSpeedClass(0) == 2 && argon::race::lapsForSpeedClass(1) == 2);
}

void testExpertRacesThreeLaps()
{
    CHECK(argon::race::lapsForSpeedClass(2) == 3);
}

void testMasterAndRedshiftRaceFiveLaps()
{
    CHECK(argon::race::lapsForSpeedClass(3) == 5 && argon::race::lapsForSpeedClass(4) == 5);
}

void testSingleRaceIgnoresTheMultiplayerOption()
{
    CHECK(argon::race::raceLaps(false, 4, 0) == 2);
}

void testMultiplayerOptionSetsTheLaps()
{
    CHECK(argon::race::raceLaps(true, 4, 0) == 9);
}

void testMultiplayerDefaultTakesTheSpeedClassLaps()
{
    CHECK(argon::race::raceLaps(true, 0, 3) == 5);
}

void testStandingCountsFinishedAndKnockedOutPlayers()
{
    const int order[] = {1, 5};
    const bool out[] = {false, false, true};
    const Standing s = argon::multiplayer::standingOf(3, order, out);
    CHECK(s.players == 3 && s.finishedOrOut == 2 && s.stillRacing == 1 && s.anyKnockedOut);
}

void testStandingLeavesOutRacersPastThePlayers()
{
    const int order[] = {3};
    const bool out[] = {false, false};
    const Standing s = argon::multiplayer::standingOf(2, order, out);
    CHECK(s.finishedOrOut == 0 && s.stillRacing == 2);
}

void testLastRacingIsTheOneNeitherFinishedNorOut()
{
    const int order[] = {0};
    const bool out[] = {false, true, false};
    CHECK(argon::multiplayer::lastRacing(3, order, out) == 2);
}

void testSessionStartsWithoutPicks()
{
    MultiplayerSession session;
    session.join(0, 7, 2);
    session.start(2);
    CHECK(session.players() == 2 && session.pick(0) == MultiplayerSession::kNoPick);
}

void testSessionKeepsEachPlayersPickAndPad()
{
    MultiplayerSession session;
    session.start(2);
    session.join(1, 9, 3);
    CHECK(session.pick(1) == 9 && session.pad(1) == 3 && session.padRaces(3) && !session.padRaces(2));
}

void testSessionOfOnePlayerRacesNoPad()
{
    MultiplayerSession session;
    session.start(1);
    session.join(0, 4, 0);
    CHECK(!session.split() && !session.padRaces(0));
}

void testSessionKeepsItsWinsAcrossRaces()
{
    MultiplayerSession session;
    session.start(2);
    session.countWin(1);
    session.start(3);
    session.countWin(1);
    CHECK(session.wins(1) == 2 && session.wins(0) == 0);
}

void testSessionPlayersAreOneToFour()
{
    MultiplayerSession session;
    session.start(9);
    const int most = session.players();
    session.leave();
    CHECK(most == 4 && session.players() == 1);
}

int main()
{
    testNoviceAndAmateurRaceTwoLaps();
    testExpertRacesThreeLaps();
    testMasterAndRedshiftRaceFiveLaps();
    testSingleRaceIgnoresTheMultiplayerOption();
    testMultiplayerOptionSetsTheLaps();
    testMultiplayerDefaultTakesTheSpeedClassLaps();
    testDefaultLapsKeepTheRacesOwn();
    testLapOptionsAreThreeToNine();
    testRaceGoesOnWhileOneOfTwoHasFinished();
    testRaceEndsWhenEveryoneHasFinished();
    testKnockOutLeavingOneEndsTheRace();
    testKnockOutLeavingTwoRacingGoesOn();
    testStandingCountsFinishedAndKnockedOutPlayers();
    testStandingLeavesOutRacersPastThePlayers();
    testLastRacingIsTheOneNeitherFinishedNorOut();
    testSessionStartsWithoutPicks();
    testSessionKeepsEachPlayersPickAndPad();
    testSessionOfOnePlayerRacesNoPad();
    testSessionKeepsItsWinsAcrossRaces();
    testSessionPlayersAreOneToFour();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
