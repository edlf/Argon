#include "logic/explosion.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace argon::weapons
{
namespace
{
const float kPercent = 0.01f;               // 0x1a355c
const unsigned kTenBits = 0x3ffu;
const float kScatterCentre = 512.f;         // 0x200
const float kScatterScale = 1.f / 512.f;    // 0x1a35bc
const float kTurnScale = 1.f / 1023.f;      // 0x1a3718
const float kFullTurn = 360.f;              // 0x1a34fc
const float kGreyFull = 255.f;              // 0x1a3564
const int kFlashCell = 15;                  // 0x9d000's EAX 0xf
const float kFlashLife = 0.5f;              // 0x3f000000
const float kFlashGrowth = 0.f;             // 0x9d000's growth argument 0
const uint32_t kWhite = 0xffffffffu;
const float kMinImpactFlash = 10.f;         // 0x1a35a4
const unsigned kByteMask = 0xffu;
const float kByteScale = 0.003921568859f;   // 0x1a3548: 1 / 255
const float kHalf = 0.5f;                   // 0x1a3504
const float kSparkLead = 0.1f;              // 0x1a3508
const float kCraftHitSpeed = 3.f;           // 0x1a351c
const float kCraftHitLife = 3.f;            // 0x1a351c
const float kImpactSpeed = 10.f;            // 0x1a35a4
const float kImpactSpeedRange = 20.f;       // 0x1a35d8
const float kImpactLife = 1.f;              // 0x1a3454
const float kSparkHold = 0.8f;              // 0x1a361c: the colour holds this share of the life

const float kKnockOutBlastDuration = 5.f, kKnockOutBlastSize = 3.f;   // 0x40a00000, 0x40400000
const float kKnockOutEchoDuration = 2.f, kKnockOutEchoSize = 1.f;     // 0x40000000, 0x3f800000
const float kKnockOutEchoStep = 0.2f;                                 // 0x1a358c
const unsigned kElevenBits = 0x7ffu;
const float kKnockOutCentre = 1024.f;                                 // 0x400
const float kKnockOutReach = 20.f / 1024.f;                           // 0x1a35b8 x 0x1a35d8

float knockOutAxis(unsigned random)
{
    return (float(random & kElevenBits) - kKnockOutCentre) * kKnockOutReach;
}

float tenBitTurn(unsigned random)
{
    return float(random & kTenBits) * kTurnScale * kFullTurn;
}

float scatterAxis(unsigned random, float scatter)
{
    return (float(random & kTenBits) - kScatterCentre) * kScatterScale * scatter;
}

float byteShare(unsigned random)
{
    return float(random & kByteMask) * kByteScale;
}

// 0x67240 / 0x67e90: ((rand & 0xff) / 255 - 0.5) per axis, normalized (a zero vector stays zero)
glm::vec3 sparkDirection(const SparkRoll &roll)
{
    const glm::vec3 d(byteShare(roll.x) - kHalf, byteShare(roll.y) - kHalf, byteShare(roll.z) - kHalf);
    const float length = glm::length(d);
    return length > 0.f ? d / length : glm::vec3(0.f);
}

// it starts 0.1 x `lead` out from `at`
SparkParticle sparkOf(const glm::vec3 &at, const glm::vec3 &lead, const glm::vec3 &vel, SparkSystem system, float life)
{
    SparkParticle particle;
    particle.pos = at + lead * kSparkLead;
    particle.vel = vel;
    particle.system = system;
    particle.life = life;
    particle.hold = life * kSparkHold;
    return particle;
}
}  // namespace

int explosionModelOf(int type)
{
    return type >= 0 && type < kExplosionModels ? type : kExplosionModels - 1;
}

ExplosionOrder burstExplosionOrder(uint8_t sizePercent)
{
    const float share = float(sizePercent) * kPercent;
    return ExplosionOrder{share, share};
}

ExplosionSize explosionSize(const ExplosionLook &look, const ExplosionOrder &order)
{
    return ExplosionSize{order.duration * look.durationFactor, look.sizeFactor * kPercent * order.size};
}

glm::vec3 burstScatter(const TripleRoll &roll, float scatter, bool flat)
{
    const float y = flat ? 0.f : scatterAxis(roll.second, scatter);
    return glm::vec3(scatterAxis(roll.first, scatter), y, scatterAxis(roll.third, scatter));
}

glm::vec3 explosionTurnDegrees(const ExplosionLook &look, const TripleRoll &roll)
{
    if (look.turns)
    {
        return glm::vec3(tenBitTurn(roll.second), 0.f, 0.f);
    }
    if (look.tumbles)
    {
        return glm::vec3(tenBitTurn(roll.second), tenBitTurn(roll.first), tenBitTurn(roll.third));
    }
    return glm::vec3(0.f);
}

glm::mat4 explosionMatrix(const glm::vec3 &at, const glm::vec3 &turnDegrees, float scale)
{
    glm::mat4 placement = glm::translate(glm::mat4(1.f), at);
    placement = glm::rotate(placement, glm::radians(turnDegrees.x), glm::vec3(0.f, 1.f, 0.f));
    placement = glm::rotate(placement, glm::radians(turnDegrees.y), glm::vec3(1.f, 0.f, 0.f));
    placement = glm::rotate(placement, glm::radians(turnDegrees.z), glm::vec3(0.f, 0.f, 1.f));
    return glm::scale(placement, glm::vec3(scale));
}

bool explosionShows(float start, float duration, float now)
{
    return start <= now && now <= start + duration;
}

bool explosionSpent(float start, float duration, float now)
{
    return start + duration < now;
}

float explosionProgress(float start, float duration, float now)
{
    return duration > 0.f ? (now - start) / duration : 1.f;
}

uint32_t explosionGrey(float progress)
{
    const int level = std::clamp(255 - int(std::nearbyint(kGreyFull * progress)), 0, 255);
    const uint32_t grey = uint32_t((level * 255) >> 8);
    return grey * 0x01010101u;
}

TrailSprite explosionFlash(const glm::vec3 &at, float halfSize, float now)
{
    TrailSprite sprite;
    sprite.pos = at;
    sprite.born = now;
    sprite.life = kFlashLife;
    sprite.halfSize = halfSize;
    sprite.grow = kFlashGrowth;
    sprite.from = kWhite;
    sprite.to = 0u;
    sprite.cell = kFlashCell;
    return sprite;
}

float impactFlashHalfSize(float areaInnerRadius)
{
    return areaInnerRadius < kMinImpactFlash ? kMinImpactFlash : areaInnerRadius;
}

ExplosionOrder knockOutBlastOrder()
{
    return ExplosionOrder{kKnockOutBlastDuration, kKnockOutBlastSize};
}

ExplosionOrder knockOutEchoOrder()
{
    return ExplosionOrder{kKnockOutEchoDuration, kKnockOutEchoSize};
}

float knockOutEchoDelay(int echo)
{
    return float(echo) * kKnockOutEchoStep;
}

glm::vec3 knockOutEchoOffset(const TripleRoll &roll)
{
    return glm::vec3(knockOutAxis(roll.first), knockOutAxis(roll.second), knockOutAxis(roll.third));
}

bool ExplosionSlots::take(int model)
{
    const int modelIndex = explosionModelOf(model);
    if (total_ >= kExplosionSlots || taken_[size_t(modelIndex)] >= kExplosionCopies)
    {
        return false;
    }
    ++taken_[size_t(modelIndex)];
    ++total_;
    return true;
}

void ExplosionSlots::release(int model)
{
    const int modelIndex = explosionModelOf(model);
    if (taken_[size_t(modelIndex)] > 0)
    {
        --taken_[size_t(modelIndex)];
        --total_;
    }
}

void ExplosionSlots::clear()
{
    taken_.fill(0);
    total_ = 0;
}

SparkParticle craftHitSpark(const glm::vec3 &at, const SparkRoll &roll)
{
    const glm::vec3 direction = sparkDirection(roll);
    const float life = kCraftHitLife + byteShare(roll.life) * kHalf;
    return sparkOf(at, direction, direction * kCraftHitSpeed, SparkSystem::Starburst, life);
}

SparkParticle impactSpark(const glm::vec3 &at, const SparkRoll &roll)
{
    glm::vec3 vel = sparkDirection(roll) * (byteShare(roll.spread) * kImpactSpeedRange + kImpactSpeed);
    vel.y = std::abs(vel.y);
    const float life = kImpactLife + byteShare(roll.life) * kHalf * kHalf;
    return sparkOf(at, vel, vel, SparkSystem::StarburstRed, life);
}

}  // namespace argon::weapons
