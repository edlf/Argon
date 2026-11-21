// Argon: the race's lap count g_race_laps (0x52cdd8), set by 0x3b0b0 when a game mode starts a race. It is the
// finish line and the start / per-lap turbo's multiplier (0x429c0: (1.5 + 0.5 x Turbo level) x laps).
#pragma once

#include "logic/multiplayer_rules.hpp"

namespace argon::race
{

// 0x3b1b5: by the speed class (global_game_speed 0x1bbbe8: 0 Novice .. 4 Redshift): Novice / Amateur 2, Expert 3,
// Master / Redshift 5 (and 5 for a speed outside 0..4)
constexpr int lapsForSpeedClass(int speedClass)
{
    if (speedClass == 0 || speedClass == 1)
    {
        return 2;
    }
    return speedClass == 2 ? 3 : 5;
}

// 0x3b14e: game mode 4 (multiplayer) takes the Multiplayer Laps option (3 / 5 / 7 / 9); its Default, and every
// other mode, the speed class's
constexpr int raceLaps(bool multiplayer, int lapsOption, int speedClass)
{
    const int speedLaps = lapsForSpeedClass(speedClass);
    return multiplayer ? argon::multiplayer::lapsForOption(lapsOption, speedLaps) : speedLaps;
}

}  // namespace argon::race
