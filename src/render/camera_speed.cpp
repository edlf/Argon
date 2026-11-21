// Argon: the speed blur's speed (render\camera_speed.hpp).
#include "render/camera_speed.hpp"

#include <cmath>

namespace
{
constexpr float kSmoothingSeconds = 0.25f;
constexpr float kShortestFrame = 1e-4f;
}  // namespace

float CameraSpeed::measured(const HudCamera &camera, double now)
{
    const float dt = lastTime_ < 0 ? 0.f : float(now - lastTime_);
    const float speed = dt > kShortestFrame ? glm::dot(camera.eye - lastEye_, camera.forward) / dt : 0.f;
    lastEye_ = camera.eye;
    lastTime_ = now;
    return speed;
}

float CameraSpeed::smoothed(const HudCamera &camera, float dt, double now)
{
    if (dt > kShortestFrame)
    {
        const float speed = glm::dot(camera.eye - lastEye_, camera.forward) / dt;
        steady_ += (speed - steady_) * (1.f - std::exp(-dt / kSmoothingSeconds));
    }
    lastEye_ = camera.eye;
    lastTime_ = now;
    return steady_;
}
