// Argon: the damage arithmetic of a weapon hit (0x6acf0 / 0x6aa00), see weapon_damage.hpp.
#include "logic/weapon_damage.hpp"

#include <algorithm>

namespace argon::weapons
{

namespace
{
const uint32_t kAreaWeapon = 0x400;
const float kFull = 100.f;
const float kPushRampFrom = 0.01f, kPushScale = 0.01f;   // 0x1a355c both

float percentLeft(float resistance)
{
    return (kFull - resistance) * 0.01f;
}
}  // namespace

std::optional<float> areaFactor(const original::NwpEffects &damage, float distance)
{
    if (!(damage.flags & kAreaWeapon))
    {
        return 1.f;
    }
    const float span = damage.areaOuterRadius - damage.areaInnerRadius;
    if (span <= 0.f)
    {
        return std::nullopt;
    }
    const float falloff = std::max(distance - damage.areaInnerRadius, 0.f) / span;
    if (falloff >= 1.f)
    {
        return std::nullopt;
    }
    const float inner = damage.areaInnerPercent * 0.01f, outer = damage.areaOuterPercent * 0.01f;
    return (inner - outer) * (1.f - falloff) + outer;
}

float blastPush(const original::NwpEffects &damage, float distance, float typeScale)
{
    const float pastInner = distance - damage.areaInnerRadius;
    float percent = damage.areaInnerPercent;
    if (pastInner > 0.f)
    {
        if (distance >= damage.areaOuterRadius)
        {
            return 0.f;
        }
        if (pastInner >= kPushRampFrom)
        {
            const float share = pastInner / (damage.areaOuterRadius - damage.areaInnerRadius);
            percent += (damage.areaOuterPercent - damage.areaInnerPercent) * share;
        }
    }
    return percent * typeScale * kPushScale;
}

float shieldResistance(float shieldSize, const std::vector<const original::NwpEffects *> &activeShields)
{
    float resistance = kFull - kFull * kFull / shieldSize;
    for (const original::NwpEffects *bonus : activeShields)
    {
        resistance = std::min(resistance + bonus->shieldDamage, kFull);
    }
    return resistance;
}

ScaledHit scaleHit(const original::NwpEffects &damage, const HitResistance &resistance, float areaScale, const ShieldGuard &guard)
{
    ScaledHit hit;
    hit.shieldPercent = areaScale * damage.shieldDamage * percentLeft(resistance.shield);
    hit.leechPercent = areaScale * damage.shieldLeech * percentLeft(resistance.leech);
    if (guard.absorbs)
    {
        hit.shieldPercent = -hit.shieldPercent;
    }
    if (guard.blocked)
    {
        hit.leechPercent = 0.f;
        hit.shieldPercent = std::min(hit.shieldPercent, 0.f);
    }
    return hit;
}

bool sparesThePlayer(float percentBeforeHit)
{
    return percentBeforeHit >= kMercyFromPercent;
}

float mercyPercent(unsigned random10Bits)
{
    return 1.f + float(random10Bits & 0x3ffu) * (5.f / 1023.f);
}

}  // namespace argon::weapons
