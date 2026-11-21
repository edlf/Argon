// Argon: what an explosion's push does to a craft at its next physics step (craft_blast_push 0x5dee0, the first
// piece of the physics step 0x5d670), without the craft around it (docs\physics\rumble.md "Rumble"). GameCraft::takeBlastPush
// applies it; tests\blast_knock_tests.cpp.
#pragma once
#include <glm/glm.hpp>

namespace argon::blast
{

// 0x5df3d: the push never points down (its y is made positive)
glm::vec3 upward(const glm::vec3 &push);

// 0x5df5e..0x5e0b4: the push's part across the craft's velocity (+0x68 of the state block; + 1 on x when it is 0)
// x 0.02 (0x1a36d8), 0..0.4 (0x1a3550): the momentum of both state blocks loses this x the velocity
float brakingFraction(const glm::vec3 &push, const glm::vec3 &velocity);

// 0x5e0ba: the fraction doubles for the drones (craft id +0x1c > 0) in a Tournament or Quick Race (0x52ccfc 0 / 1)
float doubledBraking(float fraction);

// 0x5e168..0x5e254: |push| x the craft's right axis (+0x268) x 10 (0x1a372c = -10) x +0x8f0 (the player 1, set by
// 0x707e0; the drone 2, 0x6d480) added to the step's force, to the right on an even rand() (0xf96ea), else to the left
glm::vec3 sidewaysForce(float pushLength, const glm::vec3 &right, float sideScale, unsigned randValue);

// +0x9c4, the drone's turbo cooldown: 0x5df6c sets it to 1 s with every push (on any craft, only the drone's turbo
// 0x6f9f0 reads it), 0x6f9f6 counts it down by the substep (at least 0), and the turbo fires only once it is 0 (0x6fa33)
class TurboCooldown
{
  public:
    void start()
    {
        seconds_ = kSeconds;
    }
    void countDown(float h);
    bool over() const
    {
        return seconds_ == 0.f;
    }

  private:
    static constexpr float kSeconds = 1.f;
    float seconds_ = 0.f;
};

constexpr float kPlayerSideScale = 1.f;   // +0x8f0, player 0x707ed
constexpr float kDroneSideScale = 2.f;    // +0x8f0, drone 0x6d4a3

}  // namespace argon::blast
