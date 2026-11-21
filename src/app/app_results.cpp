// Argon: the race end (Quick Race). docs\frontend\race-end.md "Race end": the race manager's finish order
// (0x596db0 +0x514), POSTABLE "Results" (vtable 0x19d018: init 0x8dbb0, update 0x8ea30, rows 0x8f140, close
// 0x8fa90) and WINMENU / LOSEMENU (init 0xa01c8 / 0x845b0; LOSEMENU update 0x84b30, items 0x85a50, confirm 0x85bf0).

#include "app/app.hpp"
#include "logic/tourney_rules.hpp"
#include "ui/race_menus.hpp"

// the race (logic\race.hpp RaceHost): racing until the race end opens; the weapons' nemesis race
bool App::raceRunning() const
{
    return session.raceEnd == kRacing;
}

bool App::nemesisRace() const
{
    const int level = progress.tourneyLevel;
    return modes.tournamentSelected() && argon::tourney::isNemesisRace(level, progressKeeper.tourneyDone(level, tourneyRun.character));
}

void App::resetRaceEnd()
{
    session.raceEnd = kRacing;
    multiplayer.setDefaultWinner(MultiplayerSession::kNoWinner);
    raceTime.start(simClock);
    weapons.warnings().reset();
    playerKnockOut.clear();
    race.resetStandings();
    scoring.points.clearBreakdown();
    raceMenus->reset();
}

// each 60 Hz step: the race manager's standings (logic\race.cpp); the player finishing opens POSTABLE (0x3fba0:
// 0x52afa8 = 5, the HUD "set none"); the viewer's AI pilot drives the player's craft on
void App::stepRaceEnd()
{
    race.stepStandings();
    if (session.raceEnd != kRacing)
    {
        return;
    }
    if (tests.knockOutAt >= 0.f && raceClock() >= tests.knockOutAt && !weapons.craft(0).knockedOut())
    {
        weapons.knockOut(0, 1);
    }
    if (playerPads.multiplayerRace())
    {
        stepMultiplayerEnd();
    }
    else if (weapons.craft(0).knockedOut())
    {
        stepPlayerKnockedOut();
    }
    else if (race.playerFinished())
    {
        playerFinished();
    }
    else if (raceTime.pastLimit(simClock) && logic().hasLapLimit())
    {
        endRaceAtTimeLimit();
    }
}

// 0x3fd03: the race time recorded (0x83d20), the racers still racing join the finish order by their place now
// (0x3f320), POSTABLE opens; a win counts towards the profile's Percentage Complete (0x8ecc7)
void App::playerFinished()
{
    if (race.playerWon())
    {
        progressKeeper.recordRedshiftWin({&logic() == &modes.quickRace(), picks.speedClass, picks.loadTrackId, replayRace.playing()});
    }
    race.finishByPlace();
    openResultsTable(race.playerWon() ? "finished first" : "finished behind");
}

// 0x3b920: at 1200 s the player is out (+0x1145) and DNF (0x63c40), everyone still racing finishes by place
// (0x3f320), POSTABLE opens; the player's race time stat is 9999.99 (0x83d20)
void App::endRaceAtTimeLimit()
{
    raceTime.endByTime();
    race.knockOut(0);
    race.finishByPlace();
    openResultsTable("ran out of time");
}

// the player's finish or the time limit: POSTABLE (0x52afa8 = 5), the HUD "set none"; the music's mode 8 (the
// track's last segment after the current one)
void App::openResultsTable(const char *how)
{
    session.raceEnd = kPostTable;
    std::printf("race end: the player %s, %.2f s (finish order:", how, race.raceTime(0));
    for (int racer : race.finishOrder())
    {
        std::printf(" %d", racer);
    }
    std::printf(")\n");
    session.playerDrive = false;   // the AI drives the player's craft on behind the table
    replayCameras.restart();   // its cameras from their first, as at a replay's start
    if (session.paused)
    {
        resumeRace();
    }
    audio.loadUi(game);
    audio.playRaceFinale();   // POSTABLE ON (0x8eb10)
    raceSounds.stopTravelLoops();        // POSTABLE ON (0x8eab2)
}

// after the player's finish (or the time limit) the race is seen through the replay cameras, as on the console (the
// switch is not traced in the xbe: no "replay on" reaches camera0 there); not knocked out (its own camera), not in a
// split screen, not while View Replay runs its own (docs\frontend\race-end.md "Race end")
bool App::followsRaceEndCameras() const
{
    return session.raceEnd != kRacing && !playerPads.multiplayerRace() && !weapons.craft(0).knockedOut() && !replayRace.playing() && !track.data.cameras.empty();
}

// each frame of a race-end page's leaving second (audio\music_fade.hpp): the music object's volume +4 written from
// the fade against the Music Volume setting (0xdee90)
void App::fadeRaceMusic(const argon::MusicFade &fade, double now)
{
    const float settingMb = settingsMenu.volumeMillibels(SettingsMenu::kMusicVolume);
    const float levelMb = fade.levelMb(now, settingMb);
    audio.setRaceMusicFade(argon::musicFadeGain(levelMb, settingMb));
    std::fprintf(stderr, "music volume: %.0f mB at %.3f s (setting %.0f mB)\n", levelMb, fade.elapsed(now), settingMb);
}

// 0x3d5a0's turbo: + 1.5 + 0.5 x the Turbo upgrade level, the HUD fan's glow from now (craft +0x114c)
void App::grantLapTurbo()
{
    sim.addTurbo(lapTurbo(upgradeChar()));
    session.turboGrantT = simClock;
}

// 0x3d5a0, the player's lap: turbo += 1.5 + 0.5 x the Turbo upgrade level (0x1bb0ec + character x 0xb0 + 1) and the
// "Turbo Up" popup (hud0 message 0xc); the race's best and the lap record (app_laprecords.cpp)
void App::playerLapFinished(float lapTime)
{
    grantLapTurbo();
    awardPoints(0, 12);
    playerLapRecord(lapTime);
}

// the race end's pages: their input while one is up, and their draw
void App::updateRaceEnd()
{
    if (!session.raceEnd)
    {
        return;
    }
    if (!session.driving || !session.simStarted)
    {
        session.raceEnd = kRacing;
        return;
    }
    raceMenus->current().input(input.readMenu());
    pressTestLeave();
}

// --finish-leave: A twice (the score's count finished, or WINMENU / LOSEMENU's first item and Yes), then the page leaves
void App::pressTestLeave()
{
    if (tests.leaveAt < 0.f || glfwGetTime() < tests.leaveAt)
    {
        return;
    }
    tests.leaveAt = -1.f;
    menu::MenuInput press;
    press.a = true;
    raceMenus->current().input(press);
    raceMenus->current().input(press);
}

void App::drawRaceEnd()
{
    if (!session.raceEnd)
    {
        return;
    }
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = ds.y / 480.f;
    raceMenus->draw(FeCanvas(painter, ImGui::GetForegroundDrawList(), pixelsPerUnit, (ds.x / pixelsPerUnit - 640.f) * 0.5f));
}
