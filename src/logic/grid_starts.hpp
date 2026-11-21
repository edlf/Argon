// Argon: which start locator each craft of the race takes. The race setup (0x3de29) puts craft i of the race's n on
// the track's locator "player_start<n - 1 - i>" (locator.dat; found by name, 0x59f90), so the player (craft 0) starts
// last and the last craft on player_start0, the pole; a lone craft (Time Attack) takes player_start0.
#pragma once

#include "track/trackdata.hpp"

#include <string>
#include <vector>

namespace argon::grid
{

// the player_start number of craft `craft` of `craftCount`; a slot past the race's crafts (the editor shows a ship on
// every locator) keeps its own number
constexpr int startNumberOfCraft(int craft, int craftCount)
{
    return craft < craftCount ? craftCount - 1 - craft : craft;
}

inline std::string startLocatorName(int number)
{
    return "player_start" + std::to_string(number);
}

// the index in `starts` of the locator named player_start<number>; a track without that name: the number-th locator
inline size_t startIndexOf(const std::vector<StartSlot> &starts, int number)
{
    const std::string name = startLocatorName(number);
    for (size_t i = 0; i < starts.size(); ++i)
    {
        if (starts[i].name == name)
        {
            return i;
        }
    }
    return size_t(number);
}

}  // namespace argon::grid
