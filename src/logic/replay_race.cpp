// Argon: a race played from a recording (logic\replay_race.hpp, docs\replays\README.md).
#include "logic/replay_race.hpp"

#include "app/weapons.hpp"
#include "files/game_path.hpp"
#include "logic/drive_camera.hpp"
#include "logic/game_logic.hpp"
#include "logic/race.hpp"
#include "logic/start_grid.hpp"

namespace
{
constexpr float kStep = 1.f / 60.f;

// a ghost lap's samples as craft entries (the game keeps the lap in its own event buffer, "fastest" / "preloaded");
// the velocity from the neighbouring samples
argon::replay::Recording recordingOfLap(const ghost::Lap &lap)
{
    argon::replay::Recording recording;
    recording.lineup.pilots[0] = lap.pilot;
    recording.lineup.track = lap.track;
    std::vector<argon::replay::CraftEvent> &events = recording.crafts[0];
    for (size_t i = 0; i < lap.samples.size(); ++i)
    {
        const ghost::Sample &sample = lap.samples[i];
        const ghost::Sample &next = lap.samples[std::min(i + 1, lap.samples.size() - 1)];
        argon::replay::CraftEvent event;
        event.time = sample.seconds;
        event.pos = sample.pos;
        event.rotation = glm::mat3_cast(sample.rot);
        event.vel = next.seconds > sample.seconds ? (next.pos - sample.pos) / (next.seconds - sample.seconds) : glm::vec3(0.f);
        events.push_back(event);
        recording.index.push_back({sample.seconds, 0});
    }
    return recording;
}
}  // namespace

bool ReplayRace::startAttractDemo(int number)
{
    try
    {
        recording = argon::replay::loadRecording(argon::findGamePath(sources.game + "/replays"), "replay" + std::to_string(number));
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "replay %d: %s\n", number, e.what());
        return false;
    }
    begin(Kind::AttractDemo);
    useRecordingGrid();
    host.startQuickRace(recording.lineup.pilots[0], recording.lineup.track);
    return true;
}

bool ReplayRace::startViewReplay(const argon::replay::RecordingFiles &files, int goStep)
{
    goStep_ = goStep;
    try
    {
        recording = argon::replay::parseRecording(files.lineupText, files.index, files.events);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "view replay: %s\n", e.what());
        return false;
    }
    begin(Kind::ViewReplay);
    return true;
}

void ReplayRace::startGhostLap(const ghost::Lap &lap, int ghost)
{
    recording = recordingOfLap(lap);
    begin(Kind::GhostLap);
    ghost_ = ghost;
}

void ReplayRace::begin(Kind kind)
{
    kind_ = kind;
    ghost_ = -1;
    playing_ = true;
    ended_ = false;
    reported_ = false;
    clock_ = 0;
    tally.reset();
    shots = {};
    nextWeapon_ = 0;
    steps_ = 0;
    sources.weapons.clearInFlight();
}

void ReplayRace::stop()
{
    if (playing_)
    {
        finish();
    }
}

// fe_shell_start_attract 0x31c10: craft<i>'s pilot on grid slot i, the others drones (0x52cda0 = crafts - 1)
void ReplayRace::useRecordingGrid()
{
    sources.startGrid.order.clear();
    for (int i = 0; i < recording.lineup.craftCount(); ++i)
    {
        sources.startGrid.order.push_back(argon::pilotFolder(recording.lineup.pilots[size_t(i)]));
    }
    sources.race.droneCount = recording.lineup.craftCount() - 1;
}

void ReplayRace::step()
{
    if (!playing_)
    {
        return;
    }
    host.stepRaceIntro(kStep);
    if (!host.gridIntroRunning())
    {
        clock_ += kStep;
        ++steps_;
    }
    countLaps();
    placeCrafts();
    if (effectsClockRuns())
    {
        host.stepEffectsClock();   // the effects' clock (simClock), as a race step moves it
    }
    if (kind_ != Kind::GhostLap)
    {
        playWeaponEvents();
        host.stepReplayedWeapons();
    }
    if (clock_ > recording.endTime())
    {
        finish();   // attract_update 0x40af0 / 0x3e700: the replay ran out
    }
}

// the lap line on the poses placed last step, as a flying craft's frame sees them (GameCraft::frame -> trackProgress);
// the lap timers wait for GO as in the race (0x63a30)
void ReplayRace::countLaps()
{
    if (kind_ == Kind::GhostLap)
    {
        return;
    }
    for (int racer = 0; racer < sources.race.racerCount(); ++racer)
    {
        if (const Found<argon::GameCraft> craft = sources.race.craftInSlot(sources.race.slotOf(racer)))
        {
            countLapsOf(racer, *craft);
        }
    }
    if (!reported_ && tally.finished(0))
    {
        reported_ = true;
        reportFinish("the player finished");
    }
}

void ReplayRace::countLapsOf(int racer, argon::GameCraft &craft)
{
    craft.followReplay(sources.aiGraph, kStep);
    if (host.raceHeld())
    {
        craft.resetLapTimer();
    }
    tally.update(racer, craft.laps(), craft.lastLap(), sources.modes.current().hasLapLimit() ? sources.hudLaps : 0);
}

// as the race's own line (App::openResultsTable), so a recorded race and its replay can be compared
void ReplayRace::reportFinish(const char *what) const
{
    std::printf("replay: %s, %.2f s (finish order:", what, tally.raceTime(0));
    for (int racer : tally.finishOrder())
    {
        std::printf(" %d", racer);
    }
    std::printf(")\n");
}

void ReplayRace::placeCrafts()
{
    argon::replay::Pose pose;
    const int racers = kind_ == Kind::GhostLap ? 1 : sources.race.racerCount();
    for (int racer = 0; racer < racers; ++racer)
    {
        if (!argon::replay::poseAt(recording.crafts[size_t(racer)], clock_, pose))
        {
            continue;
        }
        easeOnTheGrid(racer, pose);
        if (racer == 0)
        {
            placePlayerCraft(pose);
        }
        else
        {
            sources.race.placeFromReplay(racer, pose.matrix(), pose.vel);
        }
        host.showReplayedControls(sources.race.slotOf(racer), pose.controls);
    }
}

// craft_replay_inputs 0x61c60: in the first 3 s of the clock (outside Time Attack) a craft eases 10 % a frame from
// where it is to its recorded position
void ReplayRace::easeOnTheGrid(int racer, argon::replay::Pose &pose) const
{
    const Found<const argon::GameCraft> craft = sources.race.craftInSlot(sources.race.slotOf(racer));
    if (kind_ != Kind::GhostLap && craft)
    {
        pose.pos = argon::replay::easedStartPosition(craft->position(), pose.pos, clock_);
    }
}

// View Replay: this step plays the race's next step's entries (a chunk carries the time of the frame before), so the
// effects' clock moves from the step before the race's first one after the countdown
bool ReplayRace::effectsClockRuns() const
{
    return kind_ != Kind::ViewReplay || steps_ + 1 >= goStep_;
}

// craft_weapon 0x641f0, called by eventbuffer_advance for each critical entry it reads: the weapon entries up to the
// clock (their chunk's time, as the crafts')
void ReplayRace::playWeaponEvents()
{
    while (nextWeapon_ < recording.weapons.size() && recording.weapons[nextWeapon_].time <= clock_)
    {
        const argon::replay::WeaponEvent &event = recording.weapons[nextWeapon_++];
        shots.count(event);
        host.playWeaponEvent(event);
    }
}

void ReplayRace::placePlayerCraft(const argon::replay::Pose &pose)
{
    sources.simPrev = sources.simCur;
    sources.camPrev = sources.camCur;
    sources.sim.placeFromReplay(pose.matrix(), pose.vel);
    sources.simCur = sources.sim.matrix();
    sources.camCur = sources.sim.tripod(sources.driveView.tripodIndex());
    if (kind_ == Kind::GhostLap)
    {   // 0x43770: the hidden player's craft replays, its state blocks copied into the shown ghost craft
        sources.modes.timeAttack().showOnlyGhost(ghost_, sources.simCur);
    }
}

void ReplayRace::finish()
{
    playing_ = false;
    ended_ = true;
    if (kind_ != Kind::GhostLap)
    {
        reportFinish("end");
        std::printf("replay: weapons played: %s\n", shots.summary().c_str());
    }
}
