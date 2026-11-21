// Argon: the App side of the in-race pause: pausing and resuming the race, a racer's pad pulled out, restarting or
// leaving the race (docs\frontend\race-end.md "Pause menu"). The PAUSEMENU page itself: ui\pause.cpp.
#include "app/app.hpp"
#include "ui/fe_shell.hpp"
#include "ui/race_menus.hpp"

using namespace menu::race;

void App::pauseRace()
{
    settingsKeeper.buildSettings();   // the volume rows read the Sound Settings entries
    session.paused = true;
    resetPauseMenu();
    audio.holdRaceSounds();
}

void App::resumeRace()
{
    settingsKeeper.buildSettings();
    session.paused = false;
    resetPauseMenu();
    audio.resumeRaceSounds();
}

void App::resetPauseMenu()
{
    pauseBackground.blurAgain();
    raceMenus->pause().reset();
}

// Start / Esc / P pause and resume (0x721f0 -> 0x782d0); while paused the menu takes the presses
void App::updatePause()
{
    if (!session.driving || !session.simStarted || !session.playerDrive || session.raceEnd)
    {
        padWatch.reset();
        if (session.paused)
        {
            resumeRace();
        }
        return;
    }
    if (watchControllers())
    {
        return;
    }
    if (!session.paused && !input.focused())
    {
        pauseRace();   // the window lost the focus
        return;
    }
    bool start = input.startPressed();
    if (!session.paused)
    {
        if (start)
        {
            pauseRace();
            input.readMenu();
        }   // swallow this frame's menu keys
        return;
    }
    MenuInput in = input.readMenu();
    audio.loadUi(game);
    PauseMenu &menu = raceMenus->pause();
    if (menu.isSaving())
    {   // +0x1d4: the messages are dropped, START too (0x721f0 needs the pause menu's own state)
        menu.stepSaving(glfwGetTime());
        return;
    }
    if (start)
    {
        resumeRace();
        return;
    }
    menu.input(in);
}

// 0x8a7f0: a racer's pad pulled out pauses the race behind that player's message; until every message has gone the
// presses are dropped (0x3bf60 passes none on while a pad is out or waiting), and the frame the last one goes the
// pause menu opens. True while the messages hold the race (or opened the menu this frame)
bool App::watchControllers()
{
    const menu::PadWatch::Event event = padWatch.update(racersConnected(), racersStartPresses());
    if (event != menu::PadWatch::Event::None)
    {
        pauseRace();   // PAUSE ON both times: the race holds, the cursor on Continue
    }
    if (event == menu::PadWatch::Event::None && !padWatch.holdsMenu())
    {
        return false;
    }
    input.startPressed();   // the START that cleared the wait is not a resume
    input.readMenu();
    return true;
}

menu::PadWatch::PerPlayer App::racersConnected() const
{
    menu::PadWatch::PerPlayer connected{};
    for (int p = 0; p < std::min(multiplayer.players(), menu::PadWatch::kPlayers); ++p)
    {
        connected[size_t(p)] = input.portConnected(playerPads.padOfPlayer(p));
    }
    return connected;
}

// START going down on each racer's pad (+0x89 & 1, the pad's own press)
menu::PadWatch::PerPlayer App::racersStartPresses()
{
    menu::PadWatch::PerPlayer pressed{};
    for (int p = 0; p < std::min(multiplayer.players(), menu::PadWatch::kPlayers); ++p)
    {
        GLFWgamepadstate pad;
        const bool now = input.gamepad(playerPads.padOfPlayer(p), pad) && pad.buttons[GLFW_GAMEPAD_BUTTON_START] != 0;
        pressed[size_t(p)] = now && !pzPadStartHeld[size_t(p)];
        pzPadStartHeld[size_t(p)] = now;
    }
    return pressed;
}

// 0x8b6f0: the race again from the grid (the music restarts), the countdown again (no grid intro)
void App::restartRace()
{
    audio.stopRaceEvents();
    resetDriver();
    session.driving = true;
    session.chaseCam = true;
    session.playerDrive = true;
    startRaceCountdown();
}

// out of the race to FE MAIN (0x8b5b0: a multiplayer session ends there) or FE COURSE SELECT (0x8b630)
void App::quitRaceTo(int page)
{
    if (playRaceEndScene(page))
    {
        return;   // the race leaves once the tournament's end scene is over (0x7fa50)
    }
    leaveRaceFor(page);
}

// Track Select / Main Menu with a Time Attack session ghost: FE GHOSTSAVE first
void App::openGhostSave(int nextPage, ghost::Lap lap)
{
    openMenu();
    fe->ghostSave().open(nextPage, std::move(lap));
}

void App::leaveRaceFor(int page)
{
    openMenu();
    if (page == PageMain)
    {
        leaveMultiplayer();
        return;
    }
    fe->goTo(page);
}

// while a racer's controller message holds the pause (0x8b0a6: +0x614..0x617 = 0) no page is up, so nothing is drawn
// over the race and its HUD
void App::drawPause()
{
    if (!session.paused || padWatch.holdsMenu())
    {
        return;
    }
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = ds.y / 480.f;
    raceMenus->pause().draw(FeCanvas(painter, ImGui::GetForegroundDrawList(), pixelsPerUnit, (ds.x / pixelsPerUnit - 640.f) * 0.5f));
}
