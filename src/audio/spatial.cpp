// Argon: the doppler of a placed sound, see spatial.hpp.
#include "audio/spatial.hpp"

#include <algorithm>
#include <cmath>

namespace argon::spatial
{
namespace
{
constexpr float kOnTopOfListener = 1e-3f;
constexpr float kSpeedOfSound = 343.f;
constexpr float kDopplerFactor = 3.f;   // 0xc3220: SetDopplerFactor(3) on the engine buffers
constexpr float kLowestRatio = 0.5f, kHighestRatio = 2.f;
}  // namespace

// the source's and the listener's speeds along the line between them: closing in raises the pitch
float dopplerRatio(const Listener &listener, const glm::vec3 &at, const glm::vec3 &velocity)
{
    const glm::vec3 offset = at - listener.position;
    const float distance = glm::length(offset);
    if (distance <= kOnTopOfListener)
    {
        return 1.f;
    }
    const glm::vec3 towardsSource = offset / distance;
    const float listenerClosing = glm::dot(listener.velocity, towardsSource) * kDopplerFactor;
    const float sourceClosing = -glm::dot(velocity, towardsSource) * kDopplerFactor;
    const float ratio = (kSpeedOfSound + listenerClosing) / std::max(1.f, kSpeedOfSound - sourceClosing);
    return std::clamp(ratio, kLowestRatio, kHighestRatio);
}

}  // namespace argon::spatial
