// Argon: a tournament won (profile_tournament_won 0x84160; docs\frontend\tournament.md "Tournament"): the trophy, the
// unlocks it gives and the record of what is new, and the tournament being raced.
#pragma once
#include "logic/game_progress.hpp"
#include "logic/tourney_awards.hpp"

// the tournament the player races (a race's character and the last one's outcome)
struct TourneyRun
{
    int character = 0;            // who races it (FE CHARSELECT's pick when it started)
    bool finished = false;        // the last race won it: the tournament is over
    TourneyAwards awards;         // what the last tournament win gave (0x1bc890), FE AWARDS lists it
    bool awardsPending = false;   // 0x52ac00: FE MAIN opens FE AWARDS (set with the last race's WINMENU)
};

namespace argon::tourney
{
// the trophy (0x1bb0ef + character x 0xb0 + level); above Novice an original unlocks its nemesis and its home track's
// other route, a nemesis winning Expert or Master for the first of the two the next easter egg; Amateur opens Expert,
// Expert Master; Master gives Draco Island, Draco's owner or nemesis and Redshift, Redshift gives Mars. Returns what
// of it is new.
TourneyAwards winTournament(GameProgress &progress, int level, int character);
}  // namespace argon::tourney
