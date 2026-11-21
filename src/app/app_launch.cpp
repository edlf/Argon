// Argon: the command line's settings for the viewer itself (launch\launch_options.hpp ViewerLaunch), taken once
// after parsing and before the start-up. --multiplayer goes before --split and --time-attack, whatever their order.
#include "app/app.hpp"
#include "launch/launch_options.hpp"
#include "ui/fe_shell.hpp"
#include "ui/race_menus.hpp"

void App::useLaunchSettings(const ViewerLaunch &launch)
{
    tests = launch.tests;
    if (launch.mute)
    {
        audioEnabled = false;
    }
    useRendererRequest(launch.renderer);
    useShotRequest(launch.shot);
    useSceneRequest(launch.scene);
    useProfileRequest(launch.profile);
    useRaceRequest(launch.race);
    useMenuPicks(launch.menu);
}

void App::useRendererRequest(const RendererRequest &request)
{
    rendererRequest = request;
}

void App::useShotRequest(const ShotRequest &request)
{
    shotRun.use(request);
}

void App::useSceneRequest(const SceneRequest &request)
{
    toggles.alphaMode = request.alphaMode.value_or(toggles.alphaMode);
    toggles.frontCounterClockwise = request.frontCounterClockwise.value_or(toggles.frontCounterClockwise);
    toggles.trailMode = request.trailMode.value_or(toggles.trailMode);
    toggles.gameMaterials = toggles.gameMaterials && !request.noMaterials;
    toggles.gameCull = toggles.gameCull && !request.noCull;
    toggles.groups[G_SKY] = toggles.groups[G_SKY] && !request.noSky;
    toggles.emitters = toggles.emitters || request.emitters;
    applied.showHud = applied.showHud && !request.noHud;
    showPanel = showPanel && !request.noPanel;
    dumpMaterials = dumpMaterials || request.dumpMaterials;
}

// --egg i switches egg i on whatever the profile says (TestScript::eggs keeps it on through the profile's load)
void App::useProfileRequest(const ProfileRequest &request)
{
    progressKeeper.unlockAll = progressKeeper.unlockAll || request.unlockAll;
    if (request.newProfile)
    {
        progress.characters = 0x007f;   // the seven original characters
        progress.speeds = 1;
    }
    for (int egg = 0; egg < RaceEggs::kCount; ++egg)
    {
        if (tests.eggs & (1u << egg))
        {
            progress.eggsOn[size_t(egg)] = true;
        }
    }
}

void App::useRaceRequest(const RaceRequest &request)
{
    picks.speedClass = request.speedClass.value_or(picks.speedClass);
    if (request.driveView)
    {
        driveView.choose(*request.driveView);
    }
    if (request.multiplayerPlayers)
    {
        startMultiplayer(*request.multiplayerPlayers);   // game mode 4
    }
    if (request.splitViews)
    {
        multiplayer.seatPlayers(*request.splitViews);
    }
    if (request.timeAttack)
    {
        modes.select(modes.timeAttack());
    }
    if (request.timeLimit)
    {
        raceTime.setLimit(*request.timeLimit);
    }
    if (request.attractIdleSeconds)
    {
        attractIdle.setIdleSeconds(*request.attractIdleSeconds);
    }
    if (request.powerups)
    {
        for (size_t k = 0; k < request.powerups->size(); ++k)
        {
            powerUps.levels[k] = tests.startPowerups[k] = (*request.powerups)[k];
        }
    }
}

void App::useMenuPicks(const MenuPicks &menuPicks)
{
    picks.courseRow = menuPicks.course.value_or(picks.courseRow);
    picks.character = menuPicks.character.value_or(picks.character);
    if (menuPicks.mainMenuRow)
    {
        fe->mainMenu().select(*menuPicks.mainMenuRow);
    }
    for (const int recordTrack : menuPicks.newRecords)
    {
        lapRecords.newRecords[size_t(recordTrack)] = true;
    }
    if (menuPicks.freezePoses)
    {
        fe->models().freezePoses();
    }
}

TestStartSources App::testStartSources(TestRuns &testRuns)
{
    return TestStartSources{*fe,
                            progress,
                            modes,
                            progressKeeper,
                            tourneyRun,
                            picks.character,
                            picks.courseRow,
                            picks.speedClass,
                            *raceMenus,
                            padWatch,
                            race,
                            raceStart,
                            strings,
                            viewerSettings,
                            tests,
                            session.playerDrive,
                            session.simStarted,
                            session.raceEnd,
                            testRuns};
}
