// Argon: the lasting effects of a weapon hit on a craft (0x6acf0 / 0x6d000), see weapon_effects.hpp.
#include "logic/weapon_effects.hpp"

#include <algorithm>

namespace argon::weapons
{

namespace
{
float scaled(float channel, float resistance, float areaScale)
{
    return areaScale * channel * (100.f - resistance) * 0.01f;
}

void countDownTime(float &time, float dt)
{
    if (time > 0.f)
    {
        time = std::max(time - dt, 0.f);
    }
}

void countDownLevel(float &level, float &time, float dt)
{
    if (time <= 0.f)
    {
        return;
    }
    countDownTime(time, dt);
    if (time == 0.f)
    {
        level = 0.f;
    }
}
}  // namespace

original::NwpEffects targetResistances(const original::NwpEffects &pilot, float shieldResistance,
                                       const std::vector<const original::NwpEffects *> &activeShields)
{
    original::NwpEffects effects = pilot;
    effects.shieldDamage = shieldResistance;
    for (const original::NwpEffects *bonus : activeShields)
    {
        effects.shieldLeech += bonus->shieldLeech;
        effects.corrodeLevel += bonus->corrodeLevel;
        effects.freezeTime += bonus->freezeTime;
        effects.controlJitter += bonus->controlJitter;
        effects.engineJitterLevel += bonus->engineJitterLevel;
        effects.engineOffTime += bonus->engineOffTime;
    }
    return effects;
}

CraftEffects scaleEffects(const original::NwpEffects &damage, const original::NwpEffects &resistance, float areaScale)
{
    CraftEffects effects;
    effects.corrodeLevel = scaled(damage.corrodeLevel, resistance.shieldDamage, areaScale);
    effects.corrodeTime = scaled(damage.corrodeTime, 0.f, areaScale);
    effects.freezeTime = scaled(damage.freezeTime, resistance.freezeTime, areaScale);
    effects.controlJitter = scaled(damage.controlJitter, resistance.controlJitter, areaScale);
    effects.controlJitterTime = scaled(damage.controlJitterTime, resistance.controlJitter, areaScale);
    effects.engineJitter = scaled(damage.engineJitterLevel, resistance.engineJitterLevel, areaScale);
    effects.engineJitterTime = scaled(damage.engineJitterTime, resistance.engineJitterLevel, areaScale);
    effects.engineOffTime = scaled(damage.engineOffTime, resistance.engineOffTime, areaScale);
    return effects;
}

void takeLonger(CraftEffects &running, const CraftEffects &hit)
{
    running.controlJitter = std::max(running.controlJitter, hit.controlJitter);
    running.controlJitterTime = std::max(running.controlJitterTime, hit.controlJitterTime);
    running.freezeTime = std::max(running.freezeTime, hit.freezeTime);
    running.engineJitter = std::max(running.engineJitter, hit.engineJitter);
    running.engineJitterTime = std::max(running.engineJitterTime, hit.engineJitterTime);
    running.corrodeLevel = std::max(running.corrodeLevel, hit.corrodeLevel);
    running.corrodeTime = std::max(running.corrodeTime, hit.corrodeTime);
    running.engineOffTime = std::max(running.engineOffTime, hit.engineOffTime);
}

void countDown(CraftEffects &running, float dt)
{
    countDownTime(running.freezeTime, dt);
    countDownLevel(running.controlJitter, running.controlJitterTime, dt);
    countDownLevel(running.engineJitter, running.engineJitterTime, dt);
    countDownTime(running.engineOffTime, dt);
    countDownLevel(running.corrodeLevel, running.corrodeTime, dt);
}

float corrodeLoss(const CraftEffects &running, float dt)
{
    return running.corrodeTime > 0.f ? dt * running.corrodeLevel : 0.f;
}

float jitter(float input, float level, unsigned random)
{
    const float loss = float(random & 15u) * level * (1.f / 15.f);
    return std::max(input - loss, 0.f);
}

}  // namespace argon::weapons
