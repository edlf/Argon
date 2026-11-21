// Argon: the frame's 2D layer: the front end, or the race's HUD and menus and the editor panel.
#include "app/app.hpp"
#include "ui_extras/editor_panel.hpp"
#include "hud/game_hud.hpp"
#include "ui/fe_shell.hpp"
#include "ui/race_menus.hpp"
#include "movie/story_scene.hpp"

ModeSources App::modeSources()
{
    return ModeSources{progress, progressKeeper, tourneyRun, picks.character, picks.courseRow, picks.loadPilot, picks.loadTrackId,
                       picks.speedClass, raceHint, sim, simCur, exhausts};
}

// App owns the front end's pages, the race's menus and the HUD; they refer back to it
App::App()
    : fe(std::make_unique<FeShell>(feShellSources(), fePageSources(), shellDrawing)), raceMenus(std::make_unique<RaceMenus>(*this, raceMenuSources())),
      modes(*this, *fe, modeSources()),
      story(std::make_unique<StoryScene>(*this, StoryScene::Sources{audio, game, models, renderer.propPool, viewerSettings, tests.storySceneAt, sceneFrame, fe->menus()})),
      weaponHost(*this, weaponHostSources()), weapons(weaponHost, Weapons::Services{race, raceSounds, particles, weaponSprites, viewerRand, tests, playerPads})
{
    for (int p = 0; p < argon::split::kMaxPlayers; ++p)
    {
        huds[size_t(p)] = std::make_unique<GameHud>(*this, hudSources(), p);
    }
}
App::~App() = default;

RaceMenusSources App::raceMenuSources()
{
    return RaceMenusSources{strings,  fe->menus(),        painter,    audio,        session.raceEnd,        session.paused,  pauseBackground,
                            race,     aiGraph,        modes,      settingsMenu, settingsKeeper, scoring,        session.hudLaps, picks.speedClass,
                            progress, progressKeeper, tourneyRun, profiles,     viewerSettings, weapons, multiplayer, playerPads};
}

GameHudSources App::hudSources()
{
    return GameHudSources{painter,      fe->menus(),    strings,    viewerSettings, multiplayer, race,      sim,      weapons,    modes,    scoring,
                          raceWarnings, lapRecords, raceStart,  raceEggs,       padWatch,    hudCamera, track,    powerUps.caps, simClock, session.turboGrantT,
                          speedReadout,     session.hudLaps,    picks.speedClass, session.raceEnd,        powerUps.specialShown,  weaponsOn, applied.speedMph, applied.showMap,
                          craftPoses,
                          playerPads};
}

WeaponHostSources App::weaponHostSources()
{
    return WeaponHostSources{simClock,    simWorld,  sim,           simCur,      race,    weapons,        props,
                             startGrid,   powerUps.levels, powerUps.special, powerUps.specialShown,  session.hudLaps, picks.speedClass,     picks.loadTrackId,
                             session.playerDrive, progress,  replayRace,    multiplayer, scoring, playerKnockOut, audio};
}

void App::drawUi()
{
    frameCopies.fitToWindow();
    renderer.prepareScreenPasses(rendererRequest.postAntiAliasing.value_or(viewerSettings.postAntiAliasing()),
                                 rendererRequest.orderIndependent.value_or(viewerSettings.orderIndependentTranslucency()));
    if (ImGui::IsKeyPressed(ImGuiKey_F11, false))
    {
        toggleFullscreen();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
    {
        toggleFpsMeter();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F12, false))
    {
        screenshots.request();
    }
    editorLoad.draw(painter);
    if (fpsMeterShown())
    {
        fpsMeter.draw(renderer.device());
    }
    if (fe->isOpen())
    {
        fe->draw();
        return;
    }
    if (story->playing())
    {   // a tournament's end scene (0x7fa50): the scene alone over the race's track
        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        const float pixelsPerUnit = ds.y / 480.f;
        frameCopies.drawTransition(ImGui::GetBackgroundDrawList());
        story->drawOverlay(FeCanvas(painter, ImGui::GetBackgroundDrawList(), pixelsPerUnit, (ds.x / pixelsPerUnit - 640.f) * 0.5f));
        return;
    }
    drawSpeedBlur();                                  // over the 3D view, under the HUD (0x93490)
    frameCopies.drawTransition(ImGui::GetBackgroundDrawList());   // over the 3D view, under the HUD
    // POSTABLE / the menus: "set none"; the pause screen clears it (0x7e5e0), and its blur is taken without it
    const bool playerOneOut = weapons.craft(0).knockedOut() && !playerPads.multiplayerRace();   // split screen: each HUD checks its own craft
    if (applied.showHud && session.driving && session.simStarted && session.chaseCam && !session.raceEnd && (!session.paused || padWatch.holdsMenu()) && !playerOneOut && !replayHidesHud())
    {
        renderHuds();
    }
    if (session.driving && session.simStarted && !session.paused)
    {
        drawKnockOutMessages();
    }
    drawPause();
    if (!replayRace.playing())
    {
        drawRaceEnd();
    }
#if ARGON_DEVELOP
    drawDeveloperTools();
#endif
}

#if ARGON_DEVELOP
// the developer build's own keys and the editor's windows
void App::drawDeveloperTools()
{
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
    {
        showPanel = !showPanel;   // F1 hides / shows the panel
    }
    // viewer only: T adds 5 s of turbo (the game's only refill is the lap line's, docs\physics\turbo-flaps-cameras.md "Turbo")
    if (session.driving && !ImGui::GetIO().WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_T, false))
    {
        sim.addTurbo(5.f);
    }
    if (showPanel)
    {
        EditorPanel(*this, editorPanelSources()).draw();
    }
}

EditorPanelSources App::editorPanelSources()
{
    return EditorPanelSources{toggles,
                              editor.windows(),
                              trackList,
                              trackIndex,
                              pendingTrack,
                              track,
                              sky,
                              renderer.presenter(),
                              renderer.device(),
                              startGrid,
                              audio,
                              audioEnabled,
                              enginePilot,
                              routeReport,
                              route,
                              session.driving,
                              session.chaseCam,
                              session.playerDrive,
                              applied.showHud,
                              powerUps.specialShown,
                              pickupLogic,
                              powerUps.capsFromUpgrades,
                              drive.engineSoundFollows,
                              session.simStarted,
                              sim,
                              driveView,
                              powerUps.levels,
                              powerUps.special,
                              powerUps.editorLevelCap,
                              picks.speedClass,
                              powerUps.caps,
                              drive.timeScale,
                              drive.aiPower,
                              race,
                              craftStats,
                              freeCamera};
}
#endif
