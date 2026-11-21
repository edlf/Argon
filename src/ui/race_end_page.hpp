// Argon: the race end's page up (0x52afa8, RaceSession::raceEnd): racing, or the race-end menu shown (ui\race_menus.hpp)
#pragma once

namespace race_end
{
enum Page : int
{
    kRacing = 0,
    kWinMenu = 1,
    kLoseMenu = 2,
    kScoreMenu = 3,
    kPostTable = 5,
    kPointsMenu = 6,
    kMultiplayerEnd = 7
};
}  // namespace race_end
