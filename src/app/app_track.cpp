// Argon: the race track's blocking load (its steps: app\track_arrival.cpp), its grid's craft models, the start and top
// cameras, the animated models' clock.
#include "app/app.hpp"

void App::loadTrack(int index)
{
    trackArrival.loadNow(index);
}

void App::loadGridShips()
{
    for (size_t s = 0; s < track.data.starts.size(); ++s)
    {
        props.craft(startGrid.pilotOf(int(s)));
    }
}

void App::choosePlayerPilot(int pilot)
{
    startGrid.playerPilot = pilot;
    loadGridShips();
    enginePilot = startGrid.pilotOf(0);
    resetDriver();
    audio.loadEngine(game, enginePilot);
}

// the game's quick-race grid again: you, your nemesis in slot 1, the rest random
void App::reshuffleGrid()
{
    startGrid.reshuffle();
    loadGridShips();
    resetDriver();
}

void App::cameraToStart()
{
    if (!track.data.starts.empty())
    {
        const auto &start = craftPoses.startOfSlot(0);
        glm::vec3 forward = glm::normalize(glm::vec3(start.forward[0], 0, start.forward[2]));
        // above and behind the player's start, high enough to clear the ships parked behind it
        freeCamera.position = glm::vec3(start.pos[0], start.pos[1], start.pos[2]) - forward * 16.f + glm::vec3(0, 11, 0);
        freeCamera.yaw = std::atan2(forward.x, -forward.z);
        freeCamera.pitch = -0.42f;
    }
    else
    {
        cameraToTop();
    }
}

void App::cameraToTop()
{
    glm::vec3 centre = (track.bmin + track.bmax) * 0.5f;
    float radius = glm::length(track.bmax - track.bmin) * 0.5f;
    if (!(radius > 0 && radius < 1e6f))
    {
        centre = glm::vec3(0);
        radius = 500;
    }
    freeCamera.position = centre + glm::vec3(0, radius * 1.1f, radius * 0.35f);
    freeCamera.yaw = 0;
    freeCamera.pitch = -1.25f;
}

