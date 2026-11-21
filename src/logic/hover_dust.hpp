// Argon: the dust the hover raises behind a craft on rough / loose ground (craft_hover 0x5e6d0, 0x5ebe6..0x5f667;
// docs\effects\README.md "Dust"). Each physics substep, each rear hover ray (2, 3) that held the craft on surface 1 or 2 has a
// 1 in 16 chance (rand & 0xf == 0) of one trail sprite (0x9d000, cell 0 of quadsprite.dds, alpha blended, faded in)
// at its hit point, blown back along the craft's forward.
#pragma once
#include <cstdint>

#include <glm/glm.hpp>

#include "logic/trail_sprite.hpp"

namespace argon
{

// the rear hover rays that raise dust (0x5f377 reads ray 2's surface, 0x5f4f1 ray 3's)
constexpr int kFirstDustRay = 2, kSecondDustRay = 3;

// a rear ray's dust this substep, as the craft records it
struct DustPuff
{
    glm::vec3 at{0.f};                    // the ray's hit point (collision +0x81cc)
    glm::vec3 forward{0.f, 0.f, 1.f};     // the craft's forward (+0x1f0)
    uint32_t groundColour = 0;            // the vertex colour (0xa46a0) under ray 3 (both rays use it, 0x5f40d)
    bool fromRay3 = false;                // ray 3: the spin turns the other way (0x5f5fd)
};

// the base colour scaled by the ground colour, plus the offset (0x5f35b): grey on Mount (track id 6), else sandy
struct DustTint
{
    uint32_t base = 0xffb0a070u, offset = 0xff302816u;
};

// what the spawn draws from rand() after the roll (in the XBE's order) and when it happens
struct DustRoll
{
    unsigned driftX = 0, driftZ = 0;              // (rand & 0xff) - 127 x 0.0125 m/s
    unsigned alphaA = 0, alphaB = 0;              // alpha = (rand & 0x3f) + (rand & 0x1f) + 40
    unsigned spin = 0, angle = 0, size = 0;       // 40 + rand % 60 deg/s, rand % 360 deg, 2 + rand % 20 x 0.1
    float now = 0.f;
};

// 0x5f377 / 0x5f4f1: rough (1) and loose (2) only; track, water, slippery and a ray that held nothing raise none
bool raisesDust(int surface);

// 0x5f38b / 0x5f505: one rand() a ray, dust when its low four bits are 0
bool dustRollWins(unsigned random);

DustTint dustTintFor(int trackId);

// 0x5f3e9..0x5f466: per channel (base x ground >> 8) + offset, as a byte; the alpha from the roll
uint32_t dustColour(uint32_t groundColour, const DustTint &tint, const DustRoll &roll);

// 0x5f3b3..0x5f449: 10 m/s back along the forward in x / z with up to +-1.6 m/s each, 1.5 m/s up
glm::vec3 dustVelocity(const glm::vec3 &forward, const DustRoll &roll);

// 0x5f4e6 / 0x5f662 -> 0x9d000: life 2.5 s, fades in over 0.5 s then out to alpha 0, half size 2..3.9 growing x 1.08
weapons::TrailSprite makeDustSprite(const DustPuff &puff, const DustTint &tint, const DustRoll &roll);

} // namespace argon
