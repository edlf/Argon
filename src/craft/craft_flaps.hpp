// Argon: the ships' airbrake flaps. The game builds one ship class per pilot (0x58e60, a switch on the pilot
// id); slot 2 of its vtable (0x1a0254..) animates the parts every frame and turns each flap bone with 0xb99f0 (the
// bone's rotation = D3DXMatrixRotationYawPitchRoll, rows times the bone's row scale, translation kept).
// docs\physics\turbo-flaps-cameras.md "Airbrake flaps".
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "model/cmm.hpp"

namespace argon
{

class GameCraft;

// how one ship class moves its flaps; side 0 follows the control slots [5] / [7], side 1 [6] / [8]
struct FlapRule
{
    const char *bones[2];        // the flap bone per side (null: none)
    float demandPerPercent;      // the opening asked for per control percent
    bool steeringOpens;          // the steer slot adds to the airbrake slot
    bool largerSideOnly;         // the opening follows the larger of the two airbrake slots
    bool brakeOpens;             // while the brake (+0x980) is on, the opening grows 4 / s
    float openRate, closeRate;   // easing towards a larger / smaller demand, per second
    float elevatorGain;          // the climb - dive tilt added to the opening every frame
    float lowest, highest;       // the opening's range
    glm::vec3 turn[2];           // the game's angle vector (pitch, yaw, roll) per unit opening, per side
};
const FlapRule *flapRuleFor(int pilot);   // null: that ship class has no flaps

struct FlapControls
{
    float steer[2], airbrake[2];   // control slots [5] / [6], [7] / [8] (0..100)
    float climbMinusDive;          // [3] - [2]
    bool braking;                  // +0x980 > 0
    bool hovering;                 // +0x92d: a hover ray touches
};
FlapControls flapControls(const GameCraft &craft);

// one ship's flap openings
class FlapState
{
  public:
    void update(const FlapRule &rule, const FlapControls &in, float dt);
    glm::vec3 turn(const FlapRule &rule, int side) const
    {
        return rule.turn[side] * opening_[side];
    }

  private:
    void updateElevator(const FlapControls &in, float dt);
    void easeSide(const FlapRule &rule, const FlapControls &in, int side, float dt);
    float opening_[2]{};
    float elevator_ = 0;   // Yuri / Abi: the climb - dive tilt (+0x1b0 / +0x1a0)
};

// one ship model's flap bones
class FlapRig
{
  public:
    FlapRig() = default;
    FlapRig(const std::string &folder, const Cmm &model);
    const FlapRule *rule() const
    {
        return rule_;
    }
    int sideOf(int bone) const;                             // the flap the bone is on or under, -1 none
    glm::mat4 offset(int side, const glm::vec3 &turn) const;   // model space: the rest pose -> the flap turned

  private:
    struct Flap
    {
        glm::mat4 parentWorld{1}, restWorldInverse{1};
        glm::vec3 position{0}, rowScale{1};
    };
    void addFlap(const Cmm &model, int side, int bone);
    const FlapRule *rule_ = nullptr;
    std::array<Flap, 2> flaps_{};
    std::vector<int8_t> sideOfBone_;
};

} // namespace argon
