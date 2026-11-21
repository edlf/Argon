// Argon: the race's players' pads (logic\player_pads.hpp) from the game mode and the multiplayer session's ports.
#pragma once
#include "logic/player_pads.hpp"

class GameModes;
class MultiplayerSession;

class RacePlayerPads : public PlayerPads
{
  public:
    RacePlayerPads(const GameModes &modes, const MultiplayerSession &multiplayer) : modes_(modes), multiplayer_(multiplayer)
    {
    }

    bool multiplayerRace() const override;
    int padOfPlayer(int player) const override;

  private:
    const GameModes &modes_;
    const MultiplayerSession &multiplayer_;
};
