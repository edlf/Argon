// Argon: which pad each player of the race plays on: in game mode 4 (multiplayer) the port the player joined with, else
// player n on the n-th pad. Implemented by app\race_player_pads.hpp; the HUD, the loader, the race's menus and the
// weapons ask it.
#pragma once

class PlayerPads
{
  public:
    virtual bool multiplayerRace() const = 0;
    virtual int padOfPlayer(int player) const = 0;

  protected:
    ~PlayerPads() = default;
};
