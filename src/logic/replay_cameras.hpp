// Argon: camera0 "replay on" / "replay0output" (the camera's mode 0, 0xa30e5) as the viewer runs it: the replay cameras'
// director (logic\replay_director.hpp) over the track's cameras, taken again from their first on a replay's first frame
// (after its load) and when a race-end page starts following the race. A change of camera is logged as the game's
// "Changed to cam %d with score %.1f".
#pragma once
#include "logic/replay_director.hpp"

class ReplayCameras
{
  public:
    void restart();   // the track's cameras taken again on the next shot
    argon::replay::CameraShot shot(const std::vector<argon::TrackCamera> &cameras, int speedClass, const argon::replay::ReplayScene &scene, float dt,
                                   float clockForLog);

  private:
    argon::replay::ReplayDirector director_;
    bool stale_ = true;
};
