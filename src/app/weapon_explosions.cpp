// Argon: the weapons' impacts and explosions (docs\weapons\explosions_.md "Explosions"; the rules: logic\explosion.cpp).
//   impact      0x65f80: bursts A and B whose flags match the impact kind each start `count` explosions `delay`
//               apart, scattered around the impact; then a flash per burst with flag 4. The projectile update adds
//               the sparks (0x67231 a craft hit, 0x67e81 every impact but a bounce)
//   explosion   0x7e0f0 (a slot of 64 and one of its model's 16 copies, or nothing), 0x7de10 frees it once spent,
//               0x7de80 draws it (SceneRecorder::drawExplosions) and makes it go off once: its flash, then with the
//               weapon's effects the push (0x7e410), an area weapon's hit on every craft (0x64ae0) and the sound
//   knock-out   0x6a8a0: four type 6 explosions carrying nothing
#include "app/weapons.hpp"

#include "logic/explosion.hpp"

namespace
{
const uint8_t kBurstFlash = 4;
const int kCraftHitSparks = 250, kImpactSparks = 50;   // 0x67231, 0x67e81
}  // namespace

// 0x65f80: the bursts' explosions (logic\explosions_.cpp), then the impact's own flashes when any burst started
void Weapons::startImpactExplosions(const glm::vec3 &at, const WeaponRec &weaponType, int shooter, int kind)
{
    const int started = explosions_.startImpact(argon::weapons::Impact{at, weaponType, shooter, kind}, argon::weapons::ExplosionClock{host_.clock(), rand_});
    loadStartedExplosionModels();
    if (started > 0)
    {
        addImpactFlashes(at, weaponType);
    }
}

void Weapons::startKnockOutExplosions(const glm::vec3 &at)
{
    explosions_.startKnockOut(at, argon::weapons::ExplosionClock{host_.clock(), rand_});
    loadStartedExplosionModels();
}

void Weapons::loadStartedExplosionModels()
{
    for (const int model : explosions_.takeStartedModels())
    {
        host_.loadExplosionModel(model);
    }
}

// 0x6659d: the impact's flash per burst with flag 4, sized by the weapon's area inner radius
void Weapons::addImpactFlashes(const glm::vec3 &at, const WeaponRec &weaponType)
{
    const float halfSize = argon::weapons::impactFlashHalfSize(weaponType.effects.areaInnerRadius);
    for (const uint8_t flags : {weaponType.impact.flagsA, weaponType.impact.flagsB})
    {
        if (flags & kBurstFlash)
        {
            sprites_.add(argon::weapons::explosionFlash(at, halfSize, host_.clock()));
        }
    }
}

// each step: the spent ones go, those whose time has come go off; a knock-out from a blast starts explosions that
// can go off in the same step, as in the game's single pass
void Weapons::updateExplosions()
{
    for (std::vector<Explosion> goingOff = explosions_.update(host_.clock()); !goingOff.empty(); goingOff = explosions_.update(host_.clock()))
    {
        for (const Explosion &explosion : goingOff)
        {
            goOff(explosion);
        }
    }
}

// 0x7df09..0x7dfde: the flash; with the weapon's effects the push, an area weapon's hit on every craft, the sound
void Weapons::goOff(const Explosion &explosion)
{
    const argon::weapons::ExplosionLook &look = weaponBook.explosionType(explosion.type).look;
    if (look.flashFactor > 0.f)
    {
        sprites_.add(argon::weapons::explosionFlash(explosion.at, explosion.scale * look.flashFactor, host_.clock()));
    }
    if (tests_.autoFire)
    {
        std::fprintf(
            stderr, "explosion: type %d at %.2f s for %.2f s, scale %.2f%s\n", explosion.type, host_.clock(), explosion.duration, explosion.scale, explosion.carriesEffects ? ", effects" : "");
    }
    if (!explosion.carriesEffects || !explosion.weaponType)
    {
        return;
    }
    if (!host_.replayPlaying())
    {
        pushCraftsFromBlast(explosion);
    }
    if (explosion.damages)
    {
        blastEveryCraft(explosion);
    }
    sounds_.playProjectileImpact(explosion.at);
}

// 0x67231: a random direction each, 3 m/s
void Weapons::throwCraftHitSparks(const glm::vec3 &at)
{
    for (int i = 0; i < kCraftHitSparks; ++i)
    {
        argon::weapons::SparkRoll roll;
        roll.x = rand_.next();
        roll.y = rand_.next();
        roll.z = rand_.next();
        roll.life = rand_.next();
        particles_.add(particleOfSpark(argon::weapons::craftHitSpark(at, roll), host_.clock()));
    }
}

// 0x67e81: a random direction each, 10..30 m/s, upwards
void Weapons::throwImpactSparks(const glm::vec3 &at)
{
    for (int i = 0; i < kImpactSparks; ++i)
    {
        argon::weapons::SparkRoll roll;
        roll.x = rand_.next();
        roll.y = rand_.next();
        roll.z = rand_.next();
        roll.spread = rand_.next();
        roll.life = rand_.next();
        particles_.add(particleOfSpark(argon::weapons::impactSpark(at, roll), host_.clock()));
    }
}

// a spark as the particle systems run it: falling (mode 0), from `from` until `hold`, then a blend to `to`
argon::Particle Weapons::particleOfSpark(const argon::weapons::SparkParticle &spark, float now)
{
    argon::Particle particle;
    particle.src = spark.pos;
    particle.vel = spark.vel;
    particle.ship = -1;
    particle.born = now;
    particle.dur = spark.life;
    particle.delay = spark.hold;
    particle.from = spark.from;
    particle.colour = spark.to;
    particle.sys = int(spark.system);
    particle.implode = false;
    return particle;
}
