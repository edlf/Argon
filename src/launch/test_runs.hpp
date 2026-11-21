// Argon: the start-up's test runs (test_runs.cpp): the turbo test, the AI drive's pre-run with its reports, the ghost
// replay. Used by ViewerStartup; they only run when the command line asks for them.
#pragma once
#include "launch/launch_options.hpp"

class Race;
class ReplayRace;
class StartGrid;
class Weapons;
struct RaceScoring;
namespace argon
{
class Audio;
class GameCraft;
class Particles;
class Pickups;
class WakeSpray;
class WeaponSprites;
class Route;
struct AiGraph;
}  // namespace argon

// what the test runs read and change
struct TestRunSources
{
    argon::GameCraft &player;          // racer 0 (App::sim): the turbo test
    const argon::Route &route;         // no route: no drive
    bool &driving, &chaseCam;
    const ReplayRace &replayRace;
    const argon::Audio &audio;         // the engine curve's input
    const argon::WakeSpray &wakeSpray;
    const argon::WeaponSprites &weaponSprites;
    StartGrid &startGrid;              // the player's pilot
    const int &speedClass;
    const argon::Pickups &pickups;
    const int (&powerLevels)[3];       // the player's red / blue / yellow levels (App::hudWeapon)
    const int &weaponSpecial;
    const argon::Particles &particles;
    const RaceScoring &scoring;
    const Race &race;
    const argon::AiGraph &aiGraph;     // the route table
    const Weapons &weapons;
};

// what the test runs ask App for: the drive stepped, the chase camera settled, a ghost replay started
class TestRunHost
{
  public:
    virtual void stepDrive(float dt) = 0;
    virtual void settleDrive(float dt) = 0;
    virtual void startGhostReplay(int ghost) = 0;

  protected:
    ~TestRunHost() = default;
};

class TestRuns
{
  public:
    TestRuns(TestRunHost &host, const TestRunSources &sources, const LaunchOptions &options) : host_(host), sources_(sources), options_(options)
    {
    }
    void applyTurboTest();   // --turbo-test s
    void preRunDrive();      // --drive / --drive-time: the AI drive, pre-run, then its report

  private:
    void preRunGhostReplay();
    void printDriveState(int frame) const;
    void printDriveReport() const;
    void printRaceReport() const;
    void printRouteTable() const;
    void printForksTaken() const;

    TestRunHost &host_;
    TestRunSources sources_;
    const LaunchOptions &options_;
};
