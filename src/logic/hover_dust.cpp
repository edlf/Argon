#include "logic/hover_dust.hpp"

namespace argon
{
namespace
{
const int kMountTrackId = 6;
const uint32_t kGreyBase = 0xffb0b0b0u, kGreyOffset = 0xff303030u;
const float kBlowBack = -10.f;          // 0x1a372c
const float kDriftScale = 0.0125f;      // 0x1a3538
const int kDriftCentre = 0x7f;
const float kRise = 1.5f;               // 0x3fc00000
const float kLife = 2.5f;               // 0x40200000
const float kFade = 0.5f;               // 0x3f000000
const float kGrowth = 1.08f;            // 0x3f8a3d71
const float kSizeStep = 0.1f;           // 0x1a3508
const float kBaseSize = 2.f;            // 0x1a3534
const float kBaseSpin = 40.f;           // 0x1a3604
const unsigned kBaseAlpha = 40;
const int kDustCell = 0;                // 0x9d000's EAX

unsigned channel(uint32_t argb, int shift)
{
    return (argb >> shift) & 0xffu;
}

uint32_t alphaOf(const DustRoll &roll)
{
    return ((roll.alphaA & 0x3fu) + (roll.alphaB & 0x1fu) + kBaseAlpha) & 0xffu;
}

float driftOf(unsigned random)
{
    return float(int(random & 0xffu) - kDriftCentre) * kDriftScale;
}
}  // namespace

bool raisesDust(int surface)
{
    return surface == 1 || surface == 2;
}

bool dustRollWins(unsigned random)
{
    return (random & 0xfu) == 0;
}

DustTint dustTintFor(int trackId)
{
    if (trackId == kMountTrackId)
    {
        return DustTint{kGreyBase, kGreyOffset};
    }
    return DustTint{};
}

uint32_t dustColour(uint32_t groundColour, const DustTint &tint, const DustRoll &roll)
{
    uint32_t out = alphaOf(roll) << 24;
    for (int shift = 0; shift < 24; shift += 8)
    {
        const unsigned scaled = (channel(tint.base, shift) * channel(groundColour, shift)) >> 8;
        out |= ((scaled + channel(tint.offset, shift)) & 0xffu) << shift;
    }
    return out;
}

glm::vec3 dustVelocity(const glm::vec3 &forward, const DustRoll &roll)
{
    return glm::vec3(forward.x * kBlowBack + driftOf(roll.driftX), kRise, forward.z * kBlowBack + driftOf(roll.driftZ));
}

weapons::TrailSprite makeDustSprite(const DustPuff &puff, const DustTint &tint, const DustRoll &roll)
{
    weapons::TrailSprite sprite;
    sprite.pos = puff.at;
    sprite.vel = dustVelocity(puff.forward, roll);
    sprite.born = roll.now;
    sprite.life = kLife;
    sprite.fade = kFade;
    sprite.from = dustColour(puff.groundColour, tint, roll);
    sprite.to = sprite.from & 0x00ffffffu;
    sprite.halfSize = float(roll.size % 20u) * kSizeStep + kBaseSize;
    sprite.grow = kGrowth;
    sprite.spin = float(roll.spin % 60u) + kBaseSpin;
    if (puff.fromRay3)
    {
        sprite.spin = -sprite.spin;
    }
    sprite.startAngle = float(roll.angle % 360u);
    sprite.cell = kDustCell;
    sprite.additive = false;
    sprite.fadeIn = true;
    return sprite;
}

} // namespace argon
