// Argon: the race's start grid (logic\start_grid.hpp).
#include "logic/start_grid.hpp"

#include "craft/craftsim.hpp"
#include "logic/game_logic.hpp"
#include "logic/replay_race.hpp"

StartGrid::StartGrid(const Sources &sources) : sources_(sources)
{
}

std::string StartGrid::pilotOf(int slot)
{
    if (pilots.empty())
    {
        return {};
    }
    const MultiplayerSession &multiplayer = sources_.multiplayer;
    if (sources_.modes.multiplayerSelected() && slot < multiplayer.players() && multiplayer.pick(slot) >= 0)
    {
        return argon::pilotFolder(multiplayer.pick(slot));   // each player's pick (0x52cdc0)
    }
    if (!stillValid())
    {
        rebuild();
    }
    if (slot >= 0 && slot < int(order.size()))
    {
        return order[size_t(slot)];
    }
    return pilots[size_t(slot) % pilots.size()];
}

void StartGrid::rebuild()
{
    order = raceGrid(rule(), sources_.track.starts.size());
}

void StartGrid::reshuffle()
{
    seed_ = seed_ * 1103515245u + 12345u;
    rebuild();
}

GridRule StartGrid::rule() const
{
    return GridRule{pilots, size_t(playerPilot), seed_, sources_.progress.characters};
}

// the kept grid while the player is unchanged and its random slots (2..) are all still unlocked
bool StartGrid::stillValid() const
{
    if (sources_.replay.playing())
    {
        return !order.empty();   // the recording's pilots
    }
    if (pilots.empty() || order.empty() || order[0] != pilots[size_t(playerPilot) % pilots.size()])
    {
        return false;
    }
    for (size_t slot = 2; slot < order.size(); ++slot)
    {   // called for every pilotOf: no GridRule here, which would copy the pilot list
        if (!randomPilotAllowed(sources_.progress.characters, order[slot]))
        {
            return false;
        }
    }
    return true;
}

std::vector<std::string> StartGrid::kept() const
{
    if (!stillValid())
    {
        return {};
    }
    std::vector<std::string> grid = order;
    const MultiplayerSession &multiplayer = sources_.multiplayer;
    for (int slot = 0; sources_.modes.multiplayerSelected() && slot < multiplayer.players() && slot < int(grid.size()); ++slot)
    {
        if (multiplayer.pick(slot) >= 0)
        {
            grid[size_t(slot)] = argon::pilotFolder(multiplayer.pick(slot));   // each player's pick (0x52cdc0)
        }
    }
    return grid;
}
