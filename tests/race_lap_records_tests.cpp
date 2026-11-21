// Argon: tests for the race's side of the lap records (logic\race_lap_records.hpp). Run with test.bat / test.sh.
#include "check.hpp"
#include "logic/race_lap_records.hpp"
#include "ui/game_strings.hpp"

namespace
{

using argon::test::checks;
using argon::test::failures;

constexpr int kTrack = 3, kSpeed = 2;

// no profile in use: nothing is written, the record takes fe 0x20c's fallback name
struct Race
{
    argon::ProfileStore profiles;
    GameStrings strings;
    std::string saves;
    RaceLapRecords records{profiles, strings, saves};
};

JudgedLap lapOf(float seconds, bool timeAttack)
{
    return JudgedLap{seconds, kTrack, kSpeed, timeAttack, 1};
}

void aLapUnderTheRecordTakesTheTable()
{
    Race race;
    race.records.startRace(kTrack, kSpeed);

    race.records.playerLap(lapOf(60.f, false));

    CHECK(race.records.table.at(kTrack, kSpeed).time == 60.f && race.records.newRecords[kTrack]);
}

void aSlowerLapLeavesTheRaceBest()
{
    Race race;
    race.records.table.at(kTrack, kSpeed).time = 60.f;
    race.records.startRace(kTrack, kSpeed);

    const argon::laps::LapVerdict verdict = race.records.playerLap(lapOf(70.f, false));

    CHECK(!verdict.raceBest && race.records.recordOnLap() == -1);
}

void aRaceBestRemembersItsLap()
{
    Race race;
    race.records.startRace(kTrack, kSpeed);

    race.records.playerLap(lapOf(60.f, false));

    CHECK(race.records.recordOnLap() == 1);
}

void aTimeAttackProfileBestOpensTheFastestLapPage()
{
    Race race;
    race.records.startRace(kTrack, kSpeed);

    race.records.playerLap(lapOf(60.f, true));

    CHECK(race.records.fastestLapPending);
}

void outsideTimeAttackNoProfileBestCounts()
{
    Race race;
    race.records.startRace(kTrack, kSpeed);

    race.records.playerLap(lapOf(60.f, false));

    CHECK(!race.records.fastestLapPending);
}

}  // namespace

int main()
{
    aLapUnderTheRecordTakesTheTable();
    aSlowerLapLeavesTheRaceBest();
    aRaceBestRemembersItsLap();
    aTimeAttackProfileBestOpensTheFastestLapPage();
    outsideTimeAttackNoProfileBestCounts();
    std::printf("race_lap_records_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
