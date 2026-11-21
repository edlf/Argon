// Argon: sprite.dat's quads (docs\tracks\README.md "sprite.dat", docs\effects\sprites.md "Track sprites"): sprite_dat_fixup
// (0xaf320) hands the file to quadsprite_AddCMMSprites (0xbdf40), which puts each record into the sprite pool for
// good; the sprite pass (0xbd8f0) draws them with quadsprite.xvu from the lower half of envdata\quadsprite.dds.
#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "render/sprite_pool.hpp"

namespace argon
{

struct TrackSpriteCorner
{
    glm::vec3 offset{0.f};
    glm::vec2 uv{0.f};      // quadsprite.dds texels / size: the record's v x 0.5 + 0.5 (its lower half)
    uint32_t argb = 0;      // D3DCOLOR
};

// one quad of the pool (0xc0 bytes: four 0x30-byte vertices v0..v3, drawn as v0 v1 v2 and v1 v2 v3)
struct TrackSprite
{
    glm::vec3 pos{0.f};
    std::array<TrackSpriteCorner, 4> corners;
    SpriteFacing facing = SpriteFacing::World;
    bool additive = false;  // flags bit 0: list B (ONE / ONE), else list A (alpha blended, sorted far to near)
};

// sprite.dat bytes -> its quads in file order (0xbdf40); none when the file is short or empty
std::vector<TrackSprite> readTrackSprites(const std::vector<uint8_t> &file);

// the corner's world position: sprite.dat's angle (90 degrees) leaves the offset unturned
glm::vec3 spriteCornerPosition(const TrackSprite &sprite, int corner, const SpriteBasis &basis);

// list A (0xbcfa0): the blended quads whose largest axis distance to the eye is under 1500, bucketed by that distance
// rounded (at most 4999) and drawn from the farthest bucket down to bucket 1 (bucket 0 is never drawn); a bucket
// holds its quads last added first
std::vector<int> blendedSpriteOrder(const std::vector<TrackSprite> &sprites, const glm::vec3 &eye);

// list B (0xbd1b0): the additive quads within 1500 of the eye, in pool order
std::vector<int> additiveSpriteOrder(const std::vector<TrackSprite> &sprites, const glm::vec3 &eye);

}  // namespace argon
