// Argon: the race's weather as App feeds it (app\race_weather.hpp, docs\effects\rain.md "Rain"): its start with the
// race or the editor's track, and each 60 Hz frame at the camera with what the race says of it (a roof, the water,
// the spray). Drawn by hud\rain_overlay.hpp.
#include "app/app.hpp"

// the race load (0x800b3) sets the rain flag; the rain object starts dry (0x90d60)
void App::startRaceWeather(int trackId)
{
    weather.startRace(trackId, raceEggs.on(Egg::AlwaysRain));
}

// the editor's track takes the game's id for its folder (its weather, lap records and drones as in a race); a folder
// that is no course rains only with Always Rain
void App::startEditorWeather(int listIndex)
{
    if (listIndex < 0 || listIndex >= int(trackList.size()))
    {
        return;
    }
    const int id = courseId(trackList[size_t(listIndex)]);
    if (id >= 0)
    {
        picks.loadTrackId = id;
    }
    startRaceWeather(id);
}

// one frame of the rain object (0x91890) at the camera; its velocity is the player's craft's (the view follows it)
void App::stepRain()
{
    WeatherStep step;
    step.covered = playerUnderRoof();
    step.underWater = cameraUnderWater();
    step.sprayOnLens = sprayNearEye();
    step.soundsRun = session.raceEnd == kRacing && !replayRace.playing();
    step.camera.eye = freeCamera.position;
    step.camera.velocity = sim.velocity();
    step.camera.back = -freeCamera.forward();
    weather.step(step);
}

// camera_set_eye (0xae640): the water's depth at the eye (0xce6b0) is 0 or less
bool App::cameraUnderWater() const
{
    float depth = 0.f;
    return simWorld.waterDepth(freeCamera.position, depth) && depth <= 0.f;
}

// the sprite pass (0xbcfa0): any of the wake's puffs or droplet sheets (0xbd670's spray flag) within 2 m of the eye
bool App::sprayNearEye() const
{
    return wakeSpray.reachesLens(freeCamera.position);
}

// the player's craft's AI node (race slot +0 at 0x5972f0) has env_inside
bool App::playerUnderRoof() const
{
    if (aiGraph.nodes.empty())
    {
        return false;
    }
    const int node = std::clamp(sim.node(), 0, int(aiGraph.nodes.size()) - 1);
    return (aiGraph.nodes[size_t(node)].flags & argon::AiGraph::EnvInside) != 0;
}
