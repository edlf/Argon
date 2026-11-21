// Argon: the weapons' explosions (docs\weapons\explosions.md "Explosions"): an impact's bursts A / B (0x65f80) each start
// `count` explosions (0x7e0f0) of a type from weapon.nwp's explosion table (0x1e8bb8). An explosion is the model
// objects\explosion_type<n> (16 copies each, loaded by 0x7dd40), drawn by 0x7de80 while it lasts: scaled, turned at
// random by its type, its bones' tracks at the progress x the type's clock factor, every bone tinted a grey that
// falls to black. It goes off once (its flash, push, damage and sound) when its start time comes.
#pragma once
#include <array>
#include <cstdint>

#include <glm/glm.hpp>

#include "logic/trail_sprite.hpp"
#include "logic/weapon_flash.hpp"

namespace argon::weapons
{

constexpr int kExplosionModels = 16;       // objects\explosion_type0..15 (0x7dd40); a type past 15 shows 15 (0x7e0f9)
constexpr int kExplosionCopies = 16;       // instances per model (0x1ea478: 16 used flags a type)
constexpr int kExplosionSlots = 64;        // the explosion pool (0x1ea580, 0x140 bytes each)

// weapon.nwp's explosion type (0xa4 bytes) as the explosions read it
struct ExplosionLook
{
    float clockFactor = 1.f;      // +0: the bones' clock = progress x this (0x7e077)
    float durationFactor = 1.f;   // +4: the duration asked for x this (0x7e117)
    float sizeFactor = 100.f;     // +8: the scale = size x this x 0.01 (0x7e125)
    float flashFactor = 0.f;      // +0xc: the flash's half size = the scale x this, none when not above 0 (0x7df15)
    bool flatScatter = false;     // +0x20: an impact burst scatters its explosions in x / z only (0x6601f)
    bool tumbles = false;         // +0x21: yaw, pitch and roll at random (0x7e247)
    bool turns = false;           // +0x22: only the yaw at random (0x7e22f)
};

int explosionModelOf(int type);

// what 0x7e0f0 is asked for: the duration and the size before the type's factors
struct ExplosionOrder
{
    float duration = 0.f, size = 0.f;
};

// 0x65f80: a burst's explosions take its size % x 0.01 as both the duration and the size
ExplosionOrder burstExplosionOrder(uint8_t sizePercent);

// 0x7e0f0: the duration x the type's factor, the scale = size x the type's size x 0.01
struct ExplosionSize
{
    float duration = 0.f, scale = 0.f;
};
ExplosionSize explosionSize(const ExplosionLook &look, const ExplosionOrder &order);

// three rand() & 0x3ff as 0x65f80 / 0x7e0f0 draw them, in their order
struct TripleRoll
{
    unsigned first = 0, second = 0, third = 0;
};

// 0x65f80: each explosion of a burst at the impact + ((rand & 0x3ff) - 512) / 512 x the burst's scatter per axis
// (+0xe4 burst A, +0xe8 burst B); no y with the flat flag of burst A's type (0x6601f / 0x66310, both bursts)
glm::vec3 burstScatter(const TripleRoll &roll, float scatter, bool flat);

// 0x7e1e1..0x7e296: yaw, pitch and roll (degrees) of (rand & 0x3ff) / 1023 x 360: the second roll is the yaw, the
// first the pitch, the third the roll; a tumbling type keeps all three, a turning one the yaw, others none
glm::vec3 explosionTurnDegrees(const ExplosionLook &look, const TripleRoll &roll);

// D3DXMatrixRotationYawPitchRoll (0xf3117) with its rows x the scale (0x7e29b..0x7e363), at `at`
glm::mat4 explosionMatrix(const glm::vec3 &at, const glm::vec3 &turnDegrees, float scale);

// 0x7de80 shows an explosion from its start to start + duration, both included; 0x7de10 frees it once past that
bool explosionShows(float start, float duration, float now);
bool explosionSpent(float start, float duration, float now);
float explosionProgress(float start, float duration, float now);

// 0x7dfe3..0x7e0a1: every bone's tint (+0x104, 0xb99b0) is grey g x 255 >> 8, g = clamp(255 - round(255 x progress))
uint32_t explosionGrey(float progress);

// 0x7df35: the flash as it goes off: 0x9d000 with cell 15, still, 0.5 s white to transparent black, half size the
// scale x the type's flash factor, shrinking (growth 0), no spin, additive
TrailSprite explosionFlash(const glm::vec3 &at, float halfSize, float now);

// 0x6659d: an impact that set off any burst adds this flash per burst with flag 4: the same sprite, half size the
// weapon's area inner radius, 10 when that is below 10
float impactFlashHalfSize(float areaInnerRadius);

// a knock-out (0x6a8a0): type 6 explosions carrying nothing (no push, damage or sound): one at the craft (5 s, size
// 3), then three (2 s, size 1) the k-th 0.2 k s later, each ((rand & 0x7ff) - 0x400) / 1024 x 20 m off per axis
constexpr int kKnockOutExplosionType = 6;
constexpr int kKnockOutEchoes = 3;
ExplosionOrder knockOutBlastOrder();
ExplosionOrder knockOutEchoOrder();
float knockOutEchoDelay(int k);
glm::vec3 knockOutEchoOffset(const TripleRoll &roll);

// 0x7e0f0's room: an explosion takes one of the 64 slots and one of its model's 16 copies; with none free it is not
// made at all (no flash, push, damage or sound)
class ExplosionSlots
{
  public:
    bool take(int model);
    void release(int model);
    void clear();

  private:
    std::array<int, kExplosionModels> taken_{};
    int total_ = 0;
};

// the impact's sparks (0x66d30 after 0x65f80): the three rand() of the direction, then the one of the spread or life
struct SparkRoll
{
    unsigned x = 0, y = 0, z = 0, spread = 0, life = 0;
};

// 0x67231: a craft hit throws 250 into starburst: a random unit direction x 3 m/s, 0.1 m out along it, life 3 + (rand
// & 0xff) / 255 x 0.5
SparkParticle craftHitSpark(const glm::vec3 &at, const SparkRoll &roll);

// 0x67e81: every impact but a bounce throws 50 into starburstred: a random direction x (10 + (rand & 0xff) / 255 x
// 20) m/s with its y made positive, 0.1 x its velocity out, life 1 + (rand & 0xff) / 255 x 0.25
SparkParticle impactSpark(const glm::vec3 &at, const SparkRoll &roll);

}  // namespace argon::weapons
