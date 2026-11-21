// Argon: a craft's weapon object (logic\craft_weapon.hpp; 0x6a390, 0x6aa00, 0x6aba0, 0x6b1b8, 0x6b7e0, 0x6d000,
// 0x6a8a0, 0x69970).
#include "logic/craft_weapon.hpp"

#include <algorithm>

namespace argon::weapons
{
namespace
{
const uint8_t kShieldAbsorbs = 4;
}  // namespace

void CraftWeapon::resetForRace()
{
    shield_ = ActiveShield{};
    aiLoadout_ = PowerLevels{};
    aiTrigger_ = AiTrigger{};
    knockedOut_ = false;
    effects_ = CraftEffects{};
    hitFlash_.clear();
    shieldFlash_.clear();
}

void CraftWeapon::equipShield(float size)
{
    shieldMax_ = size;
    energy_ = size;
}

void CraftWeapon::replayShield(float value)
{
    energy_ = value;
}

float CraftWeapon::shieldPercent() const
{
    return shieldMax_ > 0.f ? energy_ / shieldMax_ * 100.f : 0.f;
}

bool CraftWeapon::shieldUp(float now) const
{
    return shield_.weaponType && now - shield_.since <= shield_.weaponType->life;
}

std::vector<const original::NwpEffects *> CraftWeapon::shieldBonuses(float now) const
{
    if (!shieldUp(now))
    {
        return {};
    }
    return {&shield_.weaponType->shieldBonus};
}

ShieldGuard CraftWeapon::guard(float now) const
{
    if (!shieldUp(now))
    {
        return {};
    }
    return {shield_.weaponType->shieldBonus.shieldDamage > 0.f, (shield_.weaponType->shieldFlags & kShieldAbsorbs) != 0};
}

void CraftWeapon::raiseAiLoadout(int pickupType, const PowerLevels &caps)
{
    aiLoadout_ = raisePowerLevels(aiLoadout_, pickupType, caps);
}

AiFire CraftWeapon::pullAiTrigger(const AiWants &wants)
{
    return advanceTrigger(aiTrigger_, wants);
}

void CraftWeapon::delayAiFire(float delay)
{
    aiTrigger_.redDelay = delay;
    aiTrigger_.blueDelay = delay;
}

void CraftWeapon::raiseShield(const WeaponRec &weaponType, float now, int bone)
{
    shield_ = ActiveShield{&weaponType, now, 0, bone};
}

void CraftWeapon::rechargeFromShield(float points)
{
    gainShield(points);
    effects_ = CraftEffects{};
}

void CraftWeapon::loseShield(float percent)
{
    energy_ = std::min(energy_ - percent * 0.01f * shieldMax_, shieldMax_);
}

void CraftWeapon::spare(float percent)
{
    energy_ = percent * 0.01f * shieldMax_;
}

void CraftWeapon::gainShield(float points)
{
    if (points == 0.f || knockedOut_)
    {
        return;
    }
    energy_ = std::min(energy_ + points, shieldMax_);
}

std::vector<int> CraftWeapon::reachWithSphere(const ShieldSphereAt &sphere, const RaceCrafts &crafts)
{
    std::vector<int> reached;
    for (int racer = 0; racer < crafts.racerCount(); ++racer)
    {
        const int other = crafts.slotOf(racer);
        const uint16_t bit = uint16_t(1u << other);
        if (other == sphere.owner || (shield_.sphereReached & bit) || glm::length(crafts.position(other) - sphere.centre) >= sphere.radius)
        {
            continue;
        }
        shield_.sphereReached |= bit;
        reached.push_back(other);
    }
    return reached;
}

bool CraftWeapon::afflict(const CraftEffects &hit)
{
    const bool stopsEngine = hit.engineOffTime > effects_.engineOffTime;
    takeLonger(effects_, hit);
    return stopsEngine;
}

bool CraftWeapon::countDownEffects(float dt)
{
    const bool engineWasOff = effects_.engineOffTime > 0.f;
    countDown(effects_, dt);
    return engineWasOff && effects_.engineOffTime <= 0.f;
}

void CraftWeapon::corrode(float dt, float share)
{
    if (energy_ <= 0.f || effects_.corrodeTime <= 0.f)
    {
        return;
    }
    const float loss = corrodeLoss(effects_, dt) * share * 0.01f * shieldMax_;
    energy_ = std::max(energy_ - loss, 0.f);
    shieldFlash_.raise(HitFlash::kPerCorrodeFrame);
}

void CraftWeapon::flashShieldHit()
{
    shieldFlash_.raise(HitFlash::kPerHit);
}

void CraftWeapon::flashPlayerHit()
{
    hitFlash_.raise(HitFlash::kPerHit);
}

void CraftWeapon::fadeFlashes(float dt)
{
    shieldFlash_.fade(dt);
    hitFlash_.fade(dt);
}

void CraftWeapon::knockOut(int shooter, float now)
{
    knockedOut_ = true;
    knockedOutAt_ = now;
    knockedOutBy_ = shooter;
    energy_ = 0.f;
}

}  // namespace argon::weapons
