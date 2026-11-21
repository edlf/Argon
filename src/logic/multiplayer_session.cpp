// Argon: the human players of a split-screen race (multiplayer_session.hpp).
#include "logic/multiplayer_session.hpp"

#include <algorithm>

MultiplayerSession::MultiplayerSession()
{
    picks_.fill(kNoPick);
}

void MultiplayerSession::start(int players)
{
    seatPlayers(players);
    picks_.fill(kNoPick);
}

void MultiplayerSession::leave()
{
    start(1);
}

void MultiplayerSession::seatPlayers(int players)
{
    players_ = std::clamp(players, 1, argon::split::kMaxPlayers);
}

void MultiplayerSession::join(int player, int character, int pad)
{
    picks_[seat(player)] = character;
    pads_[seat(player)] = pad;
}

int MultiplayerSession::players() const
{
    return players_;
}

bool MultiplayerSession::split() const
{
    return players_ > 1;
}

int MultiplayerSession::pick(int player) const
{
    return picks_[seat(player)];
}

int MultiplayerSession::pad(int player) const
{
    return pads_[seat(player)];
}

bool MultiplayerSession::padRaces(int pad) const
{
    return split() && std::find(pads_.begin(), pads_.begin() + players_, pad) != pads_.begin() + players_;
}

void MultiplayerSession::countWin(int player)
{
    ++wins_[seat(player)];
}

int MultiplayerSession::wins(int player) const
{
    return wins_[seat(player)];
}

void MultiplayerSession::setDefaultWinner(int player)
{
    defaultWinner_ = player;
}

int MultiplayerSession::defaultWinner() const
{
    return defaultWinner_;
}

size_t MultiplayerSession::seat(int player)
{
    return size_t(std::clamp(player, 0, argon::split::kMaxPlayers - 1));
}
