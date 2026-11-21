// Argon: per frame: the free camera, what the frame's cameras and recorder read (render\view_cameras.hpp,
// render\frame_recorder.hpp), the frame's outcome (--shot, F12), main loop.
#include "app/app.hpp"
#include "gfx/vulkan/vulkan_handles.hpp"
#include "movie/story_scene.hpp"
#include "ui/fe_shell.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>

namespace
{
float keyHeld(GLFWwindow *window, int key)   // 1 while held, else 0
{
    return glfwGetKey(window, key) == GLFW_PRESS ? 1.f : 0.f;
}
}  // namespace

// the mouse and the free-camera keys move the camera in the debug view (the editor) only; the game's own modes never
// read them
void App::updateCamera(float dt)
{
    ImGuiIO &io = ImGui::GetIO();
    bool down = editor.active() && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (session.driving && session.chaseCam)
    {           // chase camera owns the view; right mouse takes it back
        if (down && !io.WantCaptureMouse)
        {
            session.chaseCam = false;
        }
        else
        {
            return;
        }
    }
    glm::dvec2 cursor;
    glfwGetCursorPos(window, &cursor.x, &cursor.y);
    if (down && !freeCamera.dragging() && io.WantCaptureMouse)
    {
        down = false;
    }
    if (down)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (freeCamera.dragging())
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    freeCamera.followCursor(cursor, down);
    if (!editor.active() || io.WantCaptureKeyboard || !input.focused())
    {
        return;
    }
    if (session.playerDrive && session.driving && session.chaseCam)
    {
        return;   // the keys steer the ship
    }
    const float along = keyHeld(window, GLFW_KEY_W) - keyHeld(window, GLFW_KEY_S);
    const float across = keyHeld(window, GLFW_KEY_D) - keyHeld(window, GLFW_KEY_A);
    const float rise = keyHeld(window, GLFW_KEY_E) - keyHeld(window, GLFW_KEY_Q);
    freeCamera.move({along, across, rise}, dt, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
}

// the loading screen's flyby (app\app_sequence.cpp) or story scene (movie\story_scene.cpp) draws the track
bool App::loaderFlybyShown() const
{
    return fe->isOpen() && fe->current() == PageLoader && (loadingFlyby.on() || story->playing()) && trackIndex >= 0;
}

bool App::frontEndScreenShown() const
{
    return fe->isOpen() && !loaderFlybyShown();
}

bool App::frontEndOpen() const
{
    return fe->isOpen();
}

bool App::modeHasPickups() const
{
    return logic().hasPickups();
}

SceneSources App::sceneSources()
{
    return SceneSources{renderer.device(),
                        renderer.presenter(),
                        renderer.trackPool,
                        renderer.waterPipe,
                        renderer.meshDraws,
                        renderer.sceneBuffers,
                        renderer.flares,
                        *renderer.translucency,
                        renderer.shadows,
                        renderer.effectBuffers,
                        toggles,
                        track,
                        sky,
                        hudCamera,
                        simClock,
                        sceneFrame.slots().frame,
                        sceneFrame.slots().view,
                        trackSerial,
                        sceneFrame.sectorMeshes(),
                        tests.hideMeshes,
                        race,
                        modes,
                        startGrid,
                        driveView,
                        raceEggs,
                        weapons,
                        session.driving,
                        session.chaseCam,
                        weaponsOn,
                        props,
                        models,
                        pickups,
                        pickupFor,
                        pickupTakenFor,
                        weaponSprites,
                        wakeSpray,
                        particles,
                        exhausts,
                        story,
                        frameCopies,
                        craftPoses};
}

// a shot owns the main view while the drive camera (with roll) does in a race, and over the loading screen's flyby
// and a story scene; the race views per human player while a race is on screen
ViewMoment App::viewMoment() const
{
    ViewMoment moment;
    moment.shotOwnsView = (viewOverride && session.driving && session.chaseCam) || loaderFlybyShown() || story->playing();
    moment.raceViewsShown = session.driving && !frontEndScreenShown();
    const int chased = std::min({multiplayer.players(), int(race.grid().size()) + 1, int(moment.playerShots.size()) + 1});
    for (int player = 1; player < chased; ++player)
    {
        moment.playerShots[size_t(player - 1)] = chaseShotOf(player);
    }
    return moment;
}

FrameSources App::frameSources()
{
    return FrameSources{renderer,
                        sceneFrame.slots().frame,
                        toggles,
                        track,
                        raceEggs,
                        story,
                        movie,
                        frameCopies,
                        *fe,
                        models,
                        engineTrails,
                        toggles.trails && session.driving && session.simStarted,
                        cameras,
                        sceneFrame,
                        *this,
                        sceneFrame.slots().view,
                        hudCamera,
                        shellDrawing.frameModels()};
}

HudCamera App::playerHudCamera(int player) const
{
    return ViewCameras::hudCameraOf(cameras.raceView(viewMoment(), player, renderer.presenter().windowSize()).camera);
}

void App::run()
{
    auto last = std::chrono::steady_clock::now();
    FrameOutcome outcome = FrameOutcome::Presented;
    while (!glfwWindowShouldClose(window) && outcome != FrameOutcome::Captured)
    {
        glfwPollEvents();
        if (!input.focused())
        {
            waitForFocus();
            last = std::chrono::steady_clock::now();   // no time jump on return
            continue;
        }
        retiredTracks.freeFinished(renderer.presenter().framesSubmitted());
        matchFontsToLanguage();
        if (pendingTrack >= 0 && !editorLoad.loading())
        {
            startEditorLoad(pendingTrack);
            pendingTrack = -1;
        }
        editorLoad.step();
        buildFrameUi(frameDeltaTime(last));
        outcome = renderFrame();
    }
    session.driving = false;      // stop the pad's motors
    updateVibration();
    renderer.device().waitForIdle();
    if (!screenshotRun())
    {
        profiles.saveCurrent();   // the menu's Exit or the window closed (Alt+F4)
    }
}

// the window lost the focus: everything holds (the race opens its pause menu, all sound stops where it is, the menus'
// music too: docs\argon\differences.md) until it is back. The GLFW clock, which the menus' timers, the movies, the
// animations and ImGui's time read, is set back to where it stopped, so none of them jumps the time away
void App::waitForFocus()
{
    if (session.driving && session.simStarted && session.playerDrive && !session.raceEnd && !session.paused)
    {
        pauseRace();
    }
    audio.suspendOutput();
    const double unfocusedAt = glfwGetTime();
    while (!glfwWindowShouldClose(window) && !input.focused())
    {
        glfwWaitEventsTimeout(0.25);
    }
    glfwSetTime(unfocusedAt);
    audio.resumeOutput();
}

// the step for this frame (at most 0.1 s); the FPS meter keeps the unclamped time
float App::frameDeltaTime(std::chrono::steady_clock::time_point &last)
{
    const auto now = std::chrono::steady_clock::now();
    const float raw = std::chrono::duration<float>(now - last).count();
    last = now;
    fpsMeter.recordFrame(raw);
    return std::min(0.1f, raw);
}

void App::buildFrameUi(float dt)
{
    renderer.ui().newFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    updateFrame(dt);
    showCursorInEditorOnly();
    // this frame's camera for the HUD's projected markers (recordFrame sets the same again): the HUD is built
    // before the frame is recorded, so last frame's matrix would trail the moving camera
    hudCamera = ViewCameras::hudCameraOf(cameras.sceneCamera(viewMoment(), windowAspect(renderer.presenter().windowSize())));
    drawUi();
    drawBrightness();
    ImGui::Render();
}

// the front end's music: frontend_music + ui_amb3 on the menu pages, levelload_music + ui_levelload on the loading
// screen, none in the race / editor or under the intro videos
void App::updateFrame(float dt)
{
    const bool menus = fe->isOpen();
    if (!menus && story->playing())
    {
        updateRaceEndScene();
        updateVibration();
        return;
    }
    if (!menus && replayRace.playing())
    {
        updateReplayInput();
    }
    else if (!menus)
    {
        updatePause();
        updateRaceEnd();
    }
    if (menus)
    {
        stepAttractIdle(dt);
        fe->update(dt);
    }
    else if (!session.paused)
    {
        updateCamera(dt);
        stepDrive(dt);
    }
    updateVibration();
    if (!fe->isOpen() && !session.paused)
    {
        audio.update(dt, freeCamera.position, freeCamera.forward(), glm::vec3(0, 1, 0));
    }
    audio.setFrontEnd(game, frontEndMusicMode());
    if (!session.paused)
    {
        audio.updateFrontEnd();
    }
}

int App::frontEndMusicMode() const
{
    if (!fe->isOpen() || fe->current() == PageIntroScreens)
    {
        return 0;
    }
    if (fe->current() == PageLoader)
    {
        return story->playing() ? 0 : 2;   // 0x9a810 stops the loading loop for a story scene (0xca9b0: its voices +0x964)
    }
    return 1;
}

App::FrameOutcome App::renderFrame()
{
    if (!renderer.presenter().beginFrame())
    {
        adoptRemadeWindow();
        return FrameOutcome::Skipped;
    }
    sceneFrame.slots().frame = renderer.presenter().frameSlot();
    const bool shotRunEnds = shotRun.countFrame();
    FrameRecorder(frameSources(), *this).record();
    const bool keepRequested = screenshots.wantsFrame();
    renderer.presenter().endFrame(shotRunEnds || keepRequested ? argon::gfx::FrameEnd::ShowAndKeep : argon::gfx::FrameEnd::Show);
    sceneFrame.slots().frame = renderer.presenter().frameSlot();
    adoptRemadeWindow();
    if (shotRunEnds)
    {
        screenshots.saveFrame(renderer.presenter(), shotRun.path());
        return FrameOutcome::Captured;
    }
    if (keepRequested)
    {
        screenshots.frameKept(std::filesystem::path(saves).parent_path() / "screenshots");
    }
    screenshots.saveOnceDrawn(renderer.presenter());
    return FrameOutcome::Presented;
}

// ImGui's image count after the window was made again (a resize)
void App::adoptRemadeWindow()
{
    if (renderer.presenter().takeRemade())
    {
        renderer.ui().windowRemade();
    }
}

void App::shutdown()
{
    if (!renderer.started())
    {
        return;
    }
    trackLoad.reset();   // joins its read
    screenshots.finish();
    renderer.device().waitForIdle();
    renderer.releaseScreenPasses();
    frameCopies.destroy();
    shutdownImGui();
    unloadAllModels();
    audio.stopAll();
    renderer.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void App::shutdownImGui()
{
    painter.freeAll();
    movie->destroy();
    renderer.stopUi();
    ImGui::DestroyContext();
}

void App::unloadAllModels()
{
    retiredTracks.destroyAll();
    models.release(track);
    models.release(sky);
    fe->models().unloadAll();
    story->unloadModels();
    props.releaseAll();
}

