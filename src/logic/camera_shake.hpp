// Argon: the internal camera's shake (+0x9cc..+0x9d4, its timer +0x9d8), as craft_rumble 0x5b610 runs it and the
// internal camera's update 0x5ba10 applies it, and 0x5ba10's contact jolt (+0x8b4), without the craft around them
// (docs\physics\rumble.md "Rumble"). The chase cameras (0x5be00) read neither. GameCraft::rumble / GameCraft::collide /
// GameCraft::tripod use them; tests\blast_knock_tests.cpp.
#pragma once
#include <glm/glm.hpp>

namespace argon::shake
{

// 0x5b6ae..0x5b70x: one axis of the frame's random direction, (rand() - 0x3fff) / 32767 (0x1a3748) x 0.1 (0x1a3508)
float randomAxis(unsigned randValue);

// one frame of the shake: the surface rumble's amplitude (the motor's, before x 300000), the surface's rumblehertz_
struct ShakeFrame
{
    float dt = 0.f;
    float amplitude = 0.f;
    float hertz = 0.f;
    glm::vec3 direction{0.f};   // three randomAxis values, x / y / z in rand() order
};

class CameraShake
{
  public:
    // 0x5b770..0x5b8b0: the shake fades x (1 - 2 dt); when the timer runs out (<= 0) it gains 1 / hertz (at least 0;
    // a hertz of 0 never runs out again) and the shake becomes amplitude x direction
    void step(const ShakeFrame &frame);
    // 0x5b8b0: while the knock +0x8b0 (already decayed this frame) is above 0 the shake gains direction x knock x 0.01
    void kick(const glm::vec3 &direction, float knock);
    glm::vec3 offset() const
    {
        return offset_;
    }

  private:
    bool timerRanOut() const;
    void rearm(float hertz);

    glm::vec3 offset_{0.f};   // +0x9cc
    float timer_ = 0.f;       // +0x9d8
};

// 0x5bdc5: the internal camera's up (tripod +0xca0) gains the shake's x and z, its look-at point (+0xcac) the y
struct ShakenView
{
    glm::vec3 up{0.f};
    glm::vec3 look{0.f};
};
ShakenView shaken(const ShakenView &view, const glm::vec3 &offset);

// one substep's world contacts as physics_substep 0xcffb0 records them for a craft (0x442e94 + craft x 0x454,
// 0x2c-byte entries): the momentum the responses changed (+0x0, summed), the contacts' normal . the velocity's
// direction (+0x24, averaged by 0xd0a30 at the substep's end)
class SubstepContacts
{
  public:
    void add(const glm::vec3 &momentumChange, float normalDot);
    bool any() const
    {
        return count_ > 0;
    }
    glm::vec3 momentumChange() const
    {
        return momentumChange_;
    }
    float averageNormalDot() const;

  private:
    glm::vec3 momentumChange_{0.f};
    float normalDotSum_ = 0.f;
    int count_ = 0;
};

// +0x8b4, the internal camera's contact jolt (0x5ba23..0x5bc5a)
class ContactJolt
{
  public:
    // once a frame: the jolt x 0.95 (0x1a3620; per frame, not per second) and the frame's records start over (0xd09e0)
    void beginFrame();
    // a substep that had contacts takes the next record; one whose normal . direction is above 0.25 (0x1a3518) adds
    // 0.5 (0x1a3504) x its momentum change. Only 24 records count: 0xd0a30 stops advancing the index at 24.
    void addSubstep(const SubstepContacts &contacts);
    glm::vec3 vector() const
    {
        return jolt_;
    }

  private:
    glm::vec3 jolt_{0.f};
    int records_ = 0;
};

// the craft's axes: right +0x268, up +0x25c, forward +0x250
struct CraftAxes
{
    glm::vec3 right{1.f, 0.f, 0.f};
    glm::vec3 up{0.f, 1.f, 0.f};
    glm::vec3 forward{0.f, 0.f, 1.f};
};
// 0x5bc5c..0x5bdbf, before the shake: the up (+0xca0) gains right x clamp((jolt . right) x -0.003, -0.1, 0.1), the
// look-at point (+0xcac) up x clamp((jolt . forward) x 0.01, -0.1, 0.1)
ShakenView jolted(const ShakenView &view, const glm::vec3 &jolt, const CraftAxes &axes);

}  // namespace argon::shake
