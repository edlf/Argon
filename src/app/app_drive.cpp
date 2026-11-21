// Argon: driving: game physics + AI or the player pad, chase / TV cameras.
#include "app/app.hpp"
#include "logic/rumble_rules.hpp"
#include "logic/ship_views.hpp"
#include "ui/fe_shell.hpp"

namespace
{
// the view camera's update (0xa33d0): every race view's field of view x 1.8 with Ultra Focal Length (0x52c8b9)
constexpr float kUltraFocalLength = 1.8f;
}  // namespace

// player 1: the keyboard and the first gamepad
argon::GameCraft::Pad App::readPad()
{
    if (!input.focused())
    {
        return {};
    }
    return finishPad(input.keyboardPad(), 0);
}

// split screen: player n (0-based) flies on the n-th connected gamepad (player 1 also on the keyboard)
argon::GameCraft::Pad App::readPlayerPad(int player)
{
    if (player == 0)
    {
        return readPad();
    }
    if (!input.focused())
    {
        return {};
    }
    return finishPad({}, player);
}

argon::GameCraft::Pad App::finishPad(argon::GameCraft::Pad padInput, int player)
{
    GLFWgamepadstate pad;
    if (gamepadOfPlayer(player, pad))
    {
        argon::input::addGamepad(padInput, pad);
    }
    if (raceEggs.on(Egg::MirrorTracks))
    {
        argon::input::mirrorSteering(padInput);
    }
    return padInput;
}

// multiplayer: the port the player joined with; else player n plays on the n-th connected gamepad
bool App::gamepadOfPlayer(int player, GLFWgamepadstate &state) const
{
    return playerPads.multiplayerRace() ? input.gamepad(playerPads.padOfPlayer(player), state) : input.nthGamepad(player, state);
}

// rb: buildRoute (track_source.hpp) over the loaded track's collision
void App::applyRoute(RouteBuild &&rb)
{
    session.driving = false;
    grid = std::move(rb.grid);
    route = std::move(rb.route);
    if (!rb.routeBuilt)
    {
        routeReport.reportNoRoute(rb.routeError);
        return;
    }
    drive.hoverHeight = rb.hover;
    simGrid = std::move(rb.simGrid);
    aiGraph = std::move(rb.aiGraph);
    simWorld.grid = &simGrid;
    simWorld.surf = &track.collSurf;
    simWorld.water = track.data.water;
    simWorld.waves = argon::WaterWaves(track.data.waterAmp);
    resetDriver();
    char summary[200];
    std::snprintf(summary,
                  sizeof summary,
                  "%.0f m, %d nodes, %s loop, hover %.2f m, %zu collision tris",
                  route.length(),
                  route.nodeCount(),
                  route.closed() ? "closed" : "OPEN",
                  drive.hoverHeight,
                  grid.triangleCount());
    routeReport.reportRoute(summary, std::move(rb.issues));
}

std::string App::viabilityText() const
{
    return route.valid() ? routeReport.issueTally() : "no route";
}

void App::chooseDriveView(int chosenView)
{
    driveView.choose(chosenView);
    if (session.simStarted)
    {
        camPrev = camCur = sim.tripod(driveView.tripodIndex());
    }
}

void App::resetDriver()
{
    session.simStarted = false;
    ticks.restart();
    simWorld.clock = 0.f;   // the race setup resets the game clock (0x3d34f / 0x3e870)
    heardSpeeds.restart(0);
    if (!craftStats.loaded() || aiGraph.nodes.size() < 3 || track.data.starts.empty())
    {
        return;
    }
    static const float quickRacePower[5] = {82, 86, 90, 92, 98};   // aivalues.txt ai_power_speedN_race2
    drive.aiPower = quickRacePower[std::clamp(picks.speedClass, 0, 4)];
    const auto &st = craftPoses.startOfSlot(0);   // the player starts last (0x3de29)
    glm::vec3 sp(st.pos[0], st.pos[1], st.pos[2]);
    int best = 0;
    float bd = 1e30f;
    for (size_t i = 0; i < aiGraph.nodes.size(); ++i)
    {
        float distance = glm::length(aiGraph.nodes[i].pos - sp);
        if (distance < bd)
        {
            bd = distance;
            best = int(i);
        }
    }
    aiGraph.buildRoutes(simWorld, track.data.pickups, picks.speedClass);
    sim.init(craftStats, argon::pilotIndex(startGrid.pilotOf(0)), picks.speedClass, drive.aiPower, glm::make_mat4(st.matrix), aiGraph, best, simWorld);
    sim.setRouteSeed(int(rnd()));
    session.simStarted = true;
    engineTrails.reset();
    resetPickups();
    wakeSpray.clear();
    race.fillGrid();
    race.seatPlayers(multiplayer.players());
    session.lapLineCrossings = sim.lineCrossings();
    logic().newRace();
    lapRecords.startRace(picks.loadTrackId, picks.speedClass);
    resetRaceWarnings();
    simPrev = simCur = sim.matrix();
    driveView.choose(driveView.view());   // the TV director starts again
    camPrev = camCur = sim.tripod(driveView.tripodIndex());
    resetRaceEnd();
    startRaceRecording();
}

// Controller n Vibration (settings 2..5, the game's flags 0x52c8c0 + port, also set by the pause): each player's pad
// gets what player_pad_input 0x70ac0 sends for their craft (logic\rumble_rules.hpp), through the pad's motor
// thresholds 0xb46d0; the pads no player uses are still
void App::updateVibration()
{
    bool used[PadRumble::kPads]{};
    const bool racing = session.driving && session.simStarted && !fe->isOpen() && !session.paused;
    for (int player = 0; player < multiplayer.players(); ++player)
    {
        const Found<const argon::GameCraft> craft = race.craftInSlot(player);
        const int pad = rumblePadOfPlayer(player);
        if (!craft || pad < 0 || pad >= PadRumble::kPads)
        {
            continue;
        }
        argon::rumble::PadState state;
        state.vibrationOn =
            racing && (!settingsMenu.hasProfileLists() || settingsMenu.current(SettingsMenu::kVibrationFirst + std::clamp(playerPads.padOfPlayer(player), 0, 3)) != 0);
        state.flying = session.playerDrive && !racerFinished(player);
        state.knockedOut = weapons.craft(player).knockedOut();
        state.secondsOut = weapons.craft(player).secondsKnockedOut(simClock);
        const argon::rumble::Motors motors = argon::rumble::padMotors(argon::rumble::playerMotors(state, {craft->rumbleFirst(), craft->rumbleSecond()}));
        padRumble.set(pad, motors.first, motors.second);
        used[pad] = true;
    }
    for (int pad = 0; pad < PadRumble::kPads; ++pad)
    {
        if (!used[pad])
        {
            padRumble.set(pad, 0, 0);
        }
    }
}

// which connected pad's motors a player feels: player 1's is the first gamepad, a split-screen player's their own
// (counted among the connected gamepads, as pad_rumble counts the pads with rumble)
int App::rumblePadOfPlayer(int player) const
{
    if (!playerPads.multiplayerRace())
    {
        return player;
    }
    const int joystick = playerPads.padOfPlayer(player);
    if (!input.portConnected(joystick))
    {
        return -1;
    }
    int before = 0;
    for (int j = GLFW_JOYSTICK_1; j < joystick; ++j)
    {
        before += glfwJoystickIsGamepad(j) == GLFW_TRUE;
    }
    return before;
}

bool App::racerFinished(int slot) const
{
    for (int racer : race.finishOrder())
    {
        if (race.slotOf(racer) == slot)
        {
            return true;
        }
    }
    return false;
}

// the Black button (the gamepad's RB, the keyboard's C), on its press: the next ship view (logic\ship_views.hpp);
// only while flying a ship view, as the game's player_pad_input
void App::followViewButton()
{
    GLFWgamepadstate pad{};   // no gamepad: every button up
    gamepadOfPlayer(0, pad);
    const bool pressed = input.viewButtonPressed(pad);
    if (pressed && session.playerDrive && !replayRace.playing() && argon::views::isShipView(driveView.view()))
    {
        chooseDriveView(argon::views::nextShipView(driveView.view()));
    }
}

namespace
{
constexpr int kMostTicksPerFrame = 30;
}  // namespace

void App::stepDrive(float dt)
{
    prepareDriveStep();
    if (!driveRunning())
    {
        return;
    }
    stepSimulation(dt);
    followDrive(dt);
}

// the drive's sound and camera without a simulation step (the camera settling before a test run)
void App::settleDrive(float dt)
{
    prepareDriveStep();
    if (!driveRunning())
    {
        return;
    }
    followDrive(dt);
}

// the start-up's test runs drive the race before the first frame (launch\test_runs.hpp)
TestRunSources App::testRunSources()
{
    return TestRunSources{sim,
                          route,
                          session.driving,
                          session.chaseCam,
                          replayRace,
                          audio,
                          wakeSpray,
                          weaponSprites,
                          startGrid,
                          picks.speedClass,
                          pickups,
                          powerUps.levels,
                          powerUps.special,
                          particles,
                          scoring,
                          race,
                          aiGraph,
                          weapons};
}

void App::prepareDriveStep()
{
    finishEndedReplay();
    startPendingViewReplay();
}

bool App::driveRunning() const
{
    return session.driving && session.simStarted;
}

void App::followDrive(float dt)
{
    followEngineSound();
    viewOverride = false;
    if (session.chaseCam)
    {
        driveCamera(dt);
    }
    introCamera();
}

void App::startPendingViewReplay()
{
    if (pendingViewReplay)
    {
        pendingViewReplay = false;
        startViewReplay();
    }
}

// the frame's time in 60 Hz ticks (at most 30 a frame)
void App::stepSimulation(float dt)
{
    if (input.focused())
    {
        followViewButton();
    }
    ticks.add(dt * drive.timeScale);
    sim.setTurboAllowed(session.raceEnd == kRacing, progress.cheatsOn[6]);   // Infinite Turbo cheat (0x1bbbf1)
    sim.setSlide(raceEggs.on(Egg::Slide));
    steerPlayerCraft();
    for (int stepped = 0; ticks.tickDue() && stepped < kMostTicksPerFrame; ++stepped)
    {
        stepRaceTick();
        ticks.takeTick();
    }
}

void App::steerPlayerCraft()
{
    if (weapons.craft(0).knockedOut() || raceTime.endedByTime())
    {
        sim.flyByPad();   // out of the race (weapons): no control
        return;
    }
    handControlToPlayer();
}

void App::handControlToPlayer()
{
    if (session.playerDrive)
    {
        sim.flyByPad(readPad());
    }
    else
    {
        sim.flyByAi();
    }
}

// one 60 Hz tick of the race
void App::stepRaceTick()
{
    if (replayRace.playing())
    {   // attract_update 0x40af0: the recording places the crafts
        replayRace.step();
        return;
    }
    simWorld.clock += argon::kGameTick;
    stepRaceIntro(argon::kGameTick);
    if (raceHeld())
    {
        stepHeldGrid();
        return;
    }
    if (session.raceWasHeld)
    {
        startFromGrid();
    }
    stepPlayerCraft();
    stepWorldEffects();
    stepRacers();
    stepWeapons();
    stepRaceProgress();
    scoreAirTime();
    stepHudReadouts();
    followEngineSpeeds();
}

// 0x43de0: no control until GO, but the crafts' physics runs (they settle onto their hover height)
void App::stepHeldGrid()
{
    session.raceWasHeld = true;
    sim.flyByPad();   // held: an empty pad
    simPrev = simCur;
    camPrev = camCur;
    sim.frame(argon::kGameTick, simWorld, aiGraph);
    sim.resetLapTimer();   // the timers wait for GO
    simCur = sim.matrix();
    camCur = sim.tripod(driveView.tripodIndex());
    race.holdOnGrid();
    recordRaceStep();
}

// GO: every pad gets control (0x78270), the lap timers start (0x63a30)
void App::startFromGrid()
{
    session.raceWasHeld = false;
    handControlToPlayer();
    sim.resetLapTimer();
    race.startDrones();
}

void App::stepPlayerCraft()
{
    simPrev = simCur;
    camPrev = camCur;
    sim.frame(argon::kGameTick, simWorld, aiGraph);
    raiseDust(sim);
    simCur = sim.matrix();
    camCur = sim.tripod(driveView.tripodIndex());
}

void App::stepWorldEffects()
{
    stepEffectsClock();
    followCraftEffects(sim, simCur, 0);
    updatePickups(glm::vec3(simCur[3]));
    if (raceEggs.on(Egg::AtlantisMode))
    {
        particles.emitBubbles(freeCamera.position, simClock, viewerRand);
    }
    stepRain();
}

// the other crafts, their sounds and the warnings about them
void App::stepRacers()
{
    race.step();
    raceSounds.stepCrafts(race, raceSoundState());
    raceSounds.stepTurbo(race, raceSoundState());
    selectListenerFilter();
    stepRaceWarnings();
}

void App::stepWeapons()
{
    weapons.warnings().beginStep();
    if (weaponsOn)
    {
        weapons.updateLocks();
        readFireInput();
        weapons.step();
    }
    weapons.warnings().endStep();
    weaponSprites.age(simClock, argon::kGameTick);
}

// the race end, the recording, the lap line, the mode's rules, the music and the laps' points
void App::stepRaceProgress()
{
    stepRaceEnd();
    recordRaceStep();
    stepLapLine();   // after the lap's turbo grant: the lap line sets the turbo (0x429c0)
    logic().step();
    cueLeadMusic();
    scoreFinishedLaps();
}

void App::scoreFinishedLaps()
{
    if (sim.laps() == scoring.lapsSeen)
    {
        return;
    }
    if (sim.laps() > scoring.lapsSeen)
    {
        scoreLap();
        cueLapMusic();
    }
    scoring.lapsSeen = sim.laps();
}

// 0x5f717 (player, on landing): air time - 1 s, if positive, x 100 x air_time_bonus points, "Airtime"
void App::scoreAirTime()
{
    if (!sim.inWater() && sim.hoverContacts() == 0)
    {
        scoring.airTimer += argon::kGameTick;
        return;
    }
    const float overASecond = scoring.airTimer - 1.f;
    if (overASecond > 0.f)
    {
        awardPoints(int(overASecond * 100.f) * scoring.cash.value("air_time_bonus"), 6);
    }
    scoring.airTimer = 0.f;
}

// 0x4cc30's popups, gamehud_update 0x496f0's count-up, the speedometer's last four speeds
void App::stepHudReadouts()
{
    scoring.points.step(simClock);
    speedReadout.push(sim.forwardSpeed());
}

// 0xc5200: the sound side follows |forward speed| by at most 1 m/s per frame
void App::followEngineSpeeds()
{
    heardSpeeds.follow(0, sim.forwardSpeed());
    followOtherEngines();
}

// curve input = 100 * speed / (absolute_max_speed[class] km/h * 0.27778)
void App::followEngineSound()
{
    if (!drive.engineSoundFollows)
    {
        return;
    }
    char key[32];
    std::snprintf(key, sizeof key, "absolute_max_speed%d", picks.speedClass + 1);
    const float top = craftStats.get(key, argon::pilotIndex(startGrid.pilotOf(0))) * 0.277777791f;
    audio.throttle = std::clamp(heardSpeeds.of(0) * 100.f / std::max(1.f, top), 0.f, 500.f);
    placeOtherEngines();
}

// 0xc5200 for every other craft: its own smoothed speed, by grid slot
void App::followOtherEngines()
{
    for (int slot = 1; slot < kShips; ++slot)
    {
        const Found<const argon::GameCraft> craft = race.craftInSlot(slot);
        if (craft)
        {
            heardSpeeds.follow(slot, craft->forwardSpeed());
        }
    }
}

// grid slots 1..n's engines: their curve input (as the player's), where they are and how fast they move; the listener
// moves with the player's craft
void App::placeOtherEngines()
{
    char key[32];
    std::snprintf(key, sizeof key, "absolute_max_speed%d", picks.speedClass + 1);
    audio.otherEngines.assign(size_t(kShips - 1), {});
    for (int slot = 1; slot < kShips; ++slot)
    {
        const Found<const argon::GameCraft> craft = race.craftInSlot(slot);
        if (!craft)
        {
            continue;
        }
        const float top = craftStats.get(key, argon::pilotIndex(startGrid.pilotOf(slot))) * 0.277777791f;
        argon::Audio::CraftEngine &engine = audio.otherEngines[size_t(slot - 1)];
        engine.throttle = std::clamp(heardSpeeds.of(slot) * 100.f / std::max(1.f, top), 0.f, 500.f);
        engine.position = craft->position();
        engine.velocity = craft->velocity();
    }
    audio.listenerVelocity = sim.velocity();
    audio.enginesOnListener = multiplayer.players() > 1;
}

// the shot of the view chosen (logic\drive_camera.hpp), then the view matrix, the listener and the UI read-outs
void App::driveCamera(float dt)
{
    const ViewShot shot = driveShot(dt);
    const glm::vec3 forward(craftPoses.driverMatrix()[2]);
    const glm::vec3 look = glm::length(shot.look - shot.eye) < 1e-4f ? shot.eye + forward : shot.look;
    viewOverride = true;
    const glm::vec3 up = glm::length(shot.up) > 1e-4f ? glm::normalize(shot.up) : glm::vec3(0, 1, 0);
    viewShot = {shot.eye, look, up, raceEggs.on(Egg::UltraFocalLength) ? shot.fov * kUltraFocalLength : shot.fov};
    freeCamera.position = shot.eye;   // audio listener + UI read-outs
    freeCamera.face(glm::normalize(look - shot.eye));
}

// the knock-out's death camera (0x620e0) and the replay director (the race end: app_results.cpp; camera0 "replay on"
// in a replay: app_replays.cpp) go first; then the view's own
ViewShot App::driveShot(float dt)
{
    const glm::mat4 ship = craftPoses.driverMatrix();
    if (driveView.showsTripod() && weapons.craft(0).knockedOut())
    {
        const argon::GameCraft::Tripod tripod = deathCamera();
        return ViewShot{tripod.pos, tripod.look, tripod.up, kDeathCameraFov};
    }
    const bool replayDirected = driveView.view() == DriveCamera::kTv && replayRace.playing() && !track.data.cameras.empty();
    if (followsRaceEndCameras() || replayDirected)
    {
        const argon::replay::CameraShot shot = replayCameraShot(ship, dt);
        return ViewShot{shot.eye, shot.look, glm::vec3(0, 1, 0), shot.fov};
    }
    if (driveView.showsTripod())
    {
        const float blend = craftPoses.simAlpha();
        return ViewShot{glm::mix(camPrev.pos, camCur.pos, blend), glm::mix(camPrev.look, camCur.look, blend), glm::mix(camPrev.up, camCur.up, blend)};
    }
    if (driveView.view() == DriveCamera::kTv)
    {
        ViewShot tv;
        if (driveView.tvShot(track.data.cameras, sim.lapProgress(aiGraph) * 100.f, ship, tv))
        {
            return tv;
        }
        const argon::GameCraft::Tripod mid = sim.tripod(1);   // nothing covers this stretch: the mid tripod
        return ViewShot{mid.pos, mid.look};
    }
    if (driveView.view() == DriveCamera::kViewerChase)
    {
        return DriveCamera::chaseShot(ship, freeCamera.position, dt, freeCamera.fov);
    }
    return driveView.shipMountedShot(track.data.cameras, ship);
}

// fire buttons (player_pad_input 0x70fd2 -> 0x71918): the held pad bits B / X / Y fire red / blue / yellow, one colour
// a frame in that order (logic\weapon_rules.cpp padFireColour), with Weapons on; a held special takes any of them
// (Weapons::fireWeapon). Works whether you or the AI drive slot 0.
void App::readFireInput()
{
    if (tests.autoFire)
    {
        for (int c = 0; c < 4; ++c)
        {
            weapons.fireWeapon(0, c);
        }
    }
    if (!weaponsOn)
    {
        return;
    }
    for (int player = 0; player < multiplayer.players() && player < race.racerCount(); ++player)
    {
        const bool human = player == 0 || session.playerDrive;
        const int colour = argon::weapons::padFireColour(heldFireButtons(player));
        if (human && colour != argon::weapons::kNoFire)
        {
            weapons.fireWeapon(race.slotOf(player), colour);
        }
    }
}

// the player's gamepad's B / X / Y (player 1: the first gamepad; the keyboard's 1 / 3 / 2 stand in for them, red /
// blue / yellow)
argon::weapons::FireButtons App::heldFireButtons(int player) const
{
    if (!input.focused())
    {
        return {};
    }
    argon::weapons::FireButtons held = player == 0 ? input.fireKeys() : argon::weapons::FireButtons{};
    GLFWgamepadstate pad;
    if (gamepadOfPlayer(player, pad))
    {
        held.b = held.b || pad.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;
        held.x = held.x || pad.buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS;
        held.y = held.y || pad.buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS;
    }
    return held;
}
