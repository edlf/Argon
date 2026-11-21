// Start-up: finds the game, opens the window and renderer, then starts where the command line says: the editor's
// start scene, or the front end, whose test pages and races are TestStart's (test_start.hpp). The test runs the
// command line asks for (the AI drive's pre-run, its reports) are TestRuns' (test_runs.hpp).
#pragma once
#include "launch/launch_options.hpp"
#include "launch/test_runs.hpp"

class App;

class ViewerStartup
{
  public:
    ViewerStartup(App &app, LaunchOptions options);

    bool locateGame();   // false: no game files found (already reported)
    void loadGameLists();
    void loadSettings();
    void initAudio();
    void openWindow();
    void initRenderer();
    void enterStartScene();
    void preRunDrive()
    {
        testRuns_.preRunDrive();
    }

  private:
    bool startsInMenu() const;
    void placeCamera();

    App &app_;
    LaunchOptions options_;
    TestRuns testRuns_;
};
