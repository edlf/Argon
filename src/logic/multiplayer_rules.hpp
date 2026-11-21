// Argon: the multiplayer race's rules (game mode 4, gl_multiplayer.cpp; docs\frontend\multiplayer.md "Multiplayer").
#pragma once
#include <algorithm>
#include <span>

namespace argon::multiplayer
{

// 0x3b0b0: the Multiplayer Laps option (0x10): Default keeps the race's own laps, then 3 / 5 / 7 / 9
constexpr int lapsForOption(int option, int defaultLaps)
{
    return option >= 1 && option <= 4 ? 1 + 2 * option : defaultLaps;
}

// MULTIPLAYERENDMENU opens once every player has finished or is out (0x41de0: the finish order holds as many entries
// as players), or, after a knock-out, at most one still races (0x620e0: that one finishes "By Default")
struct Standing
{
    int players = 0, finishedOrOut = 0, stillRacing = 0;
    bool anyKnockedOut = false;
};

constexpr bool raceOver(const Standing &standing)
{
    return standing.finishedOrOut >= standing.players || (standing.anyKnockedOut && standing.stillRacing <= 1);
}

constexpr bool winsByDefault(const Standing &standing)
{
    return standing.anyKnockedOut && standing.stillRacing == 1;
}

// the players (racers 0..players - 1) as the race stands: past the line (in finishOrder) or knocked out (knockedOut,
// per player)
inline Standing standingOf(int players, std::span<const int> finishOrder, std::span<const bool> knockedOut)
{
    Standing standing;
    standing.players = std::min(players, int(knockedOut.size()));
    for (int p = 0; p < standing.players; ++p)
    {
        const bool finished = std::find(finishOrder.begin(), finishOrder.end(), p) != finishOrder.end();
        const bool out = knockedOut[size_t(p)];
        standing.anyKnockedOut = standing.anyKnockedOut || out;
        standing.finishedOrOut += finished || out ? 1 : 0;
        standing.stillRacing += finished || out ? 0 : 1;
    }
    return standing;
}

// the first player neither past the line nor knocked out, -1 for none
inline int lastRacing(int players, std::span<const int> finishOrder, std::span<const bool> knockedOut)
{
    for (int p = 0; p < std::min(players, int(knockedOut.size())); ++p)
    {
        if (std::find(finishOrder.begin(), finishOrder.end(), p) == finishOrder.end() && !knockedOut[size_t(p)])
        {
            return p;
        }
    }
    return -1;
}

}  // namespace argon::multiplayer
