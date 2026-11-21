// Argon: the rules and writes over the profile's progress (logic\game_progress.hpp): what is open (the speeds, the
// characters, a tournament's races), FE CHOOSESAVE's Percentage Complete parts, the cheats that change it
// (docs\frontend\cheats.md "Easter Eggs and Cheats"), the Redshift wins POSTABLE remembers (0x8ecc7), --unlock-all, and
// the progress written into the profile in use (saves\profile_fields.hpp).
#pragma once
#include "logic/game_progress.hpp"
#include "saves/profile_fields.hpp"
#include "saves/profile_store.hpp"
#include "ui/page_hosts.hpp"

// a race the player has just won, as the Redshift wins look at it
struct WonRace
{
    bool quickRace;   // the mode (not Tournament, not Time Attack)
    int speed;        // the speed class, 4 = Redshift
    int track;        // the game's track id
    bool replayed;    // the attract demo or a replay: not the player's
};

class ProgressKeeper : public UnlockRules, public ProgressStore
{
  public:
    ProgressKeeper(argon::GameProgress &progress, argon::ProfileStore &profiles);

    bool unlockAll = false;   // --unlock-all: the profile's unlocks are neither read nor written
    argon::ProfileUnlocks profileUnlocks() const;
    void applyUnlockAll();   // every egg and cheat unlocked and the cheat menu shown (the switches stay the profile's)
    bool anyEgg() const;

    int speedMax() const override;
    int tourneyDone(int level, int character) const override;
    int nextTourneyTrack(int level, int character) const;
    bool charUnlocked(int id) const override;
    argon::ProfileProgress profileProgress() const override;

    void storeProgress() override;
    void activateCheat(int cheat) override;
    void switchCheat(int cheat) override;
    void recordRedshiftWin(const WonRace &race);

  private:
    argon::GameProgress &progress_;
    argon::ProfileStore &profiles_;
};
