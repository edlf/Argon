// Argon: a weapon trail's sprite particle (docs\weapons\firing.md "Trail sprites"): trail types 4.. (0x66650 ->
// 0x9d000) put a spinning quad from the track's envdata\quadsprite.dds into the spray pool (0x240540), updated by
// 0x9c540. Cell = type - 4 in a 4 x 4 grid of 128 px; list A alpha blended (record flag 2), else B additive.
#pragma once
#include <cstdint>

#include <glm/glm.hpp>

namespace argon::weapons
{

// what a weapon record (+0xf0..+0x128) asks of its trail sprites
struct TrailSpriteStyle
{
    int cell = 0;                 // +0xec - 4
    uint32_t flags = 0;           // +0xf0: 1 random spin direction, 2 alpha blended, 4 fade in
    uint32_t from = 0, to = 0;    // +0xf4 / +0xf8 ARGB
    float size = 0, sizeRange = 0;   // +0x110 / +0x114: half size + up to this x a random 0..1
    float grow = 1;               // +0x118: the size's growth
    float spin = 0, spinRange = 0;   // +0x11c / +0x120 degrees per second + up to this x a random 0..1
    float fade = 0;               // +0x124: the colour holds (or fades in) this long, then blends to `to`
    float life = 0;               // +0x128
};

struct TrailSprite
{
    glm::vec3 pos{0.f}, vel{0.f};
    float born = 0, life = 0, fade = 0, halfSize = 0, grow = 1, spin = 0;
    float startAngle = 0;   // +0x234 degrees: 0x66650 passes 0, the hover's dust rand % 360
    uint32_t from = 0, to = 0;
    int cell = 0;
    bool additive = true, fadeIn = false;

    // 0x9c540 each frame: false once its life is over; else it moves and its size grows
    bool advance(float now, float dt);
    // 0x9c540: `from` (faded in over `fade` with flag 4: alpha when blended, colour when additive), then a blend to
    // `to` from `fade` to the end of its life
    uint32_t colourAt(float now) const;
    // 0x9c540 (0x9c9d0): the quad turns to startAngle + spin x age degrees
    float angleAt(float now) const;
};

// 0x66650: a sprite for the trail point `at`; random10a / random10b are two 10-bit randoms (rand & 0x3ff, spin then
// size), flip the random bit for the spin direction
TrailSprite makeTrailSprite(const TrailSpriteStyle &style, const glm::vec3 &at, const glm::vec3 &vel, float now, unsigned random10a, unsigned random10b,
                            bool flip);

// the cell's corner in quadsprite.dds pixels (bd670: 128 px cells, 4 a row)
glm::vec2 trailSpriteCell(int cell);

}  // namespace argon::weapons
