// Argon: what a weapon does to a craft (craftweapon.cpp in the game, docs\weapons\README.md "Hits").
//   hit         weapon_damage 0x6acf0 (the arithmetic: logic\weapon_damage.cpp), leech 0x6b7e0
//   shields     launch 0x6a390 (class 2: recharge, effects cleared), resistances 0x6aa00, blocked 0x6aba0,
//               absorb 0x6ab60, retaliate 0x6abf0 -> 0x6a6c0
//   blasts      an explosion going off (weapon_explosions.cpp, 0x7de80) hits every craft (0x64ae0) and pushes the crafts
//               around it (0x7e410: the rumble's knock, GameCraft::pushByBlast)
//   knock-out   0x6a8a0 -> craftdeath 0x61fc0 / 0x58d20 (+0x1145, DNF from the back of the finish order 0x638b0)
//   effects     the timers set by the hit (0x6b1b8), counted down and corroding (craft_weapon_update 0x6d000),
//               read by the craft's physics (logic\weapon_effects.cpp, GameCraft::setWeaponEffects)
//   nemesis     in a nemesis race (0x791f0) craft 1 takes half the hit's shield damage (0x6b085) and corrode
//               (0x6d313), and its shield launch recharges half (0x6a439)
//   sphere      0x69970: a shield's growing sphere gives each craft it reaches the shield's impact once
//   flash       a human player's hit with shield damage flashes its position list row red (0x6d430, faded 0x6d3a7)
// Not ported: the AI's flash (0x6d400: +0x94, a SHIELD bone no craft has), the push's speed loss and sideways force
// (0x5dee0), the engine off / on sounds (0xc2ef0 / 0xc2f40).
#include "app/weapons.hpp"

#include "logic/race.hpp"
#include "logic/weapon_damage.hpp"
#include "logic/weapon_effects.hpp"

namespace
{
const uint8_t kShieldRetaliates = 0x10;
const int kShieldClass = 2;
const int kImpactOnCraft = 1;
const int kPlayerSlot = 0;
const float kSimStep = 1.f / 60.f;   // craft_weapon_update runs once per 60 Hz frame

// 0x6a3fc: the shield's launch recharges its owner by 10 / 20 / 30 points (TSHIELD levels 1..3, slots 1 / 4 / 7)
float shieldRecharge(int slot)
{
    return slot == 1 ? 10.f : slot == 4 ? 20.f : slot == 7 ? 30.f : 0.f;
}
}  // namespace

bool Weapons::shieldActive(int slot) const
{
    return craftWeapons[size_t(std::clamp(slot, 0, kShips - 1))].shieldUp(host_.clock());
}

std::vector<const argon::original::NwpEffects *> Weapons::shieldBonuses(int slot) const
{
    return craftWeapons[size_t(slot)].shieldBonuses(host_.clock());
}

argon::weapons::ShieldGuard Weapons::shieldGuard(int slot) const
{
    return craftWeapons[size_t(slot)].guard(host_.clock());
}

// 0x6acf0: area falloff from `at`, the target's resistances and shield, the shooter's leech, the shield damage, the
// single player's mercy and the knock-out at 0
void Weapons::applyHit(const WeaponHit &hit)
{
    if (host_.replayPlaying())
    {
        return;   // a replay's shields come from its shield entries (signal 1, Weapons::playWeaponEvent)
    }
    const Found<const argon::GameCraft> target = race_.craftInSlot(hit.target);
    if (!target || hit.target == hit.shooter || craftWeapons[hit.target].knockedOut())
    {
        return;
    }
    const auto area = argon::weapons::areaFactor(hit.weaponType->effects, glm::length(target->position() - hit.at));
    if (!area)
    {
        return;
    }
    const argon::weapons::ShieldGuard guard = shieldGuard(hit.target);
    if (!guard.absorbs)
    {
        retaliate(hit.target, hit.shooter);
    }
    const auto bonuses = shieldBonuses(hit.target);
    const float shieldResistance = argon::weapons::shieldResistance(craftWeapons[hit.target].shieldMax(), bonuses);
    const argon::original::NwpEffects resistances = argon::weapons::targetResistances(weaponBook.resistances(target->pilot()), shieldResistance, bonuses);
    const argon::weapons::HitResistance resistance{resistances.shieldDamage, resistances.shieldLeech};
    const argon::weapons::ScaledHit scaled = argon::weapons::scaleHit(hit.weaponType->effects, resistance, *area, guard);
    if (scaled.shieldPercent > 0.f)
    {
        sounds_.playPlayerHit(hit.target, hit.at, host_.soundState());
    }
    leechTo(hit.shooter, scaled.shieldPercent * 0.01f * craftWeapons[hit.target].shieldMax() * scaled.leechPercent * 0.01f);
    if (hit.target == 0 && host_.infiniteShield())
    {
        return;   // Infinite Shield (0x1bbbf0)
    }
    if (scaled.shieldPercent > 0.f)
    {
        flashShieldHit(hit.target);
    }
    const float nemesisShare = scaled.shieldPercent > 0.f ? nemesisShareOf(hit.target) : 1.f;
    takeShieldDamage(hit.target, scaled.shieldPercent * nemesisShare);
    logHit(hit, scaled.shieldPercent * nemesisShare);
    scoreHit(hit, scaled.shieldPercent);
    if (!guard.absorbs && !guard.blocked)
    {
        afflict(hit.target, argon::weapons::scaleEffects(hit.weaponType->effects, resistances, *area));
    }
    if (craftWeapons[hit.target].energy() <= 0.f)
    {
        knockOut(hit.target, hit.shooter);
        scoreKnockOut(hit);
    }
}

// 0x6b19a: weapon_damage raises the craft's shield flash by 0.8 (0x6d400: +0x94), a human craft's red flash too
// (0x6d430: +0x94 and +0x98, slot < 0x52cd8c)
void Weapons::flashShieldHit(int target)
{
    craftWeapons[target].flashShieldHit();
    if (target >= host_.humanPlayers())
    {
        return;
    }
    craftWeapons[target].flashPlayerHit();
    if (tests_.autoFire)
    {
        std::fprintf(stderr, "hit flash: slot %d at %.2f s, level %.2f\n", target, host_.raceClock(), craftWeapons[target].hitFlashLevel());
    }
}

// --autofire runs (tests): each hit's shield loss (0 behind a blocking shield, negative when one absorbs)
void Weapons::logHit(const WeaponHit &hit, float lostPercent) const
{
    if (tests_.autoFire)
    {
        std::fprintf(stderr,
                     "hit: slot %d by %d %s: shield lost %.1f %%, now %.1f %% at %.2f s\n",
                     hit.target,
                     hit.shooter,
                     hit.weaponType->name.c_str(),
                     lostPercent + 0.f,
                     shieldPercent(hit.target),
                     host_.clock());
    }
}

// the shield loses `percent` of its size; the player's last hit from 33 % or more leaves 1..6 % (0x6b6ee)
void Weapons::takeShieldDamage(int target, float percent)
{
    argon::weapons::CraftWeapon &weapon = craftWeapons[size_t(target)];
    const float before = weapon.shieldPercent();
    weapon.loseShield(percent);
    if (target == 0 && weapon.energy() <= 0.f && argon::weapons::sparesThePlayer(before))
    {
        weapon.spare(argon::weapons::mercyPercent(rand_.next()));
    }
}

// 0x6b7e0: the shooter's shield gains `amount` points, at most to full
void Weapons::leechTo(int shooter, float amount)
{
    if (shooter >= 0 && shooter < kShips)
    {
        craftWeapons[size_t(shooter)].gainShield(amount);
    }
}

// 0x6b0d3 / 0x83f90: the player's hit scores per shield point it takes ("Attack", kind 0; argon::weapons::attackPoints),
// one that takes none "Opponent Shielded" (kind 14, no points); another craft's hit on the player posts "Hit by" its
// pilot (kind 15, 0x84010). Only while the race runs (0x52af9f).
void Weapons::scoreHit(const WeaponHit &hit, float shieldPercentLost)
{
    if (!host_.raceRunning())
    {
        return;
    }
    if (hit.target == kPlayerSlot)
    {
        host_.awardPoints(0, 15, host_.pilotOf(hit.shooter));
        return;
    }
    if (hit.shooter != kPlayerSlot)
    {
        return;
    }
    if (shieldPercentLost <= 0.f)
    {
        host_.awardPoints(0, 14, 0);
        return;
    }
    const float shieldSize = craftWeapons[size_t(hit.target)].shieldMax();
    host_.awardPoints(argon::weapons::attackPoints(shieldPercentLost, shieldSize, isPlayersNemesis(hit.target), host_.cashValue("weaponhit_bonus")), 0, 0);
}

// 0x83e00: the player's knock-out scores kills_bonus ("Killed" with the pilot, kind 2), twice that on the player's
// nemesis ("Nemesis Kill", kind 11)
void Weapons::scoreKnockOut(const WeaponHit &hit)
{
    if (hit.shooter != kPlayerSlot || !host_.raceRunning())
    {
        return;
    }
    const int killPoints = host_.cashValue("kills_bonus");
    if (isPlayersNemesis(hit.target))
    {
        host_.awardPoints(killPoints * 2, 11, 0);
        return;
    }
    host_.awardPoints(killPoints, 2, host_.pilotOf(hit.target));
}

// 0x6b0de: the target's pilot is the player's nemesis (pilot id +- 8)
bool Weapons::isPlayersNemesis(int slot) const
{
    return droneKindOf(slot) == argon::weapons::DroneKind::Nemesis;
}

// class 2 (0x6a390): up for the record's life; it recharges its owner (slots 1 / 4 / 7)
void Weapons::raiseShield(int shooter, const WeaponRec &weaponType)
{
    const int ship = std::clamp(shooter, 0, kShips - 1);
    if (craftWeapons[ship].shield().weaponType && craftWeapons[ship].shield().weaponType != &weaponType)
    {   // the game's other weapon object would run its shield out; the viewer keeps one shield a craft
        sounds_.stopTravelLoop({ship, craftWeapons[ship].shield().weaponType->slot, 0});
    }
    craftWeapons[ship].raiseShield(weaponType, host_.clock(), weaponBone(ship, weaponType, 0));
    sounds_.playLaunch(travelLaunchOf(weaponType, ship, 0), host_.soundState());
    if (weaponType.shieldBonus.shieldDamage > 0.f)
    {
        craftWeapons[ship].rechargeFromShield(shieldRecharge(weaponType.slot) * nemesisShareOf(ship));
    }
}

// 0x69970: while the sphere's ages run its radius grows from the shield bone; each other craft closer than it gets the
// shield's impact (kind 1: its bursts) once (+0x16d0). Every TSHIELD's bursts are empty: nothing happens in the data.
// Its travel loop follows the sphere and stops with the shield; neither once the race is over (0x52af9f).
void Weapons::updateShieldSpheres()
{
    for (int owner = 0; owner < kShips; ++owner)
    {
        const ActiveShield &shield = craftWeapons[owner].shield();
        if (shield.weaponType && !shieldActive(owner) && host_.raceRunning())
        {
            sounds_.stopTravelLoop({owner, shield.weaponType->slot, 0});   // 0x699cd
        }
        if (!shieldActive(owner))
        {
            continue;
        }
        const auto radius = argon::weapons::shieldSphereRadius(shield.weaponType->sphere, host_.clock() - shield.since);
        if (radius)
        {
            reachWithSphere(owner, *radius);
            holdShieldTravelLoop(owner);
        }
    }
}

// 0x69cd2: at the shield bone, its distances from the sphere's final radius (+0x190), not the growing one
void Weapons::holdShieldTravelLoop(int owner)
{
    const WeaponRec &weaponType = *craftWeapons[owner].shield().weaponType;
    if (!host_.raceRunning())
    {
        return;
    }
    const argon::sound::TravelKey key{owner, weaponType.slot, 0};
    sounds_.moveTravelLoop(key, shieldCentre(owner));
    sounds_.spreadRadialTravelLoop(key, weaponType.sphere.r1);
}

glm::vec3 Weapons::shieldCentre(int owner) const
{
    return glm::vec3(boneOnShip(owner, craftWeapons[owner].shield().bone)[3]);
}

void Weapons::reachWithSphere(int owner, float radius)
{
    argon::weapons::CraftWeapon &weapon = craftWeapons[size_t(owner)];
    const argon::weapons::ShieldSphereAt sphere{owner, shieldCentre(owner), radius};
    for (const int other : weapon.reachWithSphere(sphere, host_.crafts()))
    {
        startImpactExplosions(race_.craftInSlot(other)->position(), *weapon.shield().weaponType, owner, kImpactOnCraft);
    }
}

// 0x6abf0 / 0x6a6c0: a shield with flag 0x10 answers a hit with its pilot's slot +0x1b3 (10, RETALIATOR), every
// projectile aimed at the attacker; not while that slot is still out
void Weapons::retaliate(int victim, int attacker)
{
    if (!shieldActive(victim) || !(craftWeapons[victim].shield().weaponType->shieldFlags & kShieldRetaliates))
    {
        return;
    }
    const WeaponRec *weaponType = pilotWeapon(victim, craftWeapons[victim].shield().weaponType->retaliateSlot);
    if (!weaponType || weaponType->cls == kShieldClass || weaponType->count <= 0 || shotInFlight(victim, kRetaliateColour))
    {
        return;
    }
    fireVolley(ShotLaunch{victim, kRetaliateColour, weaponType}, std::vector<int>(size_t(weaponType->count), attacker));
    if (tests_.autoFire)
    {
        std::fprintf(stderr, "retaliate: slot %d %s at %d, %.2f s\n", victim, weaponType->name.c_str(), attacker, host_.clock());
    }
}

// 0x64ae0: the explosion hits every craft; the area falloff picks who it reaches
void Weapons::blastEveryCraft(const Explosion &blast)
{
    for (int j = 0; j < race_.racerCount(); ++j)
    {
        applyHit(WeaponHit{race_.slotOf(j), blast.shooter, blast.weaponType, blast.at});
    }
}

// 0x7de80 -> 0x7e410: every craft but the shooter's is pushed away from the explosion (blastPush, along the craft minus the
// explosion, x + 1 when they coincide); a craft with its shield up drops the push (0x5dee0). The drones (craft id > 0)
// brake twice as hard in a Tournament or Quick Race (0x5e0ba)
void Weapons::pushCraftsFromBlast(const Explosion &explosion)
{
    const float typeScale = weaponBook.explosionType(explosion.type).push;
    for (int j = 0; j < race_.racerCount(); ++j)
    {
        const int slot = race_.slotOf(j);
        const Found<argon::GameCraft> craft = race_.craftInSlot(slot);
        if (!craft || slot == explosion.shooter || shieldActive(slot))
        {
            continue;
        }
        glm::vec3 away = craft->position() - explosion.at;
        const float push = argon::weapons::blastPush(explosion.weaponType->effects, glm::length(away), typeScale);
        if (push <= 0.f)
        {
            continue;
        }
        if (away == glm::vec3(0.f))
        {
            away.x += 1.f;
        }
        craft->setBlastBrakingDoubled(slot > 0 && host_.doublesDroneBlastBraking());
        craft->pushByBlast(glm::normalize(away) * push);
        if (tests_.autoFire)
        {
            std::fprintf(stderr, "blast push: slot %d by %.2f at %.2f s, %.1f km/h\n", slot, push, host_.clock(), craft->speedKmh());
        }
    }
}

// 0x6b1b8: unless a shield absorbs or blocks the hit, each running effect takes the longer of its and the hit's
void Weapons::afflict(int target, const argon::weapons::CraftEffects &hit)
{
    if (craftWeapons[size_t(target)].afflict(hit) && enginePowerSounds[size_t(target)].powerDown())
    {
        playEnginePowerDown(target);
    }
    const argon::weapons::CraftEffects &effects = craftWeapons[size_t(target)].effects();
    if (tests_.autoFire)
    {
        std::fprintf(stderr,
                     "effects: slot %d engine off %.2f freeze %.2f engine jitter %.1f/%.2f control jitter %.1f/%.2f corrode %.1f/%.2f\n",
                     target,
                     effects.engineOffTime,
                     effects.freezeTime,
                     effects.engineJitter,
                     effects.engineJitterTime,
                     effects.controlJitter,
                     effects.controlJitterTime,
                     effects.corrodeLevel,
                     effects.corrodeTime);
    }
}

// 0x6d000 (racing crafts, not knocked out): the effects count down, the corrode eats the shield; the craft's physics
// gets them next frame (0x5b1d4)
void Weapons::updateCraftEffects()
{
    for (int j = 0; j < race_.racerCount(); ++j)
    {
        const int slot = race_.slotOf(j);
        if (craftWeapons[slot].knockedOut())
        {
            continue;
        }
        restartEngineAfter(slot, kSimStep);
        craftWeapons[size_t(slot)].corrode(kSimStep, nemesisShareOf(slot));
        craftWeapons[size_t(slot)].fadeFlashes(kSimStep);
        race_.craftInSlot(slot)->setWeaponEffects(craftWeapons[slot].effects());
    }
}

// 0x6d000: the effects count down; the engine-off time running out plays the engine power up sound (0x6d261)
void Weapons::restartEngineAfter(int slot, float dt)
{
    if (craftWeapons[size_t(slot)].countDownEffects(dt) && enginePowerSounds[size_t(slot)].powerUp())
    {
        playEnginePowerUp(slot);
    }
}

void Weapons::playEnginePowerUp(int slot)
{
    logEnginePower("up", slot);
    host_.playShipSound(argon::ShipSound::EnginePowerUp, slot);
}

void Weapons::playEnginePowerDown(int slot)
{
    logEnginePower("down", slot);
    host_.playShipSound(argon::ShipSound::EnginePowerDown, slot);
}

void Weapons::logEnginePower(const char *change, int slot) const
{
    if (tests_.autoFire)
    {
        std::fprintf(stderr, "engine power %s: slot %d\n", change, slot);
    }
}


// 0x6a8a0: the craft is out of the race (DNF, kills to the shooter) in explosions (weapon_explosions.cpp)
void Weapons::knockOut(int target, int shooter)
{
    craftWeapons[size_t(target)].knockOut(shooter, host_.clock());
    race_.knockOut(target);
    if (shooter != target)
    {
        race_.countKill(shooter);
    }
    if (target == 0)
    {
        host_.playerKnockedOutBy(shooter);
    }
    startKnockOutExplosions(race_.craftInSlot(target)->position());
    host_.retireCraft(target);
    if (tests_.autoFire)
    {
        std::fprintf(stderr, "knock-out: slot %d by %d at %.2f s\n", target, shooter, host_.clock());
    }
}
