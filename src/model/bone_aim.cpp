// Argon: a bone turned towards a point (bone_aim.hpp), 0xb8630 from 0xb8f2e
#include "model/bone_aim.hpp"

namespace argon
{

namespace
{
constexpr float kOnTheBone = 1e-12f;

// the axis square to both, its sign as the game's crosses give it (x = y x z, then y' = x x z points away from y)
struct SquaredAxes
{
    glm::vec3 x, yAway;
};

SquaredAxes squareUp(const glm::vec3 &y, const glm::vec3 &z)
{
    const glm::vec3 x = glm::normalize(glm::cross(y, z));
    return {x, glm::normalize(glm::cross(x, z))};
}
} // namespace

glm::mat4 aimBone(const glm::mat4 &world, const BoneAim &aim, const glm::vec3 &rowScale)
{
    const glm::vec3 position(world[3]), posedY(world[1]), posedZ(world[2]);
    const glm::vec3 toTarget = aim.target - position;
    if (glm::dot(toTarget, toTarget) < kOnTheBone)
    {
        return world;
    }
    const glm::vec3 z = glm::normalize(toTarget);
    const SquaredAxes square = squareUp(glm::normalize(posedY), z);
    const float kept = 1.f - aim.share;
    const glm::vec3 blendedY = glm::normalize(kept * posedY - aim.share * square.yAway);
    const glm::vec3 blendedZ = glm::normalize(kept * posedZ + aim.share * z);
    const SquaredAxes turned = squareUp(blendedY, blendedZ);
    glm::mat4 aimed = world;
    aimed[0] = glm::vec4(turned.x * rowScale.x, world[0].w);
    aimed[1] = glm::vec4(-turned.yAway * rowScale.y, world[1].w);
    aimed[2] = glm::vec4(blendedZ * rowScale.z, world[2].w);
    return aimed;
}

} // namespace argon
