// Argon: the race's side of the lap records (logic\race_lap_records.hpp).
#include "logic/race_lap_records.hpp"

#include "ui/game_strings.hpp"

#include <algorithm>

RaceLapRecords::RaceLapRecords(argon::ProfileStore &profiles, const GameStrings &strings, const std::string &savesFolder)
    : profiles_(profiles), strings_(strings), savesFolder_(savesFolder)
{
}

std::string RaceLapRecords::file() const
{
    return savesFolder_ + "/records/laptimes.json";
}

void RaceLapRecords::load()
{
    table.load(file());
}

void RaceLapRecords::save() const
{
    table.save(file());
}

// a new race (resetDriver): the race's best starts at this track's record (pickups_init 0x3e4a0: +0xd338)
void RaceLapRecords::startRace(int track, int speed)
{
    raceBest_ = table.at(track, speed).time;
    recordOnLap_ = -1;
}

// faster than the race's best -> the new best (+0xd32c, huddata +0x42: Time Attack's HUD shows "New lap record"); in
// Time Attack faster than the profile's own best too -> that best, and FE MAIN goes on to FE FASTESTLAP after the
// race; then faster than the table -> the record, with the profile's name
argon::laps::LapVerdict RaceLapRecords::playerLap(const JudgedLap &lap)
{
    const argon::laps::LapTimes before{raceBest_, profileBests_.at(lap.track, lap.speed), table.at(lap.track, lap.speed).time};
    const argon::laps::LapVerdict verdict = argon::laps::judgeLap(lap.seconds, before, lap.timeAttack);
    if (!verdict.raceBest)
    {
        return verdict;
    }
    raceBest_ = lap.seconds;
    recordOnLap_ = lap.lapsDone;
    if (verdict.profileBest)
    {
        setProfileBest(lap);
    }
    if (verdict.record)
    {
        setRecord(lap);
    }
    return verdict;
}

int RaceLapRecords::recordOnLap() const
{
    return recordOnLap_;
}

// 0x429c0: the profile's best and the way back (0x5994c1 = "FE FASTESTLAP"); the profile JSON's "bestLaps"
void RaceLapRecords::setProfileBest(const JudgedLap &lap)
{
    profileBests_.set(lap.track, lap.speed, lap.seconds);
    fastestLapPending = true;
    if (const auto current = profiles_.current())
    {
        argon::Profile &profile = *current;
        profile.bestLaps = profileBests_.text();
        profiles_.save(profile);
    }
}

// the table's record with the profile's name, flagged new for FE FASTESTLAP (0x52ac03[track])
void RaceLapRecords::setRecord(const JudgedLap &lap)
{
    LapRecord &record = table.at(lap.track, lap.speed);
    record.time = lap.seconds;
    record.name = profiles_.hasCurrent() ? profiles_.currentName() : strings_.frontEnd(0x20c, "--------");
    newRecords[size_t(std::clamp(lap.track, 0, 17))] = true;
}

void RaceLapRecords::readBestLaps(const argon::Profile &profile)
{
    profileBests_ = ProfileBestLaps::read(profile.bestLaps);
}
