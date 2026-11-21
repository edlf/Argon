#include "logic/blast_push.hpp"

#include <algorithm>
#include <cmath>

namespace argon::blast
{

namespace
{
constexpr float kBrakingPerPush = 0.02f;   // 0x1a36d8
constexpr float kMaxBraking = 0.4f;        // 0x1a3550
constexpr float kSidePerPush = 10.f;       // -0x1a372c

glm::vec3 headingOf(glm::vec3 velocity)
{
    if (velocity == glm::vec3(0.f))
    {
        velocity.x += 1.f;
    }
    return glm::normalize(velocity);
}
}  // namespace

glm::vec3 upward(const glm::vec3 &push)
{
    return {push.x, std::abs(push.y), push.z};
}

float brakingFraction(const glm::vec3 &push, const glm::vec3 &velocity)
{
    const glm::vec3 heading = headingOf(velocity);
    const glm::vec3 across = push - glm::dot(push, heading) * heading;
    return std::clamp(glm::length(across) * kBrakingPerPush, 0.f, kMaxBraking);
}

float doubledBraking(float fraction)
{
    return fraction + fraction;
}

glm::vec3 sidewaysForce(float pushLength, const glm::vec3 &right, float sideScale, unsigned randValue)
{
    const glm::vec3 force = right * (pushLength * kSidePerPush * sideScale);
    if ((randValue & 1u) == 0u)
    {
        return force;
    }
    return -force;
}

void TurboCooldown::countDown(float h)
{
    seconds_ = std::max(seconds_ - h, 0.f);
}

}  // namespace argon::blast
