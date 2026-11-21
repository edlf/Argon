// Argon: the speed blur's speed (race scene 0x93490): how fast the camera moves along its view, from its eye a frame
// ago (camera +0x338 . -(+0x32c), m/s); the Mars tracks' steady one smooths it over the frame's time instead (Argon
// Options' Mars Flicker Fix: the gantries over the track no longer flicker).
#pragma once
#include "render/scene_views.hpp"

class CameraSpeed
{
  public:
    float measured(const HudCamera &camera, double now);             // over the time since the last call
    // the Mars tracks (docs\argon\differences.md "Speed blur on Mars"): they are raced right around the blur's 200 m/s,
    // and the clock read mid-frame swung the speed by about 10 m/s from frame to frame (a frame that also ran a physics
    // step reads it later), so the zoomed echo came and went every other frame. Here the eye's move over the frame's own
    // time (ImGui's, taken at its start like the drive's), smoothed over about a quarter second
    float smoothed(const HudCamera &camera, float dt, double now);

  private:
    glm::vec3 lastEye_{0.f};
    double lastTime_ = -1;
    float steady_ = 0.f;
};
