// Argon: the race's per-frame step at the lap line: the turbo (Time Attack's refill 0x429c0, the other modes' start
// grant), then the game mode's lapLineCrossed (Time Attack's ghosts: logic\time_attack.cpp)
#include "app/app.hpp"

// 0x429c0, Time Attack's update only (vtable 0x1a16f8 [1] 0x43220): the player's craft crossing the lap line
// (craftinfo 0x597315 + 23, bit 1) with more than 1 s on the lap clock raises 0x439ff6; on the next pass the turbo is
// set (not added: it overrides race_craft_lap's +1.5 + 0.5 x level) to (1.5 + 0.5 x the Turbo upgrade level) x
// g_race_laps. The first crossing after GO counts too (the lap clock then holds the run from the grid), so a Time
// Attack starts with a full turbo and every lap refills it.
// The other modes: race_craft_lap 0x3d5a0 adds a lap's turbo on every lap the race manager counts (+0x57c bit 1), and
// the manager (0x62ba0) counts the first crossing too (its lap counter and crossings both start at 0: a 2 lap race
// ends on the third crossing), so the grid's run gets a lap's turbo at the line. The viewer's GameCraft counts laps
// from that crossing on, so the start's grant is given here (App::playerLapFinished gives the others)
void App::stepLapLine()
{
    if (sim.lineCrossings() == session.lapLineCrossings)
    {
        return;
    }
    session.lapLineCrossings = sim.lineCrossings();
    // the check's lap clock (+0xd330, from the last crossing) also holds the countdown before the first crossing;
    // the viewer's restarts at GO and reaches the line in about 1.2 s, so the first crossing always passes
    const bool first = sim.laps() == 0;
    if (logic().refillsTurboAtLapLine())
    {
        if (first || sim.crossingLapTime() > 1.f)
        {
            sim.setTurbo(argon::upgrades::lapTurboSeconds(upgradeLevel(argon::upgrades::kTurbo)) * float(session.hudLaps));
        }
    }
    else if (first)
    {
        grantLapTurbo();
    }
    logic().lapLineCrossed(first);
}
