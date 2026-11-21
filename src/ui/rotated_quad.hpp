// Argon: hud_draw_rotated_quad (0xcbe50), the textured quad FE CHOOSESAVE (the ID card) and FE TRACK INFO
// (the track's picture) draw behind their frames (docs\frontend\README.md "Rotated quad")
#pragma once
#include <array>

#include <glm/glm.hpp>

namespace menu
{
// the quad's placement in the 640 x 480 layout: it runs along the direction (sin a, cos a) from `from` to `to` past
// the pivot and is `width` wide across it; the game adds 180 deg to the angle (0x5a211c x 180)
struct RotatedQuadSpec
{
    glm::vec2 pivot{0.f};
    float angleDegrees = 0.f;
    float from = 0.f, to = 0.f;
    float width = 0.f;
};

struct RotatedQuad
{
    std::array<glm::vec2, 4> corners;   // in the game's vertex order
    std::array<glm::vec2, 4> uvs;       // (1, 0), (0, 0), (1, 1), (0, 1): V = 0 at the `to` end
};

RotatedQuad rotatedQuad(const RotatedQuadSpec &spec);
}  // namespace menu
