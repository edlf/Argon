// Argon: the viewer's free camera (logic\free_camera.hpp).
#include "logic/free_camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kTurnPerPixel = 0.0035f;   // radians
constexpr float kPitchLimit = 1.55f;       // just short of straight up / down
constexpr float kFastKeys = 5.f;
const glm::vec3 kWorldUp(0, 1, 0);
}  // namespace

glm::vec3 FreeCamera::forward() const
{
    return {std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw)};
}

glm::mat4 FreeCamera::view() const
{
    return glm::lookAt(position, position + forward(), kWorldUp);
}

void FreeCamera::face(const glm::vec3 &direction)
{
    yaw = std::atan2(direction.x, -direction.z);
    pitch = std::asin(std::clamp(direction.y, -1.f, 1.f));
}

void FreeCamera::lookAtFrom(const glm::vec3 &target, const glm::vec3 &offset)
{
    position = target + offset;
    face(glm::normalize(target - position));
}

void FreeCamera::followCursor(const glm::dvec2 &cursor, bool held)
{
    if (held && dragging_)
    {
        yaw += float(cursor.x - lastCursor_.x) * kTurnPerPixel;
        pitch = std::clamp(pitch - float(cursor.y - lastCursor_.y) * kTurnPerPixel, -kPitchLimit, kPitchLimit);
    }
    dragging_ = held;
    lastCursor_ = cursor;
}

bool FreeCamera::dragging() const
{
    return dragging_;
}

void FreeCamera::move(const glm::vec3 &keys, float dt, bool fast)
{
    const glm::vec3 ahead = forward(), right = glm::normalize(glm::cross(ahead, kWorldUp));
    const float step = speed * dt * (fast ? kFastKeys : 1.f);
    position += (ahead * keys.x + right * keys.y + kWorldUp * keys.z) * step;
}
