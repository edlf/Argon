// Argon: where a test run starts in the front end (the command line's --menu-page, --message, --route-choice and
// the races they jump into: --quick-race, --replay, --tourney-race with --race-go, the race-end and pause pages);
// ViewerStartup opens it once the start scene is up. None of it is the game's: with no test options the front end
// opens on its first page (the intro videos or the title).
#pragma once
#include "launch/launch_options.hpp"

class FeShell;
class GameModes;
class GameStrings;
class ProgressKeeper;
class Race;
class RaceMenus;
class RaceStart;
class TestRuns;
struct TestScript;
class ViewerSettings;
struct TourneyRun;
namespace argon
{
struct GameProgress;
}
namespace menu
{
class PadWatch;
}

struct TestStartSources
{
    FeShell &fe;
    argon::GameProgress &progress;      // the tests' tournament progress, cash and upgrades
    GameModes &modes;
    ProgressKeeper &progressKeeper;     // the tournament's next track
    TourneyRun &tourneyRun;             // its character, the awards to show
    int &character, &course;            // FE CHARSELECT's pick, FE COURSE SELECT's row
    int &speedClass;
    RaceMenus &raceMenus;               // the race-end and pause pages
    menu::PadWatch &padWatch;           // --pause 4's controller pulled out
    Race &race;                         // --first-branch
    RaceStart &raceStart;               // --intro-at, --countdown-at
    const GameStrings &strings;         // --message's texts
    const ViewerSettings &settings;     // skipIntroVideos
    TestScript &tests;                  // --story-at
    bool &playerDrive, &simStarted;
    int &raceEnd;                       // the race-end page up (ui\race_end_page.hpp)
    TestRuns &testRuns;                 // --turbo-test
};

// what only App can do for a test start
class TestStartHost
{
  public:
    virtual void openMenu() = 0;
    virtual bool startAttractDemo(int number) = 0;
    virtual void startQuickRace(int pilotId, int trackId) = 0;
    virtual void loadRaceNow() = 0;      // blocking: the loading screen's race load at once
    virtual void finishQuickRace() = 0;  // the loading screen left for the race
    virtual void quitRaceTo(int page) = 0;
    virtual void computeScore() = 0;
    virtual void pauseRace() = 0;
    virtual int upgradeChar() const = 0;
    virtual bool screenshotRun() const = 0;

  protected:
    ~TestStartHost() = default;
};

class TestStart
{
  public:
    TestStart(const TestStartSources &sources, TestStartHost &host, const LaunchOptions &options)
        : sources_(sources), host_(host), menu_(options.menu), race_(options.raceTest)
    {
    }
    // the front end opened on the page the options ask for, then their message box and test race
    void open();

  private:
    void openFirstPage();
    void openMenuPage(int page);
    void giveTestTourneyProgress();   // menu page shots: --speed, --tourney-done, --trophy
    bool openOptionsSubpage(int page);
    void openArgonOptionsPage(int page);
    void openMessageBox();
    void startTestRace();
    void setUpTestRace();
    void showPauseTest(int page);     // --pause 3 / 4

    TestStartSources sources_;
    TestStartHost &host_;
    MenuStart menu_;
    RaceTestStart race_;
};
