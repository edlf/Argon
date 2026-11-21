// A story scene's own light (docs\sequences\scenes.md "The scene's light"): the NMS header's +0x210 direction and +0x21c colour
// become D3D light 1 (seq_start_copy 0x96308 -> fe_set_light1 0xa7ea0), a directional light on top of the track's
// ambient and sun. Pure rules.
#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace argon::story
{

struct SceneLight
{
    glm::vec3 direction{0.f, 1.f, 0.f};   // the way the light shines, unit length (0x2f250)
    glm::vec3 colour{0.f};                // 0..1, the D3DCOLOR's red, green, blue
};

// 0xb4f80 checks the stored direction: every component finite, not all three within 1e-6 of 0, none above 1000; a
// failing one gets (0, 1, 0) and black (0x96329)
SceneLight sceneLight(const glm::vec3 &storedDirection, uint32_t storedColour);

} // namespace argon::story
