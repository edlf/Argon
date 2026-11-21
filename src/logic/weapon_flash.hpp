// Argon: the weapons' one-frame flashes (docs\weapons\shields-beams.md "Beam and spark flashes"): the beam update 0x68620 and the
// sparks 0x68150 put camera-facing quads into the additive sprite pool (0xbd670 pool 0, EAX 2, quadsprite.dds cell
// (256, 256), 128 px, angle rand & 0x1ff degrees) and mark them one-frame (0x41fa28), so 0xbd5f0 frees them before the
// next frame: they show in the frame that made them only.
#pragma once
#include <cstdint>

#include <glm/glm.hpp>

#include "logic/trail_sprite.hpp"

namespace argon::weapons
{

// one quad as 0xbd670 takes it: a square of half size `halfSize` turned by its allocation angle (0xbd670's own
// (sin, cos) slots, which nobody rewrites)
struct WeaponFlash
{
    glm::vec3 pos{0.f};
    float halfSize = 0.f;
    uint32_t argb = 0;
    float angleDegrees = 0.f;
};

// what stopped the beam this frame (0x68620: the world ray 0xd4770, then the craft test 0x683b0 along what is left;
// with no wall in the way a locked target counts as hit when the craft test misses)
enum class BeamStop
{
    Open,    // nothing: the beam ends 1000 m out
    World,   // the world, no craft before it
    Craft    // a craft (or the locked target)
};

// the beam's frame as the flashes need it
struct BeamFrame
{
    BeamStop stop = BeamStop::Open;
    bool firstFrame = false;   // the projectile's +0x28 (set at launch, cleared after its first update 0x69f7c)
    bool replayed = false;     // 0x52af9f: the beam drawn from its replay entries (0x641f0 kind 1), not cast
};

// which flashes the frame makes. Cast: the start glow every frame; a craft hit on the first frame the sparks and the
// hit flash; the world (no craft) every frame the sparks. Replayed: segment 0 has no glow (only bounce points get
// one); on the first frame a craft hit (kind 2) both, anything else (kind 1) the sparks. The sparks (0x68150) are
// their particles and flash, plus their sprite on the first frame; the hit adds the beam's own sprite (0x68fde)
struct BeamFlashes
{
    bool startGlow = false, spark = false, sparkSprite = false, hit = false;
};
BeamFlashes beamFlashesOf(const BeamFrame &frame);

// 0x68620: 1 - clamp(age / life, 0, 1): the beam's grey level (its bone tint, the start glow) and the sparks' strength
float beamFade(float age, float life);

// 0xff000000 + k x 0x10101, k = clamp(fistp(strength x 255), 0, 255)
uint32_t greyFlashColour(float strength);

// 0x688bb / 0x68df5: half size 2.5 in the beam's grey at its start (the bone's position on the ship)
WeaponFlash beamStartGlow(const glm::vec3 &start, float fade, unsigned angleRandom);

// 0x68150 (0x68311): half size 10 + (rand & 3), grey by the sparks' strength (the beam's fade), at the sparks' point
WeaponFlash sparkFlash(const glm::vec3 &at, float strength, unsigned angleRandom, unsigned sizeRandom);

// 0x68f0a / 0x69171: half size 25, white, at the beam's hit point
WeaponFlash beamHitFlash(const glm::vec3 &at, unsigned angleRandom);

// 0x68150's particles: a craft hit throws 4 into the starburst system, the world 1 into starburstred (the viewer's
// particle systems 0 and 1)
enum class SparkSystem
{
    Starburst = 0,
    StarburstRed = 1
};

struct SparkBurst
{
    int count = 0;
    SparkSystem system = SparkSystem::Starburst;
};
SparkBurst sparkBurstOf(BeamStop stop);

// one spark as neon_add_particle 0xbcb60 takes it (mode 0: it falls under its system's gravity): white for `hold`
// seconds, then a blend to transparent black by the end of its life
struct SparkParticle
{
    glm::vec3 pos{0.f}, vel{0.f};
    SparkSystem system = SparkSystem::Starburst;
    float life = 1.f, hold = 0.8f;
    uint32_t from = 0xffffffffu, to = 0u;
};

// the three rand() the game draws per spark: x, y, z
struct ParticleRoll
{
    unsigned x = 0, y = 0, z = 0;
};

// velocity per axis ((rand & 0xff) / 255 - 0.5) x 10, x 10 again in starburst; it starts at `at` + half its velocity
SparkParticle sparkParticle(const glm::vec3 &at, SparkSystem system, const ParticleRoll &roll);

// the rand() a 0x9d000 sprite draws in the beam code, in their order: spin, start angle, size
struct FlashSpriteRoll
{
    float now = 0.f;
    unsigned spin = 0, angle = 0, size = 0;
};

// 0x68150 on the first frame -> 0x9d000 (EAX 10: quadsprite.dds cell 10, list B additive): white to transparent black
// over 1 s, no growth, spin and start angle rand & 0x1ff degrees, half size ((rand & 3) + 10) / 4, moving with the
// last spark's velocity
TrailSprite sparkSprite(const glm::vec3 &at, const glm::vec3 &vel, const FlashSpriteRoll &roll);

// 0x68fde / 0x69245: the beam's own after a craft hit, as the sparks' but still and half size ((rand & 3) + 20) / 4
TrailSprite beamHitSprite(const glm::vec3 &at, const FlashSpriteRoll &roll);

// 0x683b0's hit point: on the beam, as far along it as the craft's centre is (not the centre itself)
glm::vec3 pointAlongBeam(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &craft);

}  // namespace argon::weapons
