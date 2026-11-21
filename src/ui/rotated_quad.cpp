// Argon: hud_draw_rotated_quad (0xcbe50); ui\rotated_quad.hpp
#include "ui/rotated_quad.hpp"

#include <cmath>

namespace menu
{
namespace
{
constexpr float kDegrees = 3.14159265f / 180.f;
constexpr float kAddedTurn = 180.f;   // 0x5a211c (pi / 180) x 180 (0x1a36fc), added to every angle
}  // namespace

// 0xcbe50: with d = (sin a, cos a) and the half width h along (cos a, -sin a), the corners are to-end - h, to-end + h,
// from-end - h, from-end + h
RotatedQuad rotatedQuad(const RotatedQuadSpec &spec)
{
    const float angle = (spec.angleDegrees + kAddedTurn) * kDegrees;
    const glm::vec2 along(std::sin(angle), std::cos(angle));
    const glm::vec2 halfAcross = glm::vec2(std::cos(angle), -std::sin(angle)) * (spec.width * 0.5f);
    const glm::vec2 toEnd = spec.pivot + along * spec.to, fromEnd = spec.pivot + along * spec.from;
    RotatedQuad quad;
    quad.corners = {toEnd - halfAcross, toEnd + halfAcross, fromEnd - halfAcross, fromEnd + halfAcross};
    quad.uvs = {glm::vec2(1, 0), glm::vec2(0, 0), glm::vec2(1, 1), glm::vec2(0, 1)};
    return quad;
}
}  // namespace menu
