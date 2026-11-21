// Argon: a replay's weapons (docs\replays\README.md "Weapon entry"), played back as craft_weapon 0x641f0 does: the
// projectiles launched and then placed from their recorded states, the impacts only shown (0x65f80), the shields taking
// their recorded values (0x6ac90, knocking out at 0). The entries: logic\replay_weapons.cpp; reading them and the
// recording: app\app_replay_weapons.cpp.
#include "app/weapons.hpp"

#include "logic/race.hpp"

namespace
{
constexpr int kShieldClass = 2;
constexpr int kBeamClass = 1;
constexpr int kImpactOnCraft = 1, kImpactBounced = 4;   // 0x65f80's kinds (weapon_shots.cpp)
constexpr float kStrayShotSeconds = 2.f;   // a replayed shot whose done entry never came goes after its life + this
}  // namespace

// 0x6ac90: the shield's value; at 0 or less the craft is knocked out (0x6a8a0) by the entry's killer
void Weapons::setReplayedShield(const argon::replay::WeaponEvent &event)
{
    const int slot = std::clamp(int(event.craft), 0, kShips - 1);
    craftWeapons[slot].replayShield(argon::replay::shieldOf(event));
    if (craftWeapons[slot].energy() <= 0.f && !craftWeapons[slot].knockedOut())
    {
        knockOut(slot, argon::replay::killerOf(event));
    }
}

// 0x6a390: a weapon slot fires; a shield (class 2) goes up with it (its projectiles come as launches)
void Weapons::fireReplayedWeapon(const argon::replay::WeaponEvent &event)
{
    const WeaponRec *weaponType = pilotWeapon(event.craft, argon::replay::fireSlotOf(event));
    if (weaponType && weaponType->cls == kShieldClass)
    {
        raiseShield(event.craft, *weaponType);
    }
}

// weapon_launch: the projectile leaves its bone on the shooter
void Weapons::launchReplayedShot(const argon::replay::WeaponEvent &event)
{
    const argon::replay::ProjectileId id = argon::replay::projectileOf(event);
    const WeaponRec *weaponType = pilotWeapon(id.craft, id.slot);
    const Found<const argon::GameCraft> shooter = race_.craftInSlot(id.craft);
    if (!weaponType || !shooter || weaponType->cls == kShieldClass)
    {
        return;
    }
    const int bone = weaponBone(id.craft, *weaponType, id.projectile);
    Shot shot = makeShot(*shooter, ShotLaunch{id.craft, weaponType->slot % 3, weaponType}, boneOnShip(id.craft, bone), id.projectile);
    shot.bone = bone;
    shot.target = argon::replay::launchTargetOf(event);
    shots_.push_back(shot);
}

// a projectile's state (kind 1): placed, not flown
void Weapons::moveReplayedShot(const argon::replay::WeaponEvent &event)
{
    Shot *shot = replayedShot(argon::replay::projectileOf(event));
    if (!shot)
    {
        return;
    }
    shot->pos = argon::replay::statePositionOf(event);
    const glm::vec3 forward = argon::replay::stateForwardOf(event);
    if (glm::length(forward) > 0.5f)
    {
        shot->fwd = glm::normalize(forward);
    }
}

void Weapons::endReplayedShot(const argon::replay::WeaponEvent &event)
{
    const argon::replay::ProjectileId id = argon::replay::projectileOf(event);
    for (size_t i = 0; i < shots_.size(); ++i)
    {
        if (projectileIdOf(shots_[i]) == id)
        {
            shots_[i] = shots_.back();
            shots_.pop_back();
            return;
        }
    }
}

// 0x65f80 in a replay: what an impact shows by its kind (a craft hit's starburst sparks, its explosions and flashes,
// the 50 starburstred sparks but on a bounce); its damage is in the shield entries
void Weapons::replayImpact(const argon::replay::WeaponEvent &event)
{
    const int kind = argon::replay::impactKindOf(event);
    const glm::vec3 at = argon::replay::impactPositionOf(event);
    const argon::replay::ProjectileId id = argon::replay::projectileOf(event);
    if (kind == kImpactOnCraft)
    {
        throwCraftHitSparks(at);
    }
    const WeaponRec *weaponType = pilotWeapon(id.craft, id.slot);
    if (weaponType)
    {
        startImpactExplosions(at, *weaponType, id.craft, kind);
    }
    if (kind != kImpactBounced)
    {
        throwImpactSparks(at);
    }
}

Weapons::Shot *Weapons::replayedShot(const argon::replay::ProjectileId &id)
{
    for (Shot &shot : shots_)
    {
        if (projectileIdOf(shot) == id)
        {
            return &shot;
        }
    }
    return nullptr;
}

// each replayed race step: the trails behind the placed projectiles, the beams on their bones (their hits only show),
// the explosions
void Weapons::stepReplayed()
{
    sprites_.dropFlashes();   // 0xbd5f0: the last frame's one-frame quads
    for (size_t i = 0; i < shots_.size();)
    {
        Shot &shot = shots_[i];
        if (shot.weaponType->cls == kBeamClass ? stepBeam(shot) : host_.clock() - shot.born > shot.weaponType->life + kStrayShotSeconds)
        {
            shots_[i] = shots_.back();
            shots_.pop_back();
            continue;
        }
        emitShotTrail(shot);
        ++i;
    }
    updateExplosions();
}
