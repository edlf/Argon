// Argon: the lasting effects of a weapon hit on a craft (docs\weapons\README.md "Effects on the craft"): corrode,
// freeze, control / engine jitter and engine off, as the craft weapon object keeps them (+0x8..+0x24). A hit sets
// them (weapon_damage 0x6acf0), craft_weapon_update (0x6d000) counts them down and corrodes the shield, the craft's
// physics reads them (GameCraft::setWeaponEffects: thrust 0x5e370, steering 0x5f9d0).
#pragma once
#include <vector>

#include "original/weapon_records.hpp"

namespace argon::weapons
{

struct CraftEffects
{
    float corrodeLevel = 0;        // +0x8 shield % lost per second ...
    float corrodeTime = 0;         // +0xc ... for this long
    float freezeTime = 0;          // +0x10 no throttle
    float controlJitter = 0;       // +0x14 the steering inputs drop by up to this ...
    float controlJitterTime = 0;   // +0x18 ... for this long
    float engineJitter = 0;        // +0x1c the throttle drops by up to this ...
    float engineJitterTime = 0;    // +0x20 ... for this long
    float engineOffTime = 0;       // +0x24 no throttle

    bool cutsEngine() const
    {
        return freezeTime > 0.f || engineOffTime > 0.f;
    }
    bool jittersEngine() const
    {
        return engineJitterTime > 0.f;
    }
    bool jittersControls() const
    {
        return controlJitterTime > 0.f;
    }
};

// 0x6aa00: the target's 17 resistances: its pilot's, with channel 0 the shield resistance (shieldResistance), and
// every active shield's bonus (+0x90) added to channels 1, 2, 4, 5, 7 and 9
original::NwpEffects targetResistances(const original::NwpEffects &pilot, float shieldResistance,
                                       const std::vector<const original::NwpEffects *> &activeShields);

// 0x6acf0: channel k x the area factor x (100 - its resistance) %: corrode level against the shield resistance (0),
// corrode time against none, freeze 4, control jitter level and time 5, engine jitter level and time 7, engine off 9
CraftEffects scaleEffects(const original::NwpEffects &damage, const original::NwpEffects &resistance, float areaScale);

// 0x6b1b8: each running effect takes the larger of its value and the hit's
void takeLonger(CraftEffects &running, const CraftEffects &hit);

// 0x6d000: every time counts down to 0; a jitter's or the corrode's level goes to 0 with its time
void countDown(CraftEffects &running, float dt);

// 0x6d000: the shield % the corrode takes this frame (after countDown); it stops at 0 % without knocking out
float corrodeLoss(const CraftEffects &running, float dt);

// 0x5e4b8 / 0x5fa92: a jittered input loses (rand & 15) / 15 of the level, at most down to 0
float jitter(float input, float level, unsigned random);

}  // namespace argon::weapons
