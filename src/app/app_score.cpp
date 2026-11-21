// Argon: the race's scores (docs\frontend\tournament.md "Race scores"). SCOREMENU (vtable 0x19c018: init 0x94390, update
// 0x94ff0, fill 0x95780, cash / high score 0x95930) and POINTSMENU, the breakdown (vtable 0x19c500: init 0x8bfc0,
// update 0x8d2c0). Race points 0x84060, the race score 0x84110 (both with the profile block 0x1bab30 and the
// cashmodifiers.txt values from +0x1978).

#include "app/app.hpp"


// 0x84110 / 0x95930 (logic\race_score.hpp): the score; the high score per character and track (Tournament 0x1bb108,
// Quick Race 0x1bb150, + character x 0x2c + track) takes what beats it and, in a tournament, the character's cash
// (0x1bb0e8 + character x 0xb0) too
void App::computeScore()
{
    const std::vector<int> order = race.resultOrder();
    int place = 1;
    for (size_t i = 0; i < order.size(); ++i)
    {
        if (order[i] == 0)
        {
            place = int(i) + 1;
        }
    }
    const int pilot = logic().pilot();
    int &high = logic().highScores()[pilot][std::clamp(picks.loadTrackId, 0, 17)];
    scoring.score = scoreRace(place, scoring.points.target(), picks.speedClass, high, scoring.cash);
    high += scoring.score.gain;
    logic().earnCash(pilot, scoring.score.gain);
    progressKeeper.storeProgress();
    scoring.tally.start(scoring.score.score);
}

// 0x83f00 posts a HUD event (kind, points, pilot) to the HUD's points (logic\race_points.hpp); outside the modes that
// score, only its messages
void App::awardPoints(int points, int kind, int pilot)
{
    if (!logic().scoresPoints() && kind != 12)
    {
        return; // 12: the "Turbo Up" message stays
    }
    scoring.points.award(points, kind, pilot, simClock);
}

// 0x83d80: a finished lap faster than the goal earns lap_time_bonus x (goal - lap) x 100 (kind 4)
void App::scoreLap()
{
    if (!logic().scoresPoints())
    {
        return;
    }
    float goal = scoring.lapGoals[size_t(std::clamp(picks.speedClass, 0, kSpeedClasses - 1))], lap = sim.lastLap();
    float shortfall = goal - lap;
    if (lap > 0.f && shortfall > 0.f)
    {
        awardPoints(int(shortfall * 100.f * float(scoring.cash.value("lap_time_bonus"))), 4); // "Great Lap"
    }
}
