// Camera projection and depth conventions shared by every 3D pass.
// The depth buffer is reversed (near = 1, far = 0) on a float format: float precision is densest near 0, which
// reversed Z puts at the far plane, so surfaces only centimetres apart (Silver's river over its bed and speckle
// layer, below the waterfall) keep a stable order hundreds of metres away instead of z-fighting.
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace argon
{

inline constexpr float kDepthClear = 0.f;

// a perspective with y down and depth 0..1 (Vulkan's and the shaders' clip space; D3D12 takes the same) with reversed depth: zNear maps to 1, zFar to 0.
inline glm::mat4 reversedPerspective(float fovY, float aspect, float zNear, float zFar)
{
    glm::mat4 proj = glm::perspective(fovY, aspect, zFar, zNear);
    proj[1][1] *= -1.f;
    return proj;
}

}  // namespace argon
