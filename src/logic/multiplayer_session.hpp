// Argon: the human players of a split-screen race (game mode 4; docs\frontend\multiplayer.md "Multiplayer"): how many
// (0x52cd8c), each one's character (0x52cdc0) and controller (0x52abec) as FE CHARSELECT seated them, MULTIPLAYERENDMENU's
// Wins this session and the race's winner "By Default". The rules of the race's end: logic\multiplayer_rules.hpp.
#pragma once
#include "logic/split_screen.hpp"

#include <array>

class MultiplayerSession
{
  public:
    static constexpr int kNoPick = -1;
    static constexpr int kNoWinner = -1;

    MultiplayerSession();

    void start(int players);    // that many players, no picks yet (a player without one keeps the grid's craft)
    void leave();               // one player again, no picks
    void seatPlayers(int players);   // the views only (tests: --split), the picks kept
    void join(int player, int character, int pad);

    int players() const;
    bool split() const;   // more than one view
    int pick(int player) const;   // kNoPick for none
    int pad(int player) const;
    bool padRaces(int pad) const;   // a split race's player joined with it

    void countWin(int player);
    int wins(int player) const;

    void setDefaultWinner(int player);   // kNoWinner: none
    int defaultWinner() const;

  private:
    static size_t seat(int player);

    int players_ = 1;
    std::array<int, argon::split::kMaxPlayers> picks_;
    std::array<int, argon::split::kMaxPlayers> pads_{0, 1, 2, 3};
    std::array<int, argon::split::kMaxPlayers> wins_{};
    int defaultWinner_ = kNoWinner;
};
