// Argon: data\weapon.nwp's records and explosion types (logic\weapon_book.hpp; loader 0x640c0).
#include "logic/weapon_book.hpp"
#include "original/record_read.hpp"

#include <algorithm>
#include <cstring>

namespace argon::weapons
{
namespace
{
TrailSpriteStyle spriteStyle(const original::NwpWeapon &entry)
{
    TrailSpriteStyle style;
    style.cell = entry.trailType - WeaponBook::kFirstSpriteTrail;
    style.flags = entry.trailFlags;
    style.from = entry.trailFrom;
    style.to = entry.trailTo;
    style.size = entry.trailSize;
    style.sizeRange = entry.trailSizeRandom;
    style.grow = entry.trailGrow;
    style.spin = entry.trailSpin;
    style.spinRange = entry.trailSpinRandom;
    style.fade = entry.trailFade;
    style.life = entry.trailLife;
    return style;
}

WeaponRec toWeaponRec(const original::NwpWeapon &entry, int slot)
{
    WeaponRec weaponType;
    weaponType.name.assign(entry.name, strnlen(entry.name, sizeof entry.name));
    weaponType.slot = slot;
    weaponType.cls = entry.weaponClass;
    weaponType.radiusFlag = entry.radiusFlag != 0;
    weaponType.lockOn = entry.lockOn != 0;
    weaponType.effects = entry.damage;
    weaponType.shieldBonus = entry.shieldBonus;
    weaponType.impact = entry.impact;
    weaponType.shieldFlags = entry.shieldFlags;
    weaponType.retaliateSlot = entry.retaliateSlot;
    weaponType.trailType = entry.trailType;
    weaponType.trailFrom = entry.trailFrom;
    weaponType.trailTo = entry.trailTo;
    weaponType.trailStep = entry.trailStep;
    weaponType.back = entry.back;
    weaponType.trailSpeed = entry.trailSpeed;
    weaponType.trailJitter = entry.trailJitter;
    weaponType.trailLife = entry.trailLife;
    weaponType.homing = entry.homing != 0;
    weaponType.repeats = entry.repeats != 0;
    weaponType.life = entry.life;
    weaponType.repeatTime = entry.repeatTime;
    weaponType.hugBelow = entry.hugBelow != 0;
    weaponType.hugSide = entry.hugSide != 0;
    weaponType.hugAbove = entry.hugAbove != 0;
    weaponType.hugDist = entry.hugDistance;
    weaponType.turn = entry.turnRate;
    weaponType.bounces = entry.bounces != 0;
    weaponType.count = entry.count;
    weaponType.maxBounces = entry.maxBounces;
    weaponType.stagger = entry.stagger;
    weaponType.accel = entry.acceleration;
    weaponType.maxSpeed = entry.maxSpeed;
    weaponType.drop = entry.drop;
    weaponType.inherit = entry.inherit;
    weaponType.lockRange = entry.lockRange;
    weaponType.hitRadius = entry.hitRadius;
    weaponType.homingDelay = entry.homingDelay;
    weaponType.speedPct = entry.speedPercent;
    weaponType.speedAdd = entry.speedAdd;
    weaponType.holdsSpeedAfterBounce = entry.holdsSpeedAfterBounce != 0;
    weaponType.aimsAtLock = entry.aimsAtLock != 0;
    weaponType.pitchedLaunch = entry.pitchedLaunch != 0;
    weaponType.launchPitch = entry.launchPitchDegrees;
    weaponType.reactSpeed = entry.surfaceReactSpeed;
    weaponType.scatterA = entry.scatterA;
    weaponType.scatterB = entry.scatterB;
    weaponType.sprite = spriteStyle(entry);
    weaponType.fade = ShieldFade{entry.shieldFadeFrom, entry.shieldFadeTo, entry.shieldColour, entry.shieldFadeColour};
    weaponType.sphere = ShieldSphere{entry.sphereStart, entry.lockRange, entry.launchPitchDegrees, entry.hitRadius};
    return weaponType;
}

ExplosionLook explosionLookOf(const original::NwpExplosionType &explosionType)
{
    ExplosionLook look;
    look.clockFactor = explosionType.animationScale;
    look.durationFactor = explosionType.durationScale;
    look.sizeFactor = explosionType.sizeScale;
    look.flashFactor = explosionType.flashScale;
    look.flatScatter = explosionType.flatScatter != 0;
    look.tumbles = explosionType.tumbles != 0;
    look.turns = explosionType.turns != 0;
    return look;
}
}  // namespace

void WeaponBook::read(const std::vector<uint8_t> &nwp)
{
    using original::NwpPilot;
    records_.clear();
    if (nwp.size() < original::kNwpPilotCount * sizeof(NwpPilot))
    {
        return;
    }
    records_.resize(original::kNwpPilotCount);
    for (int p = 0; p < original::kNwpPilotCount; ++p)
    {
        const auto pilot = original::readAt<NwpPilot>(nwp, size_t(p) * sizeof(NwpPilot));
        resistances_[size_t(p)] = pilot.resistances;
        for (int i = 0; i < kSlots; ++i)
        {
            records_[size_t(p)][size_t(i)] = toWeaponRec(pilot.weapons[i], i);
        }
    }
    readExplosionTypes(nwp);
}

// the explosion types after the pilot blocks (0x640c0 copies them to 0x1e8bb8)
void WeaponBook::readExplosionTypes(const std::vector<uint8_t> &nwp)
{
    using original::NwpExplosionType;
    const size_t start = original::kNwpPilotCount * sizeof(original::NwpPilot);
    if (nwp.size() < start + original::kNwpExplosionTypes * sizeof(NwpExplosionType))
    {
        return;
    }
    for (int t = 0; t < original::kNwpExplosionTypes; ++t)
    {
        const auto type = original::readAt<NwpExplosionType>(nwp, start + size_t(t) * sizeof(NwpExplosionType));
        explosionTypes_[size_t(t)] = ExplosionType{explosionLookOf(type), type.pushScale};
    }
}

bool WeaponBook::hasRecords() const
{
    return !records_.empty();
}

const WeaponRec &WeaponBook::record(int pilot, int slot) const
{
    return records_[size_t(pilot & 15)][size_t(slot)];
}

const original::NwpEffects &WeaponBook::resistances(int pilot) const
{
    return resistances_[size_t(pilot & 15)];
}

const ExplosionType &WeaponBook::explosionType(int type) const
{
    return explosionTypes_[size_t(std::clamp(type, 0, original::kNwpExplosionTypes - 1))];
}

float WeaponBook::shieldSize(int pilot, int level) const
{
    return float(level) * 20.f + 100.f / ((100.f - resistances(pilot).shieldDamage) * 0.01f);
}

}  // namespace argon::weapons
