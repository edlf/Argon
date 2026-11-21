// Argon: weapon trail sprites (trail types > 3, docs\weapons\firing.md "Trail sprites") and the hover's dust (docs\effects\README.md
// "Dust"): 0x66650 / 0x5e6d0 -> 0x9d000 puts them in the spray pool, 0x9c540 moves, grows, spins and colours them, the
// sprite pass (0xbd8f0) draws list A alpha blended and list B additive. The beams' and sparks' one-frame flashes
// (0x68620 / 0x68150 -> 0xbd670) join list B; argon::WeaponSprites (effects\weapon_sprites.cpp) holds and draws them.
// Logic: logic\trail_sprite.cpp, logic\hover_dust.cpp, logic\weapon_flash.cpp; the texture is the spray's (the track's
// quadsprite.dds).
#include "app/app.hpp"

// 0x66650 for types > 3: velocity back along the forward plus jitter, as the point particles
void App::emitTrailSprite(const Shot &shot, const glm::vec3 &at)
{
    if (!weaponSprites.hasRoom())
    {
        return;
    }
    const unsigned spinRandom = rnd() & 0x3ffu, sizeRandom = rnd() & 0x3ffu;
    const bool flip = (rnd() & 1u) != 0;
    weaponSprites.add(argon::weapons::makeTrailSprite(shot.weaponType->sprite, at, weapons.trailVelocity(shot), simClock, spinRandom, sizeRandom, flip));
}

// craft_hover 0x5f3b3..0x5f4e6: after the roll, rand() for the drift, the alpha, the spin, the angle and the size
void App::raiseDust(const argon::GameCraft &craft)
{
    if (craft.dustPuffs().empty())
    {
        return;
    }
    const argon::DustTint tint = argon::dustTintFor(courseId(track.name));
    for (const argon::DustPuff &puff : craft.dustPuffs())
    {
        if (!weaponSprites.hasRoom())
        {
            return;
        }
        argon::DustRoll roll;
        roll.driftX = rnd();
        roll.driftZ = rnd();
        roll.alphaA = rnd();
        roll.alphaB = rnd();
        roll.spin = rnd();
        roll.angle = rnd();
        roll.size = rnd();
        roll.now = simClock;
        weaponSprites.add(argon::makeDustSprite(puff, tint, roll));
    }
}

// 0x68620 / 0x68150: the frame's flashes, sparks and sprites by what the beam met (logic\weapon_flash.hpp), rand()
// in the game's order
void App::flashBeam(const Shot &shot, argon::weapons::BeamStop stop)
{
    const argon::weapons::BeamFlashes flashes = argon::weapons::beamFlashesOf(argon::weapons::BeamFrame{stop, shot.firstStep, replayRace.playing()});
    if (flashes.startGlow)
    {
        weaponSprites.addFlash(argon::weapons::beamStartGlow(shot.pos, argon::weapons::beamFade(simClock - shot.born, shot.weaponType->life), rnd()));
    }
    if (flashes.spark)
    {
        throwSparks(shot, flashes, stop);
    }
    if (flashes.hit)
    {
        weaponSprites.addFlash(argon::weapons::beamHitFlash(shot.beamEnd, rnd()));
        weaponSprites.add(argon::weapons::beamHitSprite(shot.beamEnd, rollFlashSprite()));
    }
}

// 0x68150 at the beam's end: its particles, its flash, and on the first frame its sprite with the last particle's
// velocity
void App::throwSparks(const Shot &shot, const argon::weapons::BeamFlashes &flashes, argon::weapons::BeamStop stop)
{
    const glm::vec3 lastVelocity = throwSparkParticles(shot.beamEnd, stop);
    const float strength = argon::weapons::beamFade(simClock - shot.born, shot.weaponType->life);
    const unsigned angleRandom = rnd();
    weaponSprites.addFlash(argon::weapons::sparkFlash(shot.beamEnd, strength, angleRandom, rnd()));
    if (flashes.sparkSprite)
    {
        weaponSprites.add(argon::weapons::sparkSprite(shot.beamEnd, lastVelocity, rollFlashSprite()));
    }
}

// neon_add_particle 0xbcb60 for each spark (the systems drop what they cannot hold); the last one's velocity
glm::vec3 App::throwSparkParticles(const glm::vec3 &at, argon::weapons::BeamStop stop)
{
    const argon::weapons::SparkBurst burst = argon::weapons::sparkBurstOf(stop);
    glm::vec3 lastVelocity(0.f);
    for (int i = 0; i < burst.count; ++i)
    {
        argon::weapons::ParticleRoll roll;
        roll.x = rnd();
        roll.y = rnd();
        roll.z = rnd();
        const argon::weapons::SparkParticle spark = argon::weapons::sparkParticle(at, burst.system, roll);
        lastVelocity = spark.vel;
        particles.add(Weapons::particleOfSpark(spark, simClock));
    }
    return lastVelocity;
}

// 0x9d000's spin, start angle and size, drawn in that order
argon::weapons::FlashSpriteRoll App::rollFlashSprite()
{
    argon::weapons::FlashSpriteRoll roll;
    roll.now = simClock;
    roll.spin = rnd();
    roll.angle = rnd();
    roll.size = rnd();
    return roll;
}

