#include "craft/craft_flaps.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "craft/craftsim.hpp"

namespace argon
{

namespace
{

const glm::vec3 kPitchDown{-1, 0, 0}, kPitchUp{1, 0, 0}, kYawLeft{0, -1, 0}, kYawRight{0, 1, 0};

// 0x504a0, shared by ten classes (pushed with 0.005, 4 and "no sign flip"): demand = (airbrake + steer) x 0.005,
// eased at 4 / s (8 / s while opening), 0..1, turned to pitch -opening
const FlapRule kSharedFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.005f, true, false, true, 8.f, 4.f, 0.f, 0.f, 1.f, {kPitchDown, kPitchDown}};
// 0x563b0 (Zafir): the same with AIRBRAKE_LEFT1 as the second flap
const FlapRule kZafirFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_LEFT1"}, 0.005f, true, false, true, 8.f, 4.f, 0.f, 0.f, 1.f, {kPitchDown, kPitchDown}};
// 0x576c0 (Vladimir): x 0.003, the flaps swing out about yaw (left -opening, right +opening)
const FlapRule kVladimirFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.003f, true, false, true, 8.f, 4.f, 0.f, 0.f, 1.f, {kYawLeft, kYawRight}};
// 0x57990 (Yuri): the airbrake slot alone x 0.009, plus the climb - dive tilt x 0.001 each frame, -0.9..0.9, pitch +opening
const FlapRule kYuriFlaps{{"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.009f, false, false, true, 8.f, 4.f, 0.001f, -0.9f, 0.9f, {kPitchUp, kPitchUp}};
// 0x51d40 (Abi): as Yuri with the tilt x 0.0003, pitch +0.4 x opening
const FlapRule kAbiFlaps{
    {"AIRBRAKE_LEFT", "AIRBRAKE_RIGHT"}, 0.009f, false, false, true, 8.f, 4.f, 0.0003f, -0.9f, 0.9f, {glm::vec3(0.4f, 0, 0), glm::vec3(0.4f, 0, 0)}};
// 0x54b90 (Kumiko): one flap following the larger airbrake slot x 0.01 at 5 / s, no brake, 0..1, pitch -opening
const FlapRule kKumikoFlaps{{"AIRBRAKE_MAIN", nullptr}, 0.01f, false, true, false, 5.f, 5.f, 0.f, 0.f, 1.f, {kPitchDown, kPitchDown}};

// the ship class per pilot id (0x58e60); Kukai (0x544f0) and Kutoo (0x54c80) move no flaps
const FlapRule *const kRuleByPilot[16] = {&kSharedFlaps,
                                          &kYuriFlaps,
                                          &kKumikoFlaps,
                                          &kSharedFlaps,
                                          &kSharedFlaps,
                                          &kSharedFlaps,
                                          &kSharedFlaps,
                                          nullptr,
                                          &kSharedFlaps,
                                          &kVladimirFlaps,
                                          &kAbiFlaps,
                                          &kZafirFlaps,
                                          &kSharedFlaps,
                                          &kSharedFlaps,
                                          &kSharedFlaps,
                                          nullptr};

float demandOf(const FlapRule &rule, const FlapControls &in, int side)
{
    if (rule.largerSideOnly)
    {
        return std::max(in.airbrake[0], in.airbrake[1]) * rule.demandPerPercent;
    }
    const float steer = rule.steeringOpens ? in.steer[side] : 0.f;
    return (in.airbrake[side] + steer) * rule.demandPerPercent;
}

// 0xb99f0 -> 0xf3117 D3DXMatrixRotationYawPitchRoll(yaw = y, pitch = x, roll = z); as column vectors Ry x Rx x Rz
glm::mat4 yawPitchRoll(const glm::vec3 &turn)
{
    const glm::mat4 yaw = glm::rotate(glm::mat4(1), turn.y, glm::vec3(0, 1, 0));
    const glm::mat4 pitch = glm::rotate(glm::mat4(1), turn.x, glm::vec3(1, 0, 0));
    const glm::mat4 roll = glm::rotate(glm::mat4(1), turn.z, glm::vec3(0, 0, 1));
    return yaw * pitch * roll;
}

int pilotOfFolder(const std::string &folder)
{
    for (int id = 0; id < 16; ++id)
    {
        if (folder == pilotFolder(id))
        {
            return id;
        }
    }
    return -1;
}

} // namespace

const FlapRule *flapRuleFor(int pilot)
{
    return pilot >= 0 && pilot < 16 ? kRuleByPilot[pilot] : nullptr;
}

FlapControls flapControls(const GameCraft &craft)
{
    FlapControls in{};
    for (int side = 0; side < 2; ++side)
    {
        in.steer[side] = craft.controlSlot(5 + side);
        in.airbrake[side] = craft.controlSlot(7 + side);
    }
    in.climbMinusDive = craft.controlSlot(3) - craft.controlSlot(2);
    in.braking = craft.brakeLevel() > 0.f;
    in.hovering = craft.hoverTouching();
    return in;
}

void FlapState::update(const FlapRule &rule, const FlapControls &in, float dt)
{
    if (rule.elevatorGain != 0.f)
    {
        updateElevator(in, dt);
    }
    for (int side = 0; side < 2; ++side)
    {
        if (rule.bones[side])
        {
            easeSide(rule, in, side, dt);
        }
    }
}

// Yuri / Abi: on the hover it fades at 3 / s, in the air it follows climb - dive at 3 / s
void FlapState::updateElevator(const FlapControls &in, float dt)
{
    if (in.hovering)
    {
        elevator_ *= 1.f - 3.f * dt;
    }
    else
    {
        elevator_ += (in.climbMinusDive - elevator_) * dt * 3.f;
    }
}

void FlapState::easeSide(const FlapRule &rule, const FlapControls &in, int side, float dt)
{
    float &opening = opening_[side];
    if (rule.brakeOpens && in.braking)
    {
        opening += 4.f * dt;
    }
    else
    {
        const float demand = demandOf(rule, in, side);
        opening -= (opening - demand) * dt * (demand > opening ? rule.openRate : rule.closeRate);
    }
    opening += elevator_ * rule.elevatorGain;
    opening = std::clamp(opening, rule.lowest, rule.highest);
}

FlapRig::FlapRig(const std::string &folder, const Cmm &model) : rule_(flapRuleFor(pilotOfFolder(folder)))
{
    sideOfBone_.assign(model.bones.size(), int8_t(-1));
    if (!rule_)
    {
        return;
    }
    for (int side = 0; side < 2; ++side)
    {
        if (!rule_->bones[side])
        {
            continue;
        }
        for (size_t b = 0; b < model.bones.size(); ++b)
        {
            if (model.bones[b].name == rule_->bones[side])
            {
                addFlap(model, side, int(b));
            }
        }
    }
}

void FlapRig::addFlap(const Cmm &model, int side, int bone)
{
    const CmmBone &record = model.bones[size_t(bone)];
    Flap &flap = flaps_[size_t(side)];
    glm::mat4 world(1.f);
    model.boneWorld(bone, glm::value_ptr(world));
    const glm::mat4 local = glm::make_mat4(record.local);   // the D3D row-vector floats are the column-major matrix
    flap.parentWorld = world * glm::inverse(local);
    flap.restWorldInverse = glm::inverse(world);
    flap.position = glm::vec3(local[3]);
    flap.rowScale = glm::vec3(record.scale[0], record.scale[1], record.scale[2]);
    for (size_t b = 0; b < model.bones.size(); ++b)
    {
        for (int a = int(b), depth = 0; a >= 0 && depth < 64; a = model.bones[size_t(a)].parent, ++depth)
        {
            if (a == bone)
            {
                sideOfBone_[b] = int8_t(side);
                break;
            }
        }
    }
}

int FlapRig::sideOf(int bone) const
{
    return bone >= 0 && bone < int(sideOfBone_.size()) ? sideOfBone_[size_t(bone)] : -1;
}

// the turned local: its rows (D3D) = the rotation's rows x the row scale, the translation kept (0xb99f0)
glm::mat4 FlapRig::offset(int side, const glm::vec3 &turn) const
{
    const Flap &flap = flaps_[size_t(side)];
    glm::mat4 local = yawPitchRoll(turn);
    for (int row = 0; row < 3; ++row)
    {
        local[row] *= flap.rowScale[row];
    }
    local[3] = glm::vec4(flap.position, 1.f);
    return flap.parentWorld * local * flap.restWorldInverse;
}

} // namespace argon
