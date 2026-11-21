#include "logic/weapon_flash.hpp"

#include <algorithm>
#include <cmath>

namespace argon::weapons
{
namespace
{
const float kStartGlowHalfSize = 2.5f;   // 0x40200000
const float kSparkHalfSize = 10.f;       // 0x1a35a4, + rand & 3
const float kHitFlashHalfSize = 25.f;    // 0x41c80000
const uint32_t kOpaqueBlack = 0xff000000u, kGreyStep = 0x10101u, kWhite = 0xffffffffu;
const unsigned kAngleMask = 0x1ffu, kSizeMask = 3u;
const int kCraftSparks = 4, kWorldSparks = 1;
const unsigned kByteMask = 0xffu;
const float kByteScale = 0.003921568859f;     // 0x1a3548: 1 / 255
const float kSparkSpread = 10.f;              // 0x1a35a4, twice in starburst
const float kSparkLead = 0.5f;                // 0x1a3504: the start ahead along the velocity
const int kSpriteCell = 10;                   // 0x9d000's EAX
const float kSpriteLife = 1.f;                // 0x3f800000
const float kSparkSpriteSize = 10.f;          // 0x1a35a4, + rand & 3, x 0.25
const float kHitSpriteSize = 20.f;            // 0x1a35d8, + rand & 3, x 0.25
const float kSpriteSizeScale = 0.25f;         // 0x1a3518

float allocationAngle(unsigned angleRandom)
{
    return float(angleRandom & kAngleMask);
}

float sparkAxis(unsigned random, float spread)
{
    return (float(random & kByteMask) * kByteScale - kSparkLead) * spread;
}

float flashSpriteHalfSize(float baseSize, unsigned sizeRandom)
{
    return (float(sizeRandom & kSizeMask) + baseSize) * kSpriteSizeScale;
}

TrailSprite flashSprite(const glm::vec3 &at, const glm::vec3 &vel, const FlashSpriteRoll &roll)
{
    TrailSprite sprite;
    sprite.pos = at;
    sprite.vel = vel;
    sprite.born = roll.now;
    sprite.life = kSpriteLife;
    sprite.spin = float(roll.spin & kAngleMask);
    sprite.startAngle = float(roll.angle & kAngleMask);
    sprite.from = kWhite;
    sprite.to = 0u;
    sprite.cell = kSpriteCell;
    return sprite;
}
}  // namespace

BeamFlashes beamFlashesOf(const BeamFrame &frame)
{
    BeamFlashes flashes;
    const bool craftHit = frame.stop == BeamStop::Craft;
    if (frame.replayed)
    {
        flashes.spark = frame.firstFrame;
        flashes.sparkSprite = frame.firstFrame;
        flashes.hit = frame.firstFrame && craftHit;
        return flashes;
    }
    flashes.startGlow = true;
    flashes.spark = frame.stop == BeamStop::World || (craftHit && frame.firstFrame);
    flashes.sparkSprite = flashes.spark && frame.firstFrame;
    flashes.hit = craftHit && frame.firstFrame;
    return flashes;
}

float beamFade(float age, float life)
{
    return 1.f - std::clamp(age / life, 0.f, 1.f);
}

// fistp in the default rounding mode: to nearest, ties to even
uint32_t greyFlashColour(float strength)
{
    const uint32_t grey = uint32_t(std::clamp(int(std::nearbyint(strength * 255.f)), 0, 255));
    return kOpaqueBlack + grey * kGreyStep;
}

WeaponFlash beamStartGlow(const glm::vec3 &start, float fade, unsigned angleRandom)
{
    return WeaponFlash{start, kStartGlowHalfSize, greyFlashColour(fade), allocationAngle(angleRandom)};
}

WeaponFlash sparkFlash(const glm::vec3 &at, float strength, unsigned angleRandom, unsigned sizeRandom)
{
    return WeaponFlash{at, kSparkHalfSize + float(sizeRandom & kSizeMask), greyFlashColour(strength), allocationAngle(angleRandom)};
}

WeaponFlash beamHitFlash(const glm::vec3 &at, unsigned angleRandom)
{
    return WeaponFlash{at, kHitFlashHalfSize, kWhite, allocationAngle(angleRandom)};
}

SparkBurst sparkBurstOf(BeamStop stop)
{
    if (stop == BeamStop::Craft)
    {
        return SparkBurst{kCraftSparks, SparkSystem::Starburst};
    }
    return SparkBurst{kWorldSparks, SparkSystem::StarburstRed};
}

SparkParticle sparkParticle(const glm::vec3 &at, SparkSystem system, const ParticleRoll &roll)
{
    const float spread = system == SparkSystem::Starburst ? kSparkSpread * kSparkSpread : kSparkSpread;
    SparkParticle particle;
    particle.vel = glm::vec3(sparkAxis(roll.x, spread), sparkAxis(roll.y, spread), sparkAxis(roll.z, spread));
    particle.pos = at + particle.vel * kSparkLead;
    particle.system = system;
    return particle;
}

TrailSprite sparkSprite(const glm::vec3 &at, const glm::vec3 &vel, const FlashSpriteRoll &roll)
{
    TrailSprite sprite = flashSprite(at, vel, roll);
    sprite.halfSize = flashSpriteHalfSize(kSparkSpriteSize, roll.size);
    return sprite;
}

TrailSprite beamHitSprite(const glm::vec3 &at, const FlashSpriteRoll &roll)
{
    TrailSprite sprite = flashSprite(at, glm::vec3(0.f), roll);
    sprite.halfSize = flashSpriteHalfSize(kHitSpriteSize, roll.size);
    return sprite;
}

glm::vec3 pointAlongBeam(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &craft)
{
    const glm::vec3 along = to - from;
    const float lengthSquared = glm::dot(along, along);
    if (lengthSquared <= 0.f)
    {
        return from;
    }
    return from + along * (glm::dot(craft - from, along) / lengthSquared);
}

}  // namespace argon::weapons
