// Argon: projectiles and beams in flight (craftweapon.cpp in the game, docs\weapons\README.md).
//   projectile  update 0x66d30 (motion, homing, surface hugging, bounces, craft and world hits, the frame turned along
//               the path: logic\shot_motion.cpp), trail 0x66650
//   beam        update 0x68620 (class 1): from the ship's bone to the target or 1000 m ahead, hits on its first frame
//   impact      0x65f80 (kind 1 craft, 2 world, 3 expired, 4 bounce): the bursts' explosions (weapon_explosions.cpp), then
//               50 starburstred sparks; a craft hit throws 250 starburst sparks first
#include "app/weapons.hpp"

#include "app/app_race_crafts.hpp"
#include "logic/race.hpp"

namespace
{
const float kBeamHitRadius = 5.f;      // 0x68620: weapon_craft_hit_test(.., 5.0)
const uint32_t kAreaWeapon = 0x400;
const int kBeamClass = 1;

enum ImpactKind
{
    kHitCraft = 1,
    kHitWorld = 2,
    kExpired = 3,
    kBounced = 4
};
}  // namespace

// one 60 Hz step; true once the shot is spent (hit a craft, the world without bouncing, or out of life). After the
// move: the craft test, the world ray (a bounce goes on), the hug rays, then the frame turned along the path
bool Weapons::stepShot(Shot &shot)
{
    if (shot.weaponType->cls == kBeamClass)
    {
        return stepBeam(shot);
    }
    const WeaponRec &weaponType = *shot.weaponType;
    const float age = host_.clock() - shot.born;
    steerShot(shot, age);
    const glm::vec3 old = shot.pos;
    moveShot(shot);
    warnCraftsOnPath(shot.shooter, Segment{old, shot.pos}, argon::weapons::shotHitRadius(weaponType));
    const int hit = craftHitAlong(shot.shooter, Segment{old, shot.pos}, argon::weapons::shotHitRadius(weaponType));
    const SimWorldRay world(host_.world());
    const WorldHit wall = argon::weapons::worldHitAlong(shot, old, hit >= 0, world);
    emitShotTrail(shot);
    if (hit >= 0)
    {
        return impactCraft(shot, hit);
    }
    if (wall.hit && !argon::weapons::canBounce(shot))
    {
        shot.pos = wall.point;
        finishShot(shot, kHitWorld);
        return true;
    }
    if (wall.hit)
    {
        bounceOff(shot, wall);
    }
    const std::optional<glm::vec3> hugNormal = argon::weapons::hugSurfaces(shot, wall, world);
    if (argon::weapons::turnsAlongPath(shot.speed, weaponType.reactSpeed, weaponType.bounces, wall.hit))
    {
        const glm::vec3 along = argon::weapons::pathDirection(shot.pos - old, hugNormal ? &*hugNormal : nullptr);
        argon::weapons::setShotFrame(shot, argon::weapons::frameAlong(along, argon::weapons::shotFrameOf(shot)));
    }
    argon::weapons::carryInheritedVelocity(shot);
    if (age <= weaponType.life)
    {
        return false;
    }
    finishShot(shot, kExpired);
    return true;
}

// 0x66e45..0x6712d: homing picks a target, a knocked-out target is dropped, the speed and the drop grow, and past the
// homing delay the shot turns to its target
void Weapons::steerShot(Shot &shot, float age)
{
    const WeaponRec &weaponType = *shot.weaponType;
    warnCraftsNear(shot.pos, shot.shooter);   // 0x66e60
    const argon::weapons::RaceCrafts &crafts = host_.crafts();
    if (weaponType.homing && shot.target < 0)
    {
        shot.target = argon::weapons::homingTarget(shot, crafts, rand_);
    }
    if (shot.target >= 0 && craftWeapons[size_t(shot.target)].knockedOut())
    {
        shot.target = -1;   // 0x66fd9
    }
    argon::weapons::gatherSpeed(shot);
    if (shot.target >= 0 && age > weaponType.homingDelay)
    {
        if (crafts.flies(shot.target))
        {
            argon::weapons::steerTowards(shot, crafts.position(shot.target));
        }
        attackWarnings.warn(shot.target);   // 0x67038
        sounds_.playIncoming(shot.target, host_.soundState());          // 0x6704b
    }
    argon::weapons::squareShotFrame(shot);
}

void Weapons::moveShot(Shot &shot)
{
    argon::weapons::moveShot(shot);
    sounds_.moveTravelLoop(travelKeyOf(shot), shot.pos);   // 0x671a7
}

// 0x68620: the beam starts at its bone on the ship each frame and reaches the locked target, else 1000 m along the
// ship's forward; the world stops it; a craft within 5 m of it is hit, on the beam's first frame only (+0x28); its
// flashes, sparks and sprites (flashBeam) each frame
bool Weapons::stepBeam(Shot &shot)
{
    if (host_.clock() - shot.born > shot.weaponType->life)
    {
        return true;
    }
    shot.pos = glm::vec3(boneOnShip(shot.shooter, shot.bone)[3]);
    shot.beamEnd = argon::weapons::beamAimPoint(shot, shooterMatrix(shot.shooter), lockedTargetPosition(shot));
    const BeamCast cast = castBeam(shot);
    if (shot.target >= 0 && !craftWeapons[shot.target].knockedOut())
    {
        attackWarnings.warn(shot.target);   // 0x68d1a
        sounds_.playIncoming(shot.target, host_.soundState());          // 0x68d2d
    }
    if (cast.craft >= 0)
    {
        sounds_.playLaserImpact(cast.craft, host_.soundState());
    }
    if (cast.craft >= 0 && shot.firstStep)
    {
        applyHit(WeaponHit{cast.craft, shot.shooter, shot.weaponType, shot.beamEnd});
        race_.craftInSlot(cast.craft)->addBeamRumble();   // 0x68f5a: the target's +0x9dc += 50000
    }
    host_.flashBeam(shot, cast.stop);
    shot.firstStep = false;
    return false;
}

// the world ray (0xd4770) shortens the beam, then the craft test (0x683b0) along what is left ends it at the hit
// craft's distance along it
Weapons::BeamCast Weapons::castBeam(Shot &shot)
{
    const std::optional<argon::weapons::WorldContact> wall = SimWorldRay(host_.world()).along(shot.pos, shot.beamEnd);
    if (wall)
    {
        shot.beamEnd = wall->point;
    }
    warnCraftsOnPath(shot.shooter, Segment{shot.pos, shot.beamEnd}, kBeamHitRadius);
    const int hit = craftHitAlong(shot.shooter, Segment{shot.pos, shot.beamEnd}, kBeamHitRadius);
    if (hit >= 0)
    {
        shot.beamEnd = argon::weapons::pointAlongBeam(shot.pos, shot.beamEnd, race_.craftInSlot(hit)->position());
        return BeamCast{argon::weapons::BeamStop::Craft, hit};
    }
    return BeamCast{wall ? argon::weapons::BeamStop::World : argon::weapons::BeamStop::Open, -1};
}

// the shot's target while it is in the race
std::optional<glm::vec3> Weapons::lockedTargetPosition(const Shot &shot) const
{
    const Found<const argon::GameCraft> target = shot.target >= 0 && !craftWeapons[size_t(shot.target)].knockedOut() ? race_.craftInSlot(shot.target) : std::nullopt;
    if (!target)
    {
        return std::nullopt;
    }
    return target->position();
}

int Weapons::craftHitAlong(int shooter, const Segment &path, float radius) const
{
    return argon::weapons::craftHitAlong(shooter, path, radius, host_.crafts());
}

// 0x66e60: every other craft within 300 m of a projectile is in danger
void Weapons::warnCraftsNear(const glm::vec3 &point, int shooter)
{
    for (const int slot : argon::weapons::craftsNear(point, shooter, host_.crafts()))
    {
        attackWarnings.warn(slot);
    }
}

// 0x683b0 (0x68591): a craft in the path's way (argon::pathWarns) is in danger
void Weapons::warnCraftsOnPath(int shooter, const Segment &path, float radius)
{
    for (const int slot : argon::weapons::craftsOnPath(shooter, path, radius, host_.crafts()))
    {
        attackWarnings.warn(slot);
    }
}

// trail (0x66650): a particle every +0xfc along the path, drifting back along the forward
void Weapons::emitShotTrail(Shot &shot)
{
    const WeaponRec &weaponType = *shot.weaponType;
    if (weaponType.trailType <= 0 || weaponType.trailStep <= 0.f)
    {
        return;
    }
    const glm::vec3 trail = shot.pos - shot.trailPos;
    const float length = glm::length(trail);
    const bool sprites = weaponType.trailType >= argon::weapons::WeaponBook::kFirstSpriteTrail;
    for (float a = weaponType.trailStep; a <= length; a += weaponType.trailStep)
    {
        const glm::vec3 at = shot.trailPos + trail * (a / length) - shot.fwd * weaponType.back;
        if (sprites)
        {
            host_.emitTrailSprite(shot, at);
        }
        else if (particles_.hasRoom())
        {   // trailParticle rolls the jitter only for a particle that fits
            particles_.add(trailParticle(shot, at));
        }
    }
    if (length >= weaponType.trailStep)
    {
        shot.trailPos = shot.pos;
    }
}

argon::Particle Weapons::trailParticle(const Shot &shot, const glm::vec3 &at)
{
    const WeaponRec &weaponType = *shot.weaponType;
    argon::Particle particle;
    particle.src = at;
    particle.vel = trailVelocity(shot);
    particle.ship = -1;
    particle.born = host_.clock();
    particle.dur = std::max(0.05f, weaponType.trailLife);
    particle.delay = particle.dur * 0.8f;
    particle.from = weaponType.trailFrom;
    particle.colour = weaponType.trailTo;
    particle.sys = weaponType.trailType == 2 ? 1 : weaponType.trailType == 3 ? 2 : 0;
    particle.implode = false;
    return particle;
}

// 0x66650: back along the forward at +0x104, plus (rand & 255) - 128 x +0x108 / 128 per axis
glm::vec3 Weapons::trailVelocity(const Shot &shot)
{
    const glm::vec3 jitter(float(int(rand_.next() & 255) - 128), float(int(rand_.next() & 255) - 128), float(int(rand_.next() & 255) - 128));
    return -shot.fwd * shot.weaponType->trailSpeed + jitter * shot.weaponType->trailJitter * 0.0078125f;
}

// impact kind 1 (0x67231): 250 starburst sparks; area weapons (0x400) do no direct damage, their explosions do
bool Weapons::impactCraft(const Shot &shot, int target)
{
    throwCraftHitSparks(shot.pos);
    if (!(shot.weaponType->effects.flags & kAreaWeapon))
    {
        applyHit(WeaponHit{target, shot.shooter, shot.weaponType, shot.pos});
    }
    finishShot(shot, kHitCraft);
    return true;
}

// kind 4 (0x675f2): the geometry (logic\shot_flight.cpp), then the replay entry and the bursts
void Weapons::bounceOff(Shot &shot, const WorldHit &wall)
{
    argon::weapons::bounceOff(shot, wall);
    host_.recordWeaponEvent(argon::replay::impactEvent(projectileIdOf(shot), kBounced, shot.pos));
    startImpactExplosions(shot.pos, *shot.weaponType, shot.shooter, kBounced);
}

// 0x65f80, then (0x67e66) the special's impact sound and 50 starburstred sparks
void Weapons::finishShot(const Shot &shot, int kind)
{
    host_.recordWeaponEvent(argon::replay::impactEvent(projectileIdOf(shot), kind, shot.pos));
    startImpactExplosions(shot.pos, *shot.weaponType, shot.shooter, kind);
    if (shot.weaponType->slot == kSpecialSlot)
    {
        sounds_.playMegaImpact(shot.pos);   // 0x67e7c
    }
    throwImpactSparks(shot.pos);
    logImpact(shot, kind);
}

void Weapons::logImpact(const Shot &shot, int kind) const
{
    if (tests_.autoFire)
    {
        std::fprintf(stderr, "impact: shooter %d %s%d kind %d target %d at %.2f s\n", shot.shooter, shot.weaponType->name.c_str(), shot.index, kind, shot.target, host_.clock());
    }
}

void Weapons::recordShotState(const Shot &shot)
{
    host_.recordWeaponEvent(argon::replay::projectileStateEvent(projectileIdOf(shot), shot.pos, shot.fwd * shot.speed + shot.vel, shot.fwd));
}
