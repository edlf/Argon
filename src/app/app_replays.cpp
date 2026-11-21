// Argon: the replays' side of the race and the front end (docs\replays\README.md, docs\frontend\README.md "Attract
// demo"): the race recorded in the game's format each race step (neon_gamelogic_record_replay 0x3f210, 0x3eee0) for
// View Replay (WINMENU / LOSEMENU, 0x3e8e0), the attract demo's idle trigger (fe_shell_update 0x306b0) and way back
// (attract_update 0x40af0, main_menu 0x30af0), the pause menu's ghost replays (GAMELOGIC 3 / 4 -> 0x43460, ended by
// 0x43920). The playback itself: logic\replay_race.cpp.
#include "app/app.hpp"
#include "ui/fe_shell.hpp"

namespace
{
constexpr int kTvCameraView = 4;   // the drive camera view that shows the replay director while a replay plays
}  // namespace

// ---- recording (logic\race_recording.hpp: 0x3f210 at the race clock's reset; 0x3eee0 each frame; stopped by 0x3f2f0
// when POSTABLE opens)

void App::startRaceRecording()
{
    std::array<float, RaceRecording::kCrafts> shields{};
    for (int slot = 0; slot < RaceRecording::kCrafts; ++slot)
    {
        shields[size_t(slot)] = weapons.craft(slot).energy();
    }
    argon::replay::Info lineup;
    lineup.track = picks.loadTrackId;
    for (int racer = 0; racer < race.racerCount() && racer < argon::replay::kMaxCrafts; ++racer)
    {
        lineup.pilots[size_t(racer)] = argon::pilotIndex(startGrid.pilotOf(race.slotOf(racer)));
    }
    recording.start(lineup, simClock, shields);
}

// one race step, after the crafts moved: the clock as ReplayRace's (held by the grid intro), every craft's state
void App::recordRaceStep()
{
    recording.tick(gridIntroRunning(), raceHeld());
    if (!recording.recording())
    {
        return;
    }
    recording.recordFrame(racerStates());
    if (session.raceEnd != kRacing || recording.pastLongest())
    {
        stopRaceRecording();   // 0x52afa8 = 5 -> vtable +0x1c; past 1200 s (0x1b9790)
    }
}

void App::stopRaceRecording()
{
    recording.stop();
    std::printf("replay: recorded %zu frames, %.2f s\n", recording.frames(), recording.clock());
    std::printf("replay: weapons recorded: %s\n", recording.weaponsSummary().c_str());
    printShieldEnergies("at the race's end");
    if (!tests.saveReplayStem.empty())
    {
        try
        {
            recording.saveLastRace(tests.saveReplayStem);
        }
        catch (const std::exception &e)
        {
            std::fprintf(stderr, "replay: %s\n", e.what());
        }
    }
    pendingViewReplay = tests.viewReplay;
}

// the race report's weapon side: each racer's shield (what the hits left) and whether it was knocked out
void App::printShieldEnergies(const char *when) const
{
    std::printf("replay: shields %s:", when);
    for (int racer = 0; racer < race.racerCount(); ++racer)
    {
        const int slot = race.slotOf(racer);
        std::printf(" %.1f%s", weapons.craft(slot).energy(), weapons.craft(slot).knockedOut() ? "x" : "");
    }
    std::printf("\n");
}

// craft_record_inputs 0x61aa0: each racer's position, velocity, angular momentum, rotation and control bits
std::vector<argon::replay::CraftEvent> App::racerStates() const
{
    std::vector<argon::replay::CraftEvent> states;
    for (int racer = 0; racer < race.racerCount(); ++racer)
    {
        const argon::GameCraft &craft = race.racer(racer);
        std::array<float, 13> slots{};
        for (int slot = 0; slot < 13; ++slot)
        {
            slots[size_t(slot)] = craft.controlSlot(slot);
        }
        argon::replay::CraftEvent event;
        event.pos = craft.position();
        event.vel = craft.velocity();
        event.angularMomentum = craft.angularMomentum();
        event.rotation = glm::mat3(craft.matrix());
        event.controls = argon::replay::controlBitsOf(slots, false);
        states.push_back(event);
    }
    return states;
}

// ---- View Replay (0x3e8e0): the crafts, the AI and the pickups back on the grid, the clock 0, the race's own buffer
// played (0x3e700); at its end the post-race menu again

bool App::startViewReplay()
{
    if (!recording.hasLastRace())
    {
        return false;
    }
    const int menuBefore = session.raceEnd;
    beforeReplay.standings = race.standings();
    resetDriver();
    recording.stopWithoutKeeping();   // the replay is not recorded: the race's recording stays the one to view
    raceStart.stop();
    session.playerDrive = false;
    beforeReplay.raceEnd = menuBefore;
    restartEffectsClock();
    if (!replayRace.startViewReplay(recording.lastRace(), recording.lastRaceTiming().goStep))
    {
        leaveViewReplay();
        return false;
    }
    useReplayCamera();
    audio.restartRaceMusic(argon::MusicModes::OnStart);   // 0x3e8e0: 0xc8e10 then mode 1 (the custom music: its next song)
    return true;
}

// the effects' clock (simClock: shields' ages, the hits' effects, particles) back to its value at the race's start, so
// the replayed weapon entries see the same times, to the float, as the race did
void App::restartEffectsClock()
{
    simClock = recording.lastRaceTiming().simClockAtStart;
    particles.clear();
    weaponSprites.clear();
}

void App::leaveViewReplay()
{
    race.restoreStandings(beforeReplay.standings);
    session.raceEnd = beforeReplay.raceEnd;
    session.playerDrive = false;
}

// ---- the pause menu's ghost replays (0x43460): the clock 0, the ghost's lap alone (the player's craft and the other
// ghost hidden, no HUD); its end (0x43920) starts the run again from the line

void App::startGhostReplay(int ghost)
{
    const TimeAttackLogic &ta = modes.timeAttack();
    const ghost::Lap &lap = ghost == 0 ? ta.loadedGhost() : ta.sessionGhost();
    if (!lap.valid())
    {
        return;
    }
    replayRace.startGhostLap(lap, ghost);
    raceStart.stop();
    useReplayCamera();
}

// ---- the attract demo

// fe_shell_start_attract 0x31c10: the page left is kept to come back to, the next replay through the loading screen
bool App::startAttractDemo(int number)
{
    beforeReplay.page = fe->current();
    beforeReplay.droneCount = race.droneCount;
    beforeReplay.mode = &modes.current();
    modes.select(modes.quickRace());   // the game's mode 2 (CAttractDemo) races as a Quick Race here
    if (!replayRace.startAttractDemo(number))
    {
        modes.select(*beforeReplay.mode);
        return false;
    }
    useReplayCamera();
    std::printf("attract demo: replay%d\n", number);
    return true;
}

// fe_shell_update 0x306b0: the idle clock runs on the front end's pages, any held input zeroes it
void App::stepAttractIdle(float dt)
{
    if (!attractIdle.step(dt, input.anyHeld(), attractAllowedOnPage()))
    {
        return;
    }
    const int number = attractCycle.next();
    attractCycle.advance();   // attract_init 0x40440
    startAttractDemo(number);
}

// not on FE INTROSCREENS, FE MESSAGEBOX or FE OPTIONS CREDITS; nor on the loading screen (a viewer guard)
bool App::attractAllowedOnPage() const
{
    const int page = fe->current();
    return page != PageIntroScreens && page != PageCredits && page != PageLoader && !fe->messageBox().isOpen();
}

// main_menu 0x30af0: the front end again, on the page the demo started from (fe_main_enter 0x29f44 goes on to it)
void App::leaveAttractDemo()
{
    race.droneCount = beforeReplay.droneCount;
    if (beforeReplay.mode)
    {
        modes.select(*beforeReplay.mode);
    }
    openMenu();
    if (beforeReplay.page != PageMain)
    {
        fe->goTo(beforeReplay.page);
    }
    attractIdle.reset();
}

// ---- while a replay plays (no pause, no race-end menus; the HUD only in View Replay)

// attract_update 0x40af0: a press of START, BACK, A, B, X, Y, White, Black or a stick click ends the demo (not the
// d-pad). View Replay and the ghost replays end the same way here (not checked in the game)
void App::updateReplayInput()
{
    if (replaySkipPressed())
    {
        replayRace.stop();
    }
}

bool App::replaySkipPressed()
{
    const MenuInput in = input.readMenu();
    return input.startPressed() || in.a || in.b || in.x || in.y;
}

// hud0 "set none" in the attract demo (attract_init 0x4057d) and the ghost replays (0x436f1); View Replay's "WINMENU OFF"
// (0x3e930) makes the post-race menu send hud0 "set all" (WINMENU 0xa0a4f, LOSEMENU 0x84edb), so its HUD shows
bool App::replayHidesHud() const
{
    return replayRace.playing() && replayRace.kind() != ReplayRace::Kind::ViewReplay;
}

// camera0 "replay on" / "replay0output" (the camera's mode 0, 0xa30e5): it follows the replay cameras' director
// (logic\replay_director.cpp), shown through the TV camera view
void App::useReplayCamera()
{
    beforeReplay.camView = driveView.view();
    chooseDriveView(kTvCameraView);
    replayCameras.restart();   // its cameras are the track's: taken on the replay's first frame (after the load)
}

// the director's shot this frame (logic\replay_cameras.hpp)
argon::replay::CameraShot App::replayCameraShot(const glm::mat4 &pose, float dt)
{
    const float clock = replayRace.playing() ? replayRace.clock() : raceClock();
    return replayCameras.shot(track.data.cameras, picks.speedClass, {followedCraft(pose), passingCrafts()}, dt, clock);
}

// the director's input dataport: the followed craft (the player's, as the game's camera follows craft0)
argon::replay::FollowedCraft App::followedCraft(const glm::mat4 &pose) const
{
    argon::replay::FollowedCraft craft;
    craft.pos = glm::vec3(pose[3]);
    craft.vel = sim.velocity();
    craft.right = glm::vec3(pose[0]);
    craft.up = glm::vec3(pose[1]);
    craft.forward = glm::vec3(pose[2]);
    craft.lapPercent = sim.lapProgress(aiGraph) * 100.f;
    return craft;
}

// the race manager's racers for the KERB cameras: where each is and how fast it goes
std::vector<argon::replay::PassingCraft> App::passingCrafts() const
{
    std::vector<argon::replay::PassingCraft> racers;
    for (int racer = 0; racer < race.racerCount(); ++racer)
    {
        const argon::GameCraft &craft = race.racer(racer);
        racers.push_back({craft.position(), glm::length(craft.velocity())});
    }
    return racers;
}

// at the top of the next race step after the replay ran out (or was stopped): back where it came from
void App::finishEndedReplay()
{
    if (!replayRace.ended())
    {
        return;
    }
    replayRace.clearEnded();
    weapons.clearInFlight();
    std::printf("replay: over after %.2f s\n", replayRace.clock());
    printShieldEnergies("at the replay's end");
    chooseDriveView(beforeReplay.camView);
    switch (replayRace.kind())
    {
    case ReplayRace::Kind::AttractDemo:
        leaveAttractDemo();
        break;
    case ReplayRace::Kind::ViewReplay:
        leaveViewReplay();
        break;
    case ReplayRace::Kind::GhostLap:
        restartRace();
        break;
    }
}
