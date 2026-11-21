// Argon: a race played from a recording (docs\replays\README.md "Playback"): the crafts are placed from the
// recording each frame instead of flying (craft_replay_inputs 0x61c60), by the race clock from the countdown's start
// (race_clock_reset 0x3f1d0 starts the game's recording there). Three users, as the game's:
// - the attract demo (attract_init 0x40440 / attract_update 0x40af0): d:\replays\replay<n> behind the loading screen;
// - View Replay (WINMENU / LOSEMENU, 0x3e8e0 / 0x3e700): the race's own recording from the grid;
// - the pause menu's ghost replays (Time Attack, GAMELOGIC 3 / 4 -> 0x43460 / 0x43770): one ghost's lap alone.
// The replayed crafts' laps are counted as the race manager would (the replay's own report), the race's results are
// left alone. The way back (the front end, the race-end menu, the restart) is App's (app_replays.cpp).
#pragma once
#include "craft/craftsim.hpp"
#include "logic/ghost.hpp"
#include "logic/lap_tally.hpp"
#include "logic/replay.hpp"
#include "logic/replay_weapons.hpp"
#include "logic/replay_writer.hpp"

#include <string>

class DriveCamera;
class GameModes;
class Race;
class StartGrid;
class Weapons;

// what a replay reads and places
struct ReplaySources
{
    const std::string &game;     // <game>\replays
    Weapons &weapons;
    StartGrid &startGrid;        // the recording's pilots
    Race &race;                  // its drones, placed from the recording
    const argon::AiGraph &aiGraph;
    const int &hudLaps;
    GameModes &modes;            // Time Attack's ghost shown alone
    argon::GameCraft &sim;       // the player's craft
    glm::mat4 &simPrev, &simCur;
    argon::GameCraft::Tripod &camPrev, &camCur;
    const DriveCamera &driveView;
};

// what a replay asks App for: the race it plays, the race's steps it keeps, the replayed weapons and controls
class ReplayHost
{
  public:
    virtual void startQuickRace(int pilotId, int trackId) = 0;
    virtual void stepRaceIntro(float dt) = 0;
    virtual bool gridIntroRunning() const = 0;
    virtual bool raceHeld() const = 0;
    virtual void stepEffectsClock() = 0;
    virtual void stepReplayedWeapons() = 0;
    virtual void showReplayedControls(int slot, uint32_t controls) = 0;
    virtual void playWeaponEvent(const argon::replay::WeaponEvent &event) = 0;

  protected:
    ~ReplayHost() = default;
};

class ReplayRace
{
  public:
    enum class Kind
    {
        AttractDemo,
        ViewReplay,
        GhostLap
    };

    ReplayRace(ReplayHost &host, const ReplaySources &sources) : host(host), sources(sources)
    {
    }
    ReplayRace(const ReplayRace &) = delete;
    ReplayRace &operator=(const ReplayRace &) = delete;

    // <game>\replays\replay<number>: its track and pilots as a Quick Race behind the loading screen; false (and a
    // message on stderr) when the files cannot be read
    bool startAttractDemo(int number);
    // the race's own recording, the crafts already back on the grid; goStep: the race's first clock step after the
    // countdown (RaceRecording::Timing). False (and a message) when it does not parse
    bool startViewReplay(const argon::replay::RecordingFiles &files, int goStep);
    // a Time Attack ghost's lap (0 the loaded, 1 the session ghost) from its start, the player's craft hidden
    void startGhostLap(const ghost::Lap &lap, int ghost);
    void stop();
    void step();   // one 60 Hz race step in place of the crafts' physics

    bool playing() const
    {
        return playing_;
    }
    bool ended() const
    {
        return ended_;
    }   // ran out (or stopped by a button): App takes the way back, then clearEnded
    void clearEnded()
    {
        ended_ = false;
    }
    Kind kind() const
    {
        return kind_;
    }
    int ghost() const
    {
        return ghost_;
    }
    bool hidesPlayerCraft() const
    {
        return playing_ && kind_ == Kind::GhostLap;
    }   // 0x43770: the player's craft +0x1129, the replayed ghost shown
    float clock() const
    {
        return clock_;
    }
    float endTime() const
    {
        return recording.endTime();
    }

  private:
    void begin(Kind kind);
    void useRecordingGrid();
    void countLaps();
    void countLapsOf(int racer, argon::GameCraft &craft);
    void reportFinish(const char *what) const;
    void placeCrafts();
    void placePlayerCraft(const argon::replay::Pose &pose);
    void easeOnTheGrid(int racer, argon::replay::Pose &pose) const;
    void playWeaponEvents();
    bool effectsClockRuns() const;
    void finish();

    ReplayHost &host;
    ReplaySources sources;
    argon::replay::Recording recording;
    Kind kind_ = Kind::AttractDemo;
    int ghost_ = -1;
    bool playing_ = false, ended_ = false, reported_ = false;
    float clock_ = 0;   // 0 at the countdown's start
    LapTally tally;
    argon::replay::ShotTally shots;   // the weapon entries played, to compare with the race's own
    size_t nextWeapon_ = 0;        // the next weapon entry to play
    int steps_ = 0, goStep_ = 0;   // clock steps played; View Replay: the race's first one after the countdown
};
