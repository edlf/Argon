// Argon: a race's score (docs\frontend\tournament.md "Race scores"): the values of config\cashmodifiers.txt (name line,
// value line: pickup_bonus, megaup_bonus, powerchip_bonus, lap_time_bonus, ...; 0x1bc4c4.. in file order), the goal lap
// times of config\trackgoals.txt ("<track>_lap_times", one per speed class; 0x1bc5ac) and the score SCOREMENU shows
// (0x84110). The pages: ui\score.cpp.
#pragma once
#include <array>
#include <istream>
#include <map>
#include <string>

class CashModifiers
{
  public:
    static CashModifiers parse(std::istream &file);
    int value(const std::string &name) const;   // 0 for a name the file does not have

  private:
    std::map<std::string, int> values_;
};

constexpr int kSpeedClasses = 5;
using LapGoals = std::array<float, kSpeedClasses>;

// the goals of a track folder ("alt_bracken" reads "alt_brackenstowe_lap_times"); 0 where the file has none
LapGoals parseLapGoals(std::istream &file, const std::string &trackFolder);

struct RaceScore
{
    int place = 1;
    int points = 0;          // earned in the race
    int bonus = 0;           // the place's (1st_bonus .. 6th_bonus)
    int speedModifier = 1;   // the speed class's (novice .. pro, qr _speed_modifer)
    int score = 0;           // (bonus + points) x speedModifier
    int previousHigh = 0;    // the character's high score on the track before
    int gain = 0;            // what beat it (added to it and, in a tournament, to the cash)
};

// 0x84110 and 0x95930
RaceScore scoreRace(int place, int points, int speedClass, int previousHigh, const CashModifiers &cash);
