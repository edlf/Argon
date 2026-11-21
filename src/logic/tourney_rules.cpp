// Argon: tournament rules (logic\tourney_rules.hpp)
#include "logic/tourney_rules.hpp"

#include <algorithm>

namespace argon::tourney
{
namespace
{
enum Level
{
    Novice,
    Amateur,
    Expert,
    Master,
    Redshift
};

const int kNemesisRaceDrones = 1;
const int kDraco = 7;

// 0x1b9438: a row of 8 tracks per base character (0..7)
const int kTrackTable[8][8] = {
    {0, 4, 3, 5, 1, 2, 6, 7},
    {1, 3, 6, 0, 2, 4, 5, 7},
    {2, 5, 4, 3, 6, 0, 1, 7},
    {3, 1, 0, 2, 5, 6, 4, 7},
    {4, 6, 2, 0, 1, 3, 5, 7},
    {5, 2, 1, 4, 6, 3, 0, 7},
    {6, 0, 5, 3, 4, 1, 2, 7},
    {7, 8, 11, 12, 14, 10, 13, 9}};

// 0x791f0: Amateur / Expert race 3, Master race 4 and Redshift race 0 are on the home track's other route
bool onOtherRoute(int level, int race)
{
    return race == 0 ? level == Redshift : ((level == Amateur || level == Expert) && race == 3) || (level == Master && race == 4);
}

// Master race 3 and Redshift race 7 are Draco, unless Draco is the character's home
bool onDraco(int level, int character, int race)
{
    const bool dracoRace = (level == Master && race == 3) || (level == Redshift && race == 7);
    return dracoRace && character != kDraco && character != kDraco + 8;
}
}  // namespace

int racesIn(int level)
{
    static const int kRaces[5] = {2, 4, 4, 5, 9};
    return kRaces[std::clamp(level, 0, 4)];
}

// race 0 is the home track (Redshift: its other route); Redshift race 8 is the home track again; otherwise the
// character's row of the table from an offset per level (Expert 2, Master 4, else 0)
int trackOf(int level, int character, int race)
{
    const int base = character & 7;
    const bool nemesis = character >= 8;
    if (race == 0 && level != Redshift)
    {
        return character;
    }
    if (onOtherRoute(level, race))
    {
        return nemesis ? base : base + 8;
    }
    if (onDraco(level, character, race))
    {
        return kDraco;
    }
    if (level == Redshift && race == 8)
    {
        return kTrackTable[base][0] + (nemesis ? 8 : 0);
    }
    const int offset = level == Expert ? 2 : level == Master ? 4 : 0;
    return kTrackTable[base][std::clamp(offset + race, 0, 7)];
}

bool isNemesisRace(int level, int racesWon)
{
    switch (level)
    {
    case Amateur:
    case Expert:
        return racesWon == 3;
    case Master:
        return racesWon == 4;
    case Redshift:
        return racesWon == 0;
    default:
        return false;
    }
}

int droneSeats(int level, int racesWon, int otherwise)
{
    return isNemesisRace(level, racesWon) ? kNemesisRaceDrones : otherwise;
}
}  // namespace argon::tourney
