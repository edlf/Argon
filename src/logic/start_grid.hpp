// Argon: the race's start grid as the game builds a quick race (0x79250 + 0x82960; the rule: track\track_source.hpp
// GridRule): slot 0 the player, slot 1 the player's nemesis, the others random unlocked pilots; kept while the player
// is unchanged and its random pilots are all still unlocked. A multiplayer race seats each player's pick (0x52cdc0); a
// replay keeps its recording's pilots.
#pragma once
#include "logic/game_progress.hpp"
#include "logic/multiplayer_session.hpp"
#include "track/track_source.hpp"

#include <cstdint>
#include <string>
#include <vector>

class GameModes;
class ReplayRace;

class StartGrid
{
  public:
    // what the grid reads and does not own
    struct Sources
    {
        const GameModes &modes;                  // game mode 4 seats the players' picks
        const MultiplayerSession &multiplayer;   // the players' picks
        const ReplayRace &replay;                // a playing replay's pilots stand
        const argon::GameProgress &progress;     // the unlocked characters
        const argon::TrackData &track;           // its start slots
    };
    explicit StartGrid(const Sources &sources);

    std::string pilotOf(int slot);   // its pilot folder; the grid made again first when it no longer holds
    void rebuild();
    void reshuffle();   // a new seed: the random slots drawn again
    GridRule rule() const;
    std::vector<std::string> kept() const;   // the grid with the players' picks, none when it no longer holds

    std::vector<std::string> pilots;   // data\craft folders
    int playerPilot = 0;               // into pilots
    std::vector<std::string> order;    // the pilot folder per slot

  private:
    bool stillValid() const;

    Sources sources_;
    uint32_t seed_ = 1;
};
