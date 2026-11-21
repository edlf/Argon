// Argon: the race the front end has picked, as its pages and the race read it: the character (FE CHARSELECT), the
// course row (FE COURSE SELECT), the speed (FE SPEED SELECT) and the pilot and course of the race being loaded.
#pragma once

struct RacePicks
{
    int character = 0;     // 0..15
    int courseRow = 0;     // row 0..7 track, 8 Mars
    int speedClass = 2;    // 0..4 = speed 1..5 (character_stats *1..*5; the game's global_game_speed)
    int loadPilot = 0;     // the race being loaded: its pilot
    int loadTrackId = 0;   // and its course (the game's track id, 0x1b9538)
};
