// Argon: the App side of the front end: the shell's and its pages' sources, opening it over a race or the editor (and
// the editor from FE MAIN's Game Demos row), the mouse cursor and the scene transition. The shell's draw: ui\menu.cpp;
// its page models and menu bars: app\fe_shell_drawing.cpp; the pages: ui\*.cpp. Notes: docs\frontend\README.md.
#include "app/app.hpp"
#include "ui/fe_shell.hpp"
#include "ui/fe_shell_sources.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

FeShellSources App::feShellSources()
{
    return FeShellSources{strings, painter, audio, input, settingsMenu, modes, multiplayer, frameCopies, story, loadingFlyby, game,
                          FeModels::Sources{game, models, renderer, renderer.propPool, props, startGrid}};
}

FePageSources App::fePageSources()
{
    return FePageSources{
        MainMenuPage::Services{*this, modes, multiplayer, profiles, tourneyRun, lapRecords.fastestLapPending},
        PilotPicks{picks.character, picks.courseRow, progressKeeper, modes, progress, input, multiplayer, *this, tests.seats},
        LoadingSources{*this, story, loadingFlyby, picks.loadPilot, picks.loadTrackId, input, multiplayer, modes, progress, tourneyRun, replayRace, progressKeeper, game, playerPads},
        *this,
        ChooseSavePage::Services{*this, profiles, profileSwitcher, game},
        NameEntryPage::Services{*this, profiles, profileSwitcher, cheatCodeEntry, viewerSettings},
        OptionSources{*this, settingsKeeper, settingsMenu, customMusic, musicStore, profiles, game},
        KeyBindingsPage::Services{settingsKeeper, input},
        RecordSources{ghosts, ghostShelf, lapRecords.table, lapRecords.newRecords, lapRecordSaver, profiles, progress, picks.speedClass, picks.character, progressKeeper},
        progress,
        progressKeeper,
        customMusic,
        musicStore,
        SpeedSelectPage::Services{picks.speedClass, progressKeeper, modes},
        TrackInfoPage::Services{progressKeeper, game, applied.speedMph},
        TourneyTypePage::Services{progress, picks.character, progressKeeper, *this, game},
        GhostSelectPage::Services{ghosts, modes, *this, ghostShelf},
        ghosts,
        ghostShelf,
        IntroScreensPage::Services{*this, movie, game, viewerSettings},
        tourneyRun};
}

void App::openMenu()
{
    lapRecords.load();   // Track Records and the race's lap records (app_laprecords.cpp)
    // the editor / race only runs once a track is picked: leaving it for the menu unloads its track
    if (trackIndex >= 0)
    {
        editor.rememberTrack(trackIndex);
        loadTrack(-1);
        startTransition();   // 0x30b86: back to the front end
    }
    if (session.paused)
    {
        resumeRace();
    }
    leaveEditorMode();
    switchOffRaceEggs();
    audio.stopAll();
    session.driving = false;
    session.playerDrive = false;
    showPanel = false;
    fe->open();
}

// the debug view starts silent (the editor's Audio window unmutes it); the game's own modes always play sound
void App::enterEditorMode()
{
    editor.enter();
    audio.muted = true;
}

void App::leaveEditorMode()
{
    editor.leave();
    audio.muted = false;
}

// outside the editor nothing takes the mouse: the cursor stays hidden (a right-drag's capture is left alone)
void App::showCursorInEditorOnly()
{
    ImGuiIO &io = ImGui::GetIO();
    const int mode = glfwGetInputMode(window, GLFW_CURSOR);
    if (mode == GLFW_CURSOR_DISABLED)
    {
        return;
    }
    if (editor.active())
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        return;
    }
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    if (mode != GLFW_CURSOR_HIDDEN)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
}

// Game Demos -> the editor, on its last track (or the --track / default one)
#if ARGON_DEVELOP
void App::startEditor()
{
    enterEditorMode();
    int idx = editor.lastTrack();
    if (idx < 0)
    {
        for (int i = 0; i < int(trackList.size()); ++i)
        {
            if (trackList[i] == "bracken")
            {
                idx = i;
            }
        }
    }
    loadTrack(std::max(0, idx));
}
#endif

void App::closeMenu()
{
    fe->close();
    showPanel = true;
}

void App::quit()
{
    glfwSetWindowShouldClose(window, GLFW_TRUE);   // run() saves the profile
}

// the race menus' bars are the shell's (app\fe_shell_drawing.cpp)
void App::drawMenuBars(const FeCanvas &canvas, float vw)
{
    shellDrawing.drawMenuBars(canvas, vw);
}

// the scene transition (render\frame_copies.hpp), not in a --shot run: its shots stay clean
void App::startTransition()
{
    if (!screenshotRun())
    {
        frameCopies.startTransition();
    }
}
