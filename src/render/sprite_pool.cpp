#include "render/sprite_pool.hpp"

#include <cmath>

namespace argon
{
namespace
{
const glm::vec2 kAtlasTexel(1.f / 512.f, 1.f / 1024.f);   // 0x1a35bc, 0x1a35b8
const float kCellInset = 0.97f;                           // 0x1a35b0
const float kCornerSigns[kSpriteCorners][2] = {{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}};

// vec3_normalize_fast (0x2f250) of the axis without its y
glm::vec3 flattened(const glm::vec3 &axis)
{
    const glm::vec3 flat(axis.x, 0.f, axis.z);
    const float length = glm::length(flat);
    return length > 1e-6f ? flat / length : flat;
}

bool isLowCorner(int corner)
{
    return corner < 2;
}
}  // namespace

// 0xbd8f0: c12 = identity, c15 = (flat view x, (0, 1, 0), flat view z), c18 = the view's axes (its transpose)
SpriteBasis spriteBasis(SpriteFacing facing, const glm::mat4 &view)
{
    const glm::vec3 right(view[0][0], view[1][0], view[2][0]), up(view[0][1], view[1][1], view[2][1]);
    const glm::vec3 back(view[0][2], view[1][2], view[2][2]);
    SpriteBasis basis;
    if (facing == SpriteFacing::Camera)
    {
        basis.x = -right;
        basis.y = up;
        basis.z = -back;
    }
    else if (facing == SpriteFacing::Upright)
    {
        basis.x = flattened(-right);
        basis.z = flattened(-back);
    }
    return basis;
}

SpriteTurn SpriteTurn::ofAngle(float radians)
{
    return SpriteTurn{std::cos(radians), -std::sin(radians)};
}

SpriteTurn SpriteTurn::ofAllocation(float radians)
{
    return SpriteTurn{std::sin(radians), std::cos(radians)};
}

glm::vec3 spriteCornerOffset(const glm::vec3 &offset, const SpriteTurn &turn, const SpriteBasis &basis)
{
    const float x = offset.x * turn.first + offset.y * turn.second;
    const float y = offset.y * turn.first - offset.x * turn.second;
    return x * basis.x + y * basis.y + offset.z * basis.z;
}

glm::vec3 squareCornerOffset(float halfSize, int corner)
{
    return glm::vec3(kCornerSigns[corner][0] * halfSize, kCornerSigns[corner][1] * halfSize, 0.f);
}

glm::vec3 standingCornerOffset(float halfWidth, float height, int corner)
{
    return glm::vec3(kCornerSigns[corner][0] * halfWidth, isLowCorner(corner) ? height : 0.f, 0.f);
}

// 0xbd670: u = x / 512 + size / 1024 -+ 0.97 x size / 1024, v = y / 1024 + size / 2048 -+ 0.97 x size / 2048
SpriteCellUv SpriteCellUv::of(const glm::vec2 &corner, float size)
{
    const glm::vec2 centre = (corner + glm::vec2(size * 0.5f)) * kAtlasTexel;
    const glm::vec2 half = glm::vec2(size * 0.5f * kCellInset) * kAtlasTexel;
    return SpriteCellUv{centre - half, centre + half};
}

glm::vec2 SpriteCellUv::at(int corner) const
{
    return glm::vec2(corner % 2 == 0 ? low.x : high.x, isLowCorner(corner) ? low.y : high.y);
}

}  // namespace argon
