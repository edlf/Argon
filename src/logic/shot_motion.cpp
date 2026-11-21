#include "logic/shot_motion.hpp"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

namespace argon::weapons
{
namespace
{
const float kMaxPitch = 90.f;   // 0x1a3500
const float kTiny = 1e-6f;
const float kPercent = 0.01f;           // 0x1a355c
const float kInheritedDecayPerSecond = 0.4f;   // 0x1a3550
}  // namespace

ShotFrame frameAlong(const glm::vec3 &direction, const ShotFrame &current)
{
    if (glm::length(direction) <= kTiny)
    {
        return current;
    }
    const glm::vec3 forward = glm::normalize(direction);
    const glm::vec3 right = glm::cross(glm::vec3(0.f, 1.f, 0.f), forward);
    if (glm::length(right) <= kTiny)
    {
        return current;
    }
    ShotFrame frame;
    frame.forward = forward;
    frame.right = glm::normalize(right);
    frame.up = glm::cross(forward, frame.right);
    return frame;
}

ShotFrame frameAimedAt(const glm::vec3 &from, const glm::vec3 &target, const ShotFrame &current)
{
    return frameAlong(target - from, current);
}

ShotFrame frameRaisedFrom(const ShotFrame &craft, float pitchDegrees)
{
    const float pitch = glm::radians(std::clamp(pitchDegrees, 0.f, kMaxPitch));
    const glm::mat3 axes(craft.right, craft.up, craft.forward);
    const glm::mat3 raised = axes * glm::mat3(glm::rotate(glm::mat4(1.f), -pitch, glm::vec3(1.f, 0.f, 0.f)));
    return ShotFrame{raised[0], raised[1], raised[2]};
}

bool turnsAlongPath(float speed, float reactSpeed, bool bounces, bool bouncedThisStep)
{
    return !bouncedThisStep && (speed > reactSpeed || bounces);
}

glm::vec3 pathDirection(const glm::vec3 &moved, const glm::vec3 *hugNormal)
{
    if (!hugNormal || glm::length(*hugNormal) <= kTiny || glm::length(moved) <= kTiny)
    {
        return moved;
    }
    const glm::vec3 normal = glm::normalize(*hugNormal);
    const glm::vec3 direction = glm::normalize(moved);
    return direction - normal * glm::dot(normal, direction);
}

float acceleratedSpeed(float speed, float acceleration, float maxSpeed, float dt, bool holdsSpeed)
{
    if (holdsSpeed)
    {
        return speed;
    }
    return std::min(speed + dt * acceleration, maxSpeed);
}

glm::vec3 inheritedVelocity(const glm::vec3 &craftVelocity, float forwardSpeed, const glm::vec3 &craftForward, float inheritPercent)
{
    return (craftVelocity - craftForward * forwardSpeed) * inheritPercent * kPercent;
}

glm::vec3 decayedInherited(const glm::vec3 &inherited, float dt)
{
    return inherited * (1.f - dt * kInheritedDecayPerSecond);
}

}  // namespace argon::weapons
