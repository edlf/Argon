// Argon: FE TOURNEYTYPE's icons after each level's row (update 0x35e50): the trophy once won, then one
// icon per race of a tournament in progress (docs\frontend\tournament.md "Tournament")
#pragma once
#include <vector>

namespace menu
{
enum class TourneyIcon
{
    Trophy,     // data\frontend\tourn_C.dds (shell +0x1fd0), 40 x 40
    RaceWon,    // tourn_B.dds (+0x1fa8), 30 x 30
    RaceToGo,   // tourn_A.dds (+0x1f80), 30 x 30
};

struct PlacedTourneyIcon
{
    TourneyIcon icon;
    float x, y, size;
};

struct TourneyRowState
{
    int level = 0;            // the row: Novice .. Redshift
    float rowRight = 0.f;     // the row's x + its width (the element's +0x3c)
    bool trophy = false;      // 0x1bb0ef + character x 0xb0 + level
    int racesWon = 0;         // the progress (0x1bb0f4 ...); icons only while it is above 0
    int races = 0;            // the level's race count (0xa1320)
};

// x from rowRight + 40 (0x1a3604), the trophy at y 90 + 40 level, the races at y 95 + 40 level, 30 apart
std::vector<PlacedTourneyIcon> tourneyRowIcons(const TourneyRowState &row);
}  // namespace menu
