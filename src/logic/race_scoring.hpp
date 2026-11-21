// Argon: the race's points and score (docs\frontend\tournament.md "Race scores"): the HUD's points as the race goes
// (logic\race_points.hpp: 0x83f00), the config's values (cashmodifiers.txt, trackgoals.txt), SCOREMENU's sum and its
// count-up (0x84110 / 0x95930, 0x95b00), and the laps and air time the drive's points have counted. App awards them
// (app_score.cpp: only in the modes that score).
#pragma once
#include "logic/race_points.hpp"
#include "logic/race_score.hpp"
#include "logic/sound_rules.hpp"

#include <string>

struct RaceScoring
{
    // config\cashmodifiers.txt and config\trackgoals.txt's <track>_lap_times
    void load(const std::string &gameFolder, const std::string &trackFolder);

    RacePoints points;
    CashModifiers cash;
    LapGoals lapGoals{};
    RaceScore score;                  // SCOREMENU's sum (App::computeScore)
    argon::sound::ScoreTally tally;   // its count-up
    int lapsSeen = 0;                 // the player's laps the drive has scored
    float airTimer = 0;               // the current jump's time in the air
};
