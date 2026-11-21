// Argon: the replay cameras as the viewer runs them (replay_cameras.hpp).
#include "logic/replay_cameras.hpp"

#include <cstdio>

void ReplayCameras::restart()
{
    stale_ = true;
}

argon::replay::CameraShot ReplayCameras::shot(const std::vector<argon::TrackCamera> &cameras, int speedClass, const argon::replay::ReplayScene &scene,
                                              float dt, float clockForLog)
{
    if (stale_)
    {
        director_.reset(cameras, speedClass);
        stale_ = false;
    }
    const int before = director_.current();
    const argon::replay::CameraShot shot = director_.step(scene, dt);
    const int now = director_.current();
    if (now != before && now >= 0 && size_t(now) < cameras.size())
    {
        const argon::TrackCamera &camera = cameras[size_t(now)];
        std::printf("replay camera %d: %s \"%s\" at %.2f s\n", now, camera.type.c_str(), camera.name.c_str(), clockForLog);
    }
    return shot;
}
