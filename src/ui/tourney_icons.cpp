// Argon: FE TOURNEYTYPE's trophy and race icons (0x36100..0x362c2); ui\tourney_icons.hpp
#include "ui/tourney_icons.hpp"

namespace menu
{
namespace
{
constexpr float kGap = 40.f;   // 0x1a3604: after the row, and the trophy's size
constexpr float kRaceIcon = 30.f;
constexpr float kRowStep = 40.f;
constexpr float kTrophyTop = 90.f, kRaceTop = 95.f;
}  // namespace

std::vector<PlacedTourneyIcon> tourneyRowIcons(const TourneyRowState &row)
{
    std::vector<PlacedTourneyIcon> icons;
    float x = row.rowRight + kGap;
    const float rowOffset = kRowStep * float(row.level);
    if (row.trophy)
    {
        icons.push_back({TourneyIcon::Trophy, x, kTrophyTop + rowOffset, kGap});
        x += kGap;
    }
    if (row.racesWon <= 0)
    {
        return icons;
    }
    for (int race = 0; race < row.races; ++race)
    {
        const TourneyIcon icon = race < row.racesWon ? TourneyIcon::RaceWon : TourneyIcon::RaceToGo;
        icons.push_back({icon, x, kRaceTop + rowOffset, kRaceIcon});
        x += kRaceIcon;
    }
    return icons;
}
}  // namespace menu
