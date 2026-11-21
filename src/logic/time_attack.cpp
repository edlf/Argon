// Argon: Time Attack's ghosts (TimeAttackLogic, logic\game_logic.hpp; docs\frontend\game-modes.md "Time Attack"; the
// race step 0x429c0): the session ghost, the fastest lap so far, and the loaded ghost, picked in FE GHOSTSELECT
// (ui\ghostselect.cpp; the files app_ghosts.cpp)
#include "logic/game_logic.hpp"

#include <glm/gtc/quaternion.hpp>

#include "effects/craft_exhausts.hpp"
#include "craft/craftsim.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/tourney_win.hpp"

void TimeAttackLogic::newRace()
{
    recording = {};
    sessionLap = {};
    recordingLap = false;
    for (GhostCraft &ghost : crafts)
    {
        ghost.shown = ghost.wasShown = false;
    }
}

// 0x429c0: the first crossing starts the recording (+0xd410 = 1); every later one ends a lap, which becomes the
// session ghost when it is the fastest so far (0xd9c90, 0x52afae = 1); the next lap's recording starts
void TimeAttackLogic::lapLineCrossed(bool firstCrossing)
{
    if (recordingLap && !firstCrossing)
    {
        recording.time = sources.player.crossingLapTime();
        if (ghost::replaces(recording, sessionLap))
        {
            sessionLap = std::move(recording);
        }
    }
    recording = {};
    recording.pilot = sources.loadPilot;
    recording.track = sources.loadTrack;
    recording.speed = sources.speedClass;
    recordingLap = true;
}

void TimeAttackLogic::step()
{
    recordSample();
    placeGhostCrafts();
}

ghost::Lap TimeAttackLogic::takeSessionGhost()
{
    ghost::Lap lap = std::move(sessionLap);
    sessionLap = {};
    return lap;
}

glm::mat4 TimeAttackLogic::ghostMatrix(int index, float alpha) const
{
    const GhostCraft &ghost = crafts[clampGhost(index)];
    const glm::quat q0 = glm::quat_cast(glm::mat3(ghost.prev)), q1 = glm::quat_cast(glm::mat3(ghost.cur));
    glm::mat4 pose = glm::mat4_cast(glm::slerp(q0, q1, alpha));
    pose[3] = glm::mix(ghost.prev[3], ghost.cur[3], alpha);
    return pose;
}

// the lap's recording: the craft's state each step (the game writes its event buffer every frame)
void TimeAttackLogic::recordSample()
{
    if (!recordingLap)
    {
        return;
    }
    ghost::Sample sample;
    sample.seconds = sources.player.lapTime();
    sample.pos = glm::vec3(sources.playerPose[3]);
    sample.rot = glm::quat_cast(glm::mat3(sources.playerPose));
    sample.jet = sources.exhausts.jetSize(0);
    recording.samples.push_back(sample);
}

// the two ghost crafts (the loaded +0xce74, the session +0xce70) at this lap's time, both from the lap's start
// (0x429c0 -> 0xd06f0), only once the recording runs (+0xd410); each hidden past its lap's end (+0x1129)
void TimeAttackLogic::placeGhostCrafts()
{
    for (int i = 0; i < 2; ++i)
    {
        GhostCraft &gc = crafts[i];
        gc.prev = gc.cur;
        ghost::Sample sample;
        gc.shown = recordingLap && ghostLap(i).valid() && ghost::sampleAt(ghostLap(i), sources.player.lapTime(), sample);
        if (gc.shown)
        {
            gc.cur = glm::mat4_cast(sample.rot);
            gc.cur[3] = glm::vec4(sample.pos, 1.f);
            gc.jet = sample.jet;
            if (!gc.wasShown)
            {
                gc.prev = gc.cur;   // no blend from the old pose when it comes back at a lap's start
            }
        }
        gc.wasShown = gc.shown;
    }
}

void TimeAttackLogic::showOnlyGhost(int ghost, const glm::mat4 &pose)
{
    for (int g = 0; g < 2; ++g)
    {
        GhostCraft &gc = crafts[g];
        gc.prev = gc.wasShown ? gc.cur : pose;
        gc.cur = pose;
        gc.shown = gc.wasShown = g == clampGhost(ghost);
    }
}
