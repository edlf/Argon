// Argon: the sprite pool's quads (docs\effects\sprites.md "Sprite pool record"): 0xbd670 allocates one (four 0x30-byte vertices:
// position, corner offset, colour, uv, the two angle slots, the basis row), the users (0x9c540, 0x9ca50) rewrite the
// offsets, colours and angle slots, and quadsprite.xvu places each corner: the offset turned by the slots, then along
// the basis its row picks (c[12 + 3 row]).
#pragma once
#include <glm/glm.hpp>

namespace argon
{

// the vertex's s16 at +0x2c / 3 (0xbd670's EAX, sprite.dat's "frame"): which basis quadsprite.xvu turns the offsets by
enum class SpriteFacing
{
    World = 0,     // c12: identity, the offsets are world offsets (sprite.dat, the droplet sheets)
    Upright = 1,   // c15: the view's x and z axes flattened to the ground, y up (trees, the spray puffs)
    Camera = 2     // c18: the view's axes (glows, clouds, the trail sprites, the weapon flashes)
};

// the basis the shader turns the corner offsets with (offset x row 0 + y x row 1 + z x row 2), in world space
struct SpriteBasis
{
    glm::vec3 x{1.f, 0.f, 0.f}, y{0.f, 1.f, 0.f}, z{0.f, 0.f, 1.f};
};

// `view` is the viewer's right-handed view; the game's view x axis points to the screen's left (its projection is
// mirrored, docs\models\README.md "Coordinate system"), so the game's c18 = (-right, up, forward)
SpriteBasis spriteBasis(SpriteFacing facing, const glm::mat4 &view);

// the vertex's two angle slots (+0x24, +0x28). 0xbd670 writes (sin, cos) of its angle (sprite.dat's 90 degrees:
// (1, 0)); 0x9c540 and the droplets' add 0x9d2c0 write (cos, -sin) of theirs. (1, 0) leaves the offsets as they are
struct SpriteTurn
{
    float first = 1.f, second = 0.f;

    // 0x9c540 (0x9c9d0): the slots for a quad turned by `radians`
    static SpriteTurn ofAngle(float radians);
    // 0xbd670: the slots it writes for its own angle, kept by quads nobody rewrites (the weapon flashes)
    static SpriteTurn ofAllocation(float radians);
};

// quadsprite.xvu: (x first + y second, y first - x second, z), then along the basis
glm::vec3 spriteCornerOffset(const glm::vec3 &offset, const SpriteTurn &turn, const SpriteBasis &basis);

inline constexpr int kSpriteCorners = 4;
inline constexpr int kSpriteTriangleVertices = 6;
// 0xbcfa0 / 0xbd1b0 index each quad as v0 v1 v2, v1 v2 v3
inline constexpr int kSpriteTriangleCorners[kSpriteTriangleVertices] = {0, 1, 2, 1, 2, 3};

// 0xbd670's offsets for a half size h: v0 (-h, -h), v1 (h, -h), v2 (-h, h), v3 (h, h)
glm::vec3 squareCornerOffset(float halfSize, int corner);

// 0x9c540's mode 1 (+0x242, the spray puffs): standing on the position, v0 (-w, h), v1 (w, h), v2 (-w, 0), v3 (w, 0)
glm::vec3 standingCornerOffset(float halfWidth, float height, int corner);

// a square cell of envdata\quadsprite.dds (512 x 1024) as 0xbd670 maps it: the cell's centre -+ 0.97 of its half size
struct SpriteCellUv
{
    glm::vec2 low{0.f}, high{0.f};

    // `corner` = the cell's top left in pixels, `size` its side in pixels
    static SpriteCellUv of(const glm::vec2 &corner, float size);
    // v0 (low u, low v), v1 (high u, low v), v2 (low u, high v), v3 (high u, high v)
    glm::vec2 at(int corner) const;
};

}  // namespace argon
