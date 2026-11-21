// Argon: tournament rules that need no App (docs\frontend\tournament.md "Tournament"); the rest: app_tourney.cpp
#pragma once

namespace argon::tourney
{
// 0xa1320 / 0x365b0: races per tournament (Novice 2, Amateur 4, Expert 4, Master 5, Redshift 9)
int racesIn(int level);

// 0x79130 (the table 0x1b9438, 0x791f0): the track of a character's (0..15) race `race` (0-based) at a level; the
// nemeses (8..15) race the other routes of their home tracks
int trackOf(int level, int character, int race);

// 0x791f0 (and 0x80450 at the loader's start): the race against the nemesis, on the home track's other route:
// Amateur / Expert race 3, Master race 4, Redshift race 0 (racesWon = the races won before it)
bool isNemesisRace(int level, int racesWon);

// race setup 0x79250: in a nemesis race 0x78d60 sets the craft count (+0x3a0c) to 2, the player and the nemesis in
// slot 1, and skips the random fill 0x82960; otherwise 6 (Argon: the editor's opponent count, `otherwise`)
int droneSeats(int level, int racesWon, int otherwise);

// FE TOURNEYTYPE / FE SPEEDSELECT (0x35e50 / 0x33fa0): frontend\speedselect's SPEEDO_NEEDLE turned about its z by
// 35 deg a level / speed (0x1a37c0, 0x1a36f4, 0x1a389c, 0x1a393c for 1..4; anything else 0)
constexpr float speedoNeedleDegrees(int level)
{
    return level >= 1 && level <= 4 ? 35.f * float(level) : 0.f;
}
}  // namespace argon::tourney
