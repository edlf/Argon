// Argon: what a tournament win gave that the player did not have before: the game's record at 0x1bc890
// (game state 0x1bab30 + 0x1d60..0x1d78), cleared and filled by profile_tournament_won 0x84160 and listed by FE AWARDS
// (ui\awards.cpp). -1 / false: nothing of that kind.
#pragma once

struct TourneyAwards
{
    int trophy = -1;          // +0x1d60: the level whose trophy is new (without one FE AWARDS shows nothing)
    int character = -1;       // +0x1d64: the nemesis it unlocked (an original won above Novice)
    int track = -1;           // +0x1d68: the home track's other route it unlocked (8..15)
    int level = -1;           // +0x1d6c: the tournament level it opened (2 Expert, 3 Master, 4 Redshift)
    bool kaHekili = false;    // +0x1d70: Draco's owner (character 7)
    bool kutoo = false;       // +0x1d71: Draco's nemesis (character 15)
    bool draco = false;       // +0x1d72: the Draco Island track
    int easterEgg = -1;       // +0x1d74: the easter egg it unlocked (0..7)
    bool mars = false;        // +0x1d78: the Mars tracks
};

// FE AWARDS shows the shell ticker only with an easter egg: its row (0x137f0 -> 0x141cf) is the one place the page
// writes the ticker (fe 0x1d1) and shows it; otherwise it stays as the race start / the slide-outs left it: hidden
inline bool awardsShowTicker(const TourneyAwards &awards)
{
    return awards.easterEgg >= 0;
}
