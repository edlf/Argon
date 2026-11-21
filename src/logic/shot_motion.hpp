// Argon: how a projectile is aimed at launch (0x64e20) and turned in flight (0x66d30), docs\weapons\firing.md "Projectile".
// Its frame is the bone's world matrix: right, up and forward (rows 0..2 in the game, columns here).
#pragma once
#include <glm/glm.hpp>

namespace argon::weapons
{

struct ShotFrame
{
    glm::vec3 right{1.f, 0.f, 0.f}, up{0.f, 1.f, 0.f}, forward{0.f, 0.f, 1.f};
};

// 0xb4e70 (D3DXMatrixLookAtRH's axes): forward along `direction`, right = unit((0, 1, 0) x forward), up = forward x
// right; a direction straight up or down (no right) keeps `current`
ShotFrame frameAlong(const glm::vec3 &direction, const ShotFrame &current);

// 0x65a7d (a lock-on record with +0x167 and a target): the bone turned to look at the target
ShotFrame frameAimedAt(const glm::vec3 &from, const glm::vec3 &target, const ShotFrame &current);

// 0x65b0e (+0x13f): the craft's own axes turned about their right by +0x18c degrees clamped to 0..90, nose up
// (mat_rotation_x(-angle) x the craft's matrix); the bone's own turn on the ship is dropped
ShotFrame frameRaisedFrom(const ShotFrame &craft, float pitchDegrees);

// 0x67add: after the move the frame is rebuilt along the path when the speed is above +0x148 or the record bounces,
// not on the step it bounced
bool turnsAlongPath(float speed, float reactSpeed, bool bounces, bool bouncedThisStep);

// 0x67b24: having hugged a surface, along the move with its part along the surface's normal taken out; else along
// the move itself (0x67bca)
glm::vec3 pathDirection(const glm::vec3 &moved, const glm::vec3 *hugNormal);

// 0x66f33: a bouncing record with +0x156 keeps its speed once it has bounced; else it speeds up to its maximum
float acceleratedSpeed(float speed, float acceleration, float maxSpeed, float dt, bool holdsSpeed);

// 0x64fae..0x6503e: the velocity a projectile inherits at launch, +0x180 % of the craft's velocity (its state block's
// +0x3c) without its forward part (the forward speed +0x990 x the forward axis), which is in the projectile's speed
glm::vec3 inheritedVelocity(const glm::vec3 &craftVelocity, float forwardSpeed, const glm::vec3 &craftForward, float inheritPercent);

// 0x67c7c: at the end of each step a record without bounces (+0x165) moves by its inherited velocity x dt, which
// then decays by 1 - dt x 0.4 (0x1a3550)
glm::vec3 decayedInherited(const glm::vec3 &inherited, float dt);

}  // namespace argon::weapons
