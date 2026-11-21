// Argon: the game logic, one class per game mode (0x52ccfc). The game builds the race's object from the
// mode at 0x7f222 (jump table 0x7f384), every class on the same base (ctor 0x3ac40): 0 Tournament vtable 0x1a1bd8,
// 1 Quick Race 0x1a18f8, 2 CAttractDemo 0x1a1a0c, 3 Time Attack 0x1a16f8 (0xd4c4 bytes, the others 0xd39c: its
// ghosts), 4 0x1a197c (multiplayer). The viewer keeps one object per mode (GameModes) and selects it
// where the front end sets 0x52ccfc; the front-end pages ask it too, where the game reads 0x52ccfc.
#pragma once
#include <glm/glm.hpp>

#include "logic/ghost.hpp"
#include "logic/mode_pages.hpp"

class ProgressKeeper;
struct TourneyRun;
namespace argon
{
class CraftExhausts;
class GameCraft;
struct GameProgress;
}  // namespace argon

// what the modes read and change
struct ModeSources
{
    argon::GameProgress &progress;
    ProgressKeeper &rules;         // a tournament's races done, its progress stored
    TourneyRun &tourney;
    int &character, &track;            // FE CHARSELECT's and FE COURSE SELECT's picks
    const int &loadPilot, &loadTrack, &speedClass;   // the race being loaded
    int &raceHint;                     // the loading screen's hint shown
    const argon::GameCraft &player;
    const glm::mat4 &playerPose;       // the player's craft at the last 60 Hz step
    const argon::CraftExhausts &exhausts;
};

// what the modes ask App to do: start a race (the pages they lead to are ModePages')
class ModeHost
{
  public:
    virtual void startQuickRace(int pilotId, int trackId) = 0;

  protected:
    ~ModeHost() = default;
};

using HighScoreTable = int[16][18];   // character x track

class GameLogic
{
  public:
    static constexpr int kRaceHintBase = 0x618, kRaceHints = 32;   // race_strings 0x618 + n (the game's 0x1bbc2c)
    GameLogic(ModeHost &host, ModePages &pages, const ModeSources &sources) : host(host), pages(pages), sources(sources)
    {
    }
    virtual ~GameLogic() = default;
    GameLogic(const GameLogic &) = delete;
    GameLogic &operator=(const GameLogic &) = delete;

    // the front end
    virtual void characterPicked();                     // FE CHARSELECT's A (0x163b0): FE COURSE SELECT
    virtual int routeChoiceDefault(int) const           // FE COURSE SELECT's route choice opens on route 0 (0x1e544)
    {
        return 0;
    }
    virtual void speedPicked(int pilot, int track);     // FE SPEEDSELECT's A (0x33fa0): LOADER
    virtual int speedSelectHint() const
    {
        return 0x3d;
    }   // the fe string of FE SPEEDSELECT's "Select - [BUTA]" (0x34200)
    virtual int loaderHint() const;                     // the race_strings id of the loading screen's hint
    virtual void nextLoaderHint();                      // loader_next_hint 0x81be0

    // the race's rules
    virtual bool hasOpponents() const
    {
        return true;
    }  // drones on the grid, the position list
    virtual int droneSeats(int chosen) const
    {
        return chosen;
    }  // the drones on the grid (0x79250's craft count - 1); chosen: the editor's opponent count
    virtual bool hasPickups() const
    {
        return true;
    }
    virtual bool hasWeapons() const
    {
        return true;
    }    // also the HUD's power-up panel and shield
    virtual bool scoresPoints() const
    {
        return true;
    }  // race points, the HUD's score
    virtual bool hasLapLimit() const
    {
        return true;
    }   // the race ends after the laps
    virtual bool hasGridIntro() const
    {
        return true;
    }  // the first race after a load starts with the grid flyby
    virtual bool hasGhosts() const
    {
        return false;
    }    // the loaded and the session ghost
    virtual bool showsLapRecord() const
    {
        return false;
    }   // the HUD's "New lap record" (CGameHudTimeAttack 0x4c850)
    // the lap line sets the turbo to the race's whole allowance (Time Attack's update 0x429c0); the other modes only
    // add a lap's turbo per counted lap (race_craft_lap 0x3d5a0)
    virtual bool refillsTurboAtLapLine() const
    {
        return false;
    }
    virtual bool doublesDroneBlastBraking() const
    {
        return false;
    }   // 0x5e0ba: an explosion's push brakes the drones twice as hard (modes 0 / 1)

    // the race
    virtual void newRace()
    {
    }                           // the start, Restart, a new track (resetDriver)
    virtual void lapLineCrossed([[maybe_unused]] bool firstCrossing)
    {
    }  // the player's craft crossed the lap line (0x429c0)
    virtual void step()
    {
    }                              // one game frame of the mode's own work

    // the race end and the character's progress
    virtual int pilot() const;                          // the character whose upgrades, scores and cash count
    virtual HighScoreTable &highScores() const = 0;     // 0x95930
    virtual void earnCash([[maybe_unused]] int character, [[maybe_unused]] int amount)
    {
    }
    virtual bool paysCash() const
    {
        return false;
    }     // SCOREMENU's cash line (else the high score)
    virtual bool hasUpgrades() const
    {
        return false;
    }  // the race end's Upgrade Menu, the tournament's menus
    virtual void raceFinished([[maybe_unused]] bool won)
    {
    }              // after SCOREMENU / POINTSMENU, before WINMENU / LOSEMENU

  protected:
    ModeHost &host;
    ModePages &pages;   // the pages the picks lead to (logic\mode_pages.hpp)
    ModeSources sources;
};

// mode 0 (vtable 0x1a1bd8): five levels of races for one character (app_tourney.cpp)
class TournamentLogic : public GameLogic
{
  public:
    using GameLogic::GameLogic;
    void characterPicked() override;   // FE TOURNEYTYPE
    int pilot() const override;
    HighScoreTable &highScores() const override;   // 0x1bb108
    void earnCash(int character, int amount) override;
    bool paysCash() const override
    {
        return true;
    }
    bool hasUpgrades() const override
    {
        return true;
    }
    void raceFinished(bool won) override;          // 0x95d80: the tournament's progress
    int droneSeats(int chosen) const override;     // the nemesis race: only the nemesis (0x79250)
    bool doublesDroneBlastBraking() const override
    {
        return true;
    }
    int loaderHint() const override;
    void nextLoaderHint() override;

  private:
    static const int kHintBase = 0x5dc, kHints = 40;
    int hint = 0;   // race_strings 0x5dc + n (0x1bbc30, wraps after 39)
};

// mode 1 (vtable 0x1a18f8): one race, the picked character, track and speed
class QuickRaceLogic : public GameLogic
{
  public:
    using GameLogic::GameLogic;
    void characterPicked() override;   // FE COURSE SELECT, its cursor on the character's home track
    // Argon: a nemesis's home row opens the route choice on its home route (docs\argon\differences.md)
    int routeChoiceDefault(int row) const override;
    HighScoreTable &highScores() const override;   // 0x1bb150
    bool doublesDroneBlastBraking() const override
    {
        return true;
    }
};

// mode 4 (vtable 0x1a197c, gl_multiplayer.cpp; docs\frontend\multiplayer.md "Multiplayer"): 2..4 players in a split screen, no
// AI on the grid (FE CHARSELECT sets the craft count to the players', 0x78d60; the grid fill 0x79250 skips mode 4),
// weapons and pickups, no race points, cash or upgrades; the laps from the Multiplayer Laps option (0x3b0b0). The race
// ends in MULTIPLAYERENDMENU once every player has finished or at most one still races (0x41de0 / 0x620e0)
class MultiplayerLogic : public GameLogic
{
  public:
    using GameLogic::GameLogic;
    bool hasOpponents() const override
    {
        return false;
    }
    bool scoresPoints() const override
    {
        return false;
    }
    bool paysCash() const override
    {
        return false;
    }
    HighScoreTable &highScores() const override;   // never scores: Quick Race's table
};

// mode 3 (vtable 0x1a16f8; docs\frontend\game-modes.md "Time Attack"): the player alone (no opponents, weapons, pickups or race
// points), no lap limit (the HUD's "N / --"), no grid intro; left through the pause menu. Its object is larger than
// the others' (0xd4c4 bytes, 0xd39c) for the ghosts (logic\time_attack.cpp): the lap being recorded (+0xd410), the
// session ghost (the fastest lap so far, 0x52afae, replayed as the craft +0xce70) and the loaded ghost (FE GHOSTSELECT's
// pick, 0x52afaf, the craft +0xce74), both replayed from each lap's start
class TimeAttackLogic : public GameLogic
{
  public:
    using GameLogic::GameLogic;
    void speedPicked(int pilot, int track) override;   // FE GHOSTSELECT
    int speedSelectHint() const override
    {
        return 0x32;
    }
    int loaderHint() const override;
    void nextLoaderHint() override;

    bool hasOpponents() const override
    {
        return false;
    }
    bool hasPickups() const override
    {
        return false;
    }
    bool hasWeapons() const override
    {
        return false;
    }    // 0x49440 hides the panel, 0x4a100 skips the shield
    bool scoresPoints() const override
    {
        return false;
    }  // 0x49440 hides the score
    bool hasLapLimit() const override
    {
        return false;
    }
    bool hasGridIntro() const override
    {
        return false;
    }
    bool hasGhosts() const override
    {
        return true;
    }
    bool showsLapRecord() const override
    {
        return true;
    }
    bool refillsTurboAtLapLine() const override
    {
        return true;
    }

    void newRace() override;   // no lap recorded, no session ghost (+0xd410 / +0xd414 / 0x52afae); the loaded one stays
    void lapLineCrossed(bool firstCrossing) override;
    void step() override;
    HighScoreTable &highScores() const override;   // never scores: Quick Race's table

    // the ghosts: [0] the loaded one, [1] the session one
    void useLoadedGhost(ghost::Lap lap)
    {
        loadedLap = std::move(lap);
    }   // FE GHOSTSELECT (empty: none)
    const ghost::Lap &loadedGhost() const
    {
        return loadedLap;
    }
    bool hasSessionGhost() const
    {
        return sessionLap.valid();
    }
    const ghost::Lap &sessionGhost() const
    {
        return sessionLap;
    }
    ghost::Lap takeSessionGhost();             // leaving the race for FE GHOSTSAVE
    bool ghostShown(int ghost) const
    {
        return crafts[clampGhost(ghost)].shown;
    }
    float ghostJet(int ghost) const
    {
        return crafts[clampGhost(ghost)].jet;
    }
    glm::mat4 ghostMatrix(int index, float alpha) const;   // blended between the last two race steps, as driverMatrix
    // the pause menu's ghost replay (0x43770): only ghost i shown, at the replayed pose; the other one hidden
    void showOnlyGhost(int ghost, const glm::mat4 &pose);

  private:
    struct GhostCraft
    {
        bool shown = false, wasShown = false;
        float jet = 0;
        glm::mat4 prev{1.f}, cur{1.f};
    };
    static int clampGhost(int ghost)
    {
        return ghost < 0 ? 0 : ghost > 1 ? 1 : ghost;
    }
    const ghost::Lap &ghostLap(int index) const
    {
        return index == 0 ? loadedLap : sessionLap;
    }
    void recordSample();
    void placeGhostCrafts();

    static const int kHintBase = 0x64a, kHints = 16;
    int hint = 0;   // race_strings 0x64a + n (0x1bbc28, wraps after 15)
    ghost::Lap recording, sessionLap, loadedLap;
    bool recordingLap = false;
    GhostCraft crafts[2];
};

// one object per mode; the selected one is the race's (App::logic)
class GameModes
{
  public:
    GameModes(ModeHost &host, ModePages &pages, const ModeSources &sources)
        : tournamentLogic(host, pages, sources), quickRaceLogic(host, pages, sources), timeAttackLogic(host, pages, sources),
          multiplayerLogic(host, pages, sources)
    {
    }
    GameModes(const GameModes &) = delete;
    GameModes &operator=(const GameModes &) = delete;

    TournamentLogic &tournament()
    {
        return tournamentLogic;
    }
    QuickRaceLogic &quickRace()
    {
        return quickRaceLogic;
    }
    TimeAttackLogic &timeAttack()
    {
        return timeAttackLogic;
    }
    MultiplayerLogic &multiplayer()
    {
        return multiplayerLogic;
    }
    bool multiplayerSelected() const
    {
        return selected == &multiplayerLogic;
    }
    bool tournamentSelected() const
    {
        return selected == &tournamentLogic;
    }
    void select(GameLogic &mode)
    {
        selected = &mode;
    }
    GameLogic &current() const
    {
        return *selected;
    }

  private:
    TournamentLogic tournamentLogic;
    QuickRaceLogic quickRaceLogic;
    TimeAttackLogic timeAttackLogic;
    MultiplayerLogic multiplayerLogic;
    GameLogic *selected = &quickRaceLogic;
};
