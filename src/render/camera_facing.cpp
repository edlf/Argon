// Argon: meshes placed by the camera (camera_facing.hpp).
#include "render/camera_facing.hpp"

namespace argon
{
namespace
{
const float kMinFacingLength = 1e-6f;

glm::vec3 positionOf(const glm::mat4 &world)
{
    return glm::vec3(world[3]);
}

void placeAt(glm::mat4 &world, const glm::vec3 &position)
{
    world[3] = glm::vec4(position, 1.f);
}
} // namespace

ViewPoint ViewPoint::of(const glm::mat4 &view)
{
    ViewPoint point;
    point.eye = glm::vec3(glm::inverse(view)[3]);
    point.axis = glm::normalize(glm::vec3(view[0][2], view[1][2], view[2][2]));
    return point;
}

glm::mat4 CameraPlacement::listed(glm::mat4 world, const ViewPoint &view) const
{
    if (followsCamera)
    {
        placeAt(world, positionOf(world) + view.eye);
    }
    if (facesCamera && !billboard)
    {
        world = facingEye(world, view.eye);
    }
    return world;
}

glm::mat4 CameraPlacement::drawn(glm::mat4 world, const ViewPoint &view) const
{
    if (!billboard)
    {
        return world;
    }
    placeAt(world, billboardPosition(positionOf(world), view, billboardFactor));
    if (facesCamera)
    {
        world = facingEye(world, view.eye);
    }
    return world;
}

glm::vec3 billboardPosition(const glm::vec3 &position, const ViewPoint &view, float factor)
{
    const glm::vec3 offset = position - view.eye;
    const glm::vec3 along = view.axis * glm::dot(view.axis, offset);
    return view.eye + along + (offset - along) * factor;
}

glm::mat4 facingEye(const glm::mat4 &world, const glm::vec3 &eye)
{
    const glm::vec3 position = positionOf(world);
    const glm::vec3 away = position - eye;
    const glm::vec3 side = glm::cross(glm::vec3(0.f, 1.f, 0.f), away);
    if (glm::length(away) < kMinFacingLength || glm::length(side) < kMinFacingLength)
    {
        return world;
    }
    const glm::vec3 z = glm::normalize(away);
    const glm::vec3 x = glm::normalize(side);
    const glm::vec3 y = glm::cross(z, x);
    return glm::mat4(glm::vec4(x, 0.f), glm::vec4(y, 0.f), glm::vec4(z, 0.f), world[3]);
}
} // namespace argon
