#include "app/race_player_pads.hpp"

#include <algorithm>

#include "logic/game_logic.hpp"
#include "logic/multiplayer_session.hpp"

bool RacePlayerPads::multiplayerRace() const
{
    return modes_.multiplayerSelected();
}

// the controller a player joined with (multiplayer); else player n is the n-th controller
int RacePlayerPads::padOfPlayer(int player) const
{
    const int clamped = std::clamp(player, 0, argon::split::kMaxPlayers - 1);
    return multiplayerRace() ? multiplayer_.pad(clamped) : clamped;
}
