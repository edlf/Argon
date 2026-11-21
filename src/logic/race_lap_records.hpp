// Argon: the race's side of the lap records (the table: saves\lap_records.hpp; the rules: logic\lap_record_rules.hpp).
// Every mode (race_craft_lap 0x3d5a0; Time Attack 0x429c0): the race's best (+0xd338) starts at the track's record
// (pickups_init 0x3e4a0); a lap faster than it is the race's new best ("New lap record", +0xd32c), and when it also
// beats the table the table takes it with the profile's name (0x52ac03[track] = 1: FE FASTESTLAP saves it after the
// race). Time Attack keeps the profile's own bests too (g_profile +0) and only then looks at the table.
#pragma once
#include "logic/lap_record_rules.hpp"
#include "saves/lap_records.hpp"
#include "saves/profile_store.hpp"

#include <array>
#include <string>

class GameStrings;

// the lap being judged: which table entry and how the race runs
struct JudgedLap
{
    float seconds;
    int track, speed;    // the game's track id 0..17, the speed class 0..4
    bool timeAttack;     // the profile's own bests count
    int lapsDone;        // the player's finished laps, this one included
};

class RaceLapRecords
{
  public:
    // savesFolder: <saves>, set by the launch options before the first load
    RaceLapRecords(argon::ProfileStore &profiles, const GameStrings &strings, const std::string &savesFolder);

    std::string file() const;   // <saves>\records\laptimes.json
    void load();
    void save() const;

    void startRace(int track, int speed);   // the race's best from the track's record
    // the player's finished lap: the race's best, the profile's best (written to the profile) and the record; the caller
    // writes the table when the record fell
    argon::laps::LapVerdict playerLap(const JudgedLap &lap);
    int recordOnLap() const;   // the player's laps when the last one beat the race's best, else -1

    void readBestLaps(const argon::Profile &profile);   // the profile JSON's "bestLaps"

    LapRecordBook table;
    std::array<bool, 18> newRecords{};   // 0x52ac03: the tracks whose record fell since FE FASTESTLAP last saved
    bool fastestLapPending = false;      // 0x5994c1 = "FE FASTESTLAP": FE MAIN goes on to it (Time Attack, a profile best)

  private:
    void setProfileBest(const JudgedLap &lap);
    void setRecord(const JudgedLap &lap);

    argon::ProfileStore &profiles_;
    const GameStrings &strings_;
    const std::string &savesFolder_;
    ProfileBestLaps profileBests_;      // the profile's own (Time Attack only)
    float raceBest_ = kUnsetLap;        // +0xd338
    int recordOnLap_ = -1;              // +0xd32c
};
