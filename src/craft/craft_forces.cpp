// Argon: the forces on a craft each step (thrust, hover, steering, friction, aero, water, brakes) (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
#include "craft/craftsim.hpp"
#include "files/game_path.hpp"
#include "logic/blast_push.hpp"
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include "craft/row_major.hpp"
#include "craft/craft_controllers.hpp"

namespace argon
{

static const float kSlideGrip = 0.1f;   // 0x1a3508, the Slide egg

// ------------------------------------------------------------------ forces
void GameCraft::thrust()
{   // 0x5e370
    State &state = currentState();
    if (gain_ < 0.1f)
    {
        gain_ = 0.1f;
    }
    if (gear_ == -1 || !(onGround_ || inWater_))
    {
        return;
    }
    float inv = 1.f / std::max(1e-3f, stats_->gear(0, gear_, pilot_));   // the game reads gear set 1 here
    float thrust = inv * power_;
    thrust -= (ctrl_[8] + ctrl_[7]) * thrust * 0.005f;
    float revGap = revs_ - revAvg_;
    if (gain_ < revGap)
    {
        thrust = thrust * gain_ / revGap;
    }
    float back = 0.f;
    if (fwdSpeed_ > 0.f)
    {
        back = -(inv * st("engine_falloff"));
    }
    afflictThrottle();
    float thr = ctrl_[0];
    float push = (thrust * thr + (100.f - thr) * back) * 0.01f;
    state.force += push * fwd_;
    state.torque -= push * right_ * st("engine_noseup");
}

// freeze or engine off: no throttle; engine jitter: it loses (rand & 15) / 15 of the level each frame
void GameCraft::afflictThrottle()
{
    if (weaponEffects_.cutsEngine())
    {
        ctrl_[0] = 0.f;
    }
    if (weaponEffects_.jittersEngine())
    {
        ctrl_[0] = weapons::jitter(ctrl_[0], weaponEffects_.engineJitter, nextRand());
    }
}

void GameCraft::hover(const SimWorld &world)
{   // 0x5e6d0
    State &state = currentState();
    float ride = st("ride_height");
    float gravityAcceleration = st("mass_normalgravity"), rebound = st("rebound");
    int count = 0;
    HoverHits hits;
    uint32_t colourSums[3] = {0, 0, 0};
    anyContact_ = false;
    depthSum_ = 0;
    for (int k = 0; k < 4; ++k)
    {
        contact_[k] = false;
        depth_[k] = -0.2f;
        glm::vec3 corner = shape_[current_][k];
        glm::vec3 top = corner + up_ * (1.5f - ride), bottom = corner + up_ * (-1.5f - ride);
        glm::vec3 rs = (state.position - top) * 0.5f + top;
        glm::vec3 re = (state.position - bottom) * 0.05f + bottom;
        glm::vec3 hit, N;
        TriangleSurface surf;
        if (!world.segment(rs, re, hit, N, surf))
        {
            float dep;
            if (world.waterDepth(re, dep) && dep < 0.f)
            {
                contact_[k] = true;
            }   // over water: counts, no spring
            depth_[k] = 0;
            continue;
        }
        resetRequested_ |= surf.resetsCraft;   // 0x5eb81, whatever the slope
        if (glm::dot(N, up_) < 0)
        {
            N = -N;       // face towards the craft
        }
        glm::vec3 normal = -N;                        // the game's normal points into the surface
        if (std::abs(normal.y) < 0.2f)
        {
            continue;      // steep walls give no hover
        }
        // torque that tilts the hover plane onto the surface
        state.torque += glm::vec3(up_.z * normal.y - up_.y * normal.z, up_.x * normal.z - up_.z * normal.x, up_.y * normal.x - up_.x * normal.y) * 5.f;
        float depth = -glm::length(re - hit);
        depth = std::clamp(depth, -2.f, 0.f);
        depth_[k] = depth;
        depthSum_ -= depth;
        float mag = gravityAcceleration * mass_ * depth;
        glm::vec3 springForce = normal * mag;
        glm::vec3 arm = corner - state.position;
        glm::vec3 tq = glm::cross(arm, normal) * depth * 5.f;      // decompiled terms are -(r x -n)
        state.torque += tq;
        float vf = glm::dot(state.velocity, springForce);
        float damp = 1.f;
        if (vf >= 0.f)
        {
            damp = vf <= 5.f ? vf * 0.2f * 0.5f + (1.f - vf * 0.2f) : rebound;
        }
        state.force += springForce * damp;
        anyContact_ = true;
        contact_[k] = true;
        hits.point[k] = hit;
        hits.surface[k] = surf.type;
        hits.colour[k] = surf.colour;
        ++count;
        for (int channel = 0; channel < 3; ++channel)
        {
            colourSums[channel] += (surf.colour >> (16 - 8 * channel)) & 0xffu;
        }
    }
    averageGroundColour(colourSums, count);
    raiseDust(hits);
    onGround_ = count > 2;
    if (onGround_)
    {   // three agreeing contacts decide the surface (+0x858 -> +0x85c)
        for (int a = 0; a < 4; ++a)
        {
            int same = 0;
            for (int b = 0; b < 4; ++b)
            {
                same += hits.surface[b] == hits.surface[a] && hits.surface[a] >= 0;
            }
            if (same >= 3)
            {
                groundSurf_ = hits.surface[a];
            }
        }
        surface_ = groundSurf_;
    }
}

// 0x5f377..0x5f667: each rear ray on rough or loose ground, one rand() & 0xf == 0 -> dust at its hit point. Both take
// the colour of ray 3's vertex (0x5f40d / 0x5f584 read it, only ray 3 writes it, 0x5f24e); when ray 3 held nothing
// that slot is stale stack, so ray 2 uses its own colour here.
void GameCraft::raiseDust(const HoverHits &hits)
{
    for (int ray : {kFirstDustRay, kSecondDustRay})
    {
        if (!raisesDust(hits.surface[ray]) || !dustRollWins(nextRand()))
        {
            continue;
        }
        DustPuff puff;
        puff.at = hits.point[ray];
        puff.forward = fwd_;
        puff.groundColour = hits.surface[kSecondDustRay] >= 0 ? hits.colour[kSecondDustRay] : hits.colour[ray];
        puff.fromRay3 = ray == kSecondDustRay;
        dust_.push_back(puff);
    }
}

// sums of red, green, blue over the rays that held the craft; with none the last colour stays
void GameCraft::averageGroundColour(const uint32_t sums[3], int count)
{
    if (count <= 0)
    {
        return;
    }
    groundColour_ = 0xff000000u;
    for (int channel = 0; channel < 3; ++channel)
    {
        groundColour_ |= std::min(sums[channel] / uint32_t(count), 0xffu) << (16 - 8 * channel);
    }
}

void GameCraft::gravity()
{   // 0x5e600
    int contacts = contact_[0] + contact_[1] + contact_[2] + contact_[3];
    float gravityAcceleration = (contacts * st("mass_normalgravity") + (4 - contacts) * st("mass_flightgravity")) * 0.25f;
    currentState().force.y -= gravityAcceleration * mass_;
}

void GameCraft::diveClimb()
{   // 0x5fe50 (airborne only)
    if (onGround_ || inWater_)
    {
        return;
    }
    State &state = currentState();
    if (ctrl_[3] != 0)
    {
        state.torque -= right_ * st0("climb") * ctrl_[3] * 0.01f;
    }
    if (ctrl_[2] != 0)
    {
        state.torque += right_ * st0("dive") * ctrl_[2] * 0.01f;
    }
}

void GameCraft::steering()
{   // 0x6fb20 (AI) -> 0x5f9d0
    State &state = currentState();
    float stiff = st0("turn_stiffening");
    float vlim = sp("absolute_max_speed") * 0.138888896f;
    float slowShare = std::clamp((vlim - speed_) / vlim, 0.f, 1.f);
    float fac = (1.f - stiff) + slowShare * slowShare * stiff;
    afflictSteering();
    glm::vec3 t(0);
    bool ground = onGround_ || inWater_;
    if (steerB_ > 0)
    {
        if (ground)
        {
            t -= up_ * (steerB_ * 0.01f * st0("turn_max") * steerB_);
        }
        else
        {
            float yawShare = steerB_ * 0.01f;
            t -= up_ * (yawShare * st0("turn_flyingyaw") * steerB_) + fwd_ * (yawShare * st0("turn_flyingroll") * steerB_);
        }
    }
    if (steerA_ > 0)
    {
        if (ground)
        {
            t += up_ * (steerA_ * 0.01f * st0("turn_max") * steerA_);
        }
        else
        {
            float yawShare = steerA_ * 0.01f;
            t += up_ * (yawShare * st0("turn_flyingyaw") * steerA_) + fwd_ * (yawShare * st("turn_flyingroll") * steerA_);
        }
    }
    state.torque += t * fac;
    // AI assist: yaw torque straight from the steering error
    static const float same[5] = {0, 40, 100, 200, 200}, opp[5] = {0, 200, 500, 1000, 1000};
    float ks = same[speedClass_], ko = opp[speedClass_];
    if (!onGround_)
    {
        ks *= 0.4f;
        ko *= 0.4f;
    }
    glm::vec3 u = col(state.rotation, 1) * ctrl_[12];
    state.torque += u * (glm::dot(state.angularMomentum, u) < 0.f ? ko : ks);
}

// 0x5ffd0 (player craft, vtable 0x54): k = friction_<surface>_side_factor. The airbrake level +0xa18 jumps up to
// max(+0x978, +0x97c) and decays 150 / s; grip k' = (101 - level) / 101 x k (airbrakes loosen the grip).
// At forward speed >= 100 or k >= 2: F -= right x (v . right) x mass x k'; slower, k' blends towards 2:
// g = (100 - speed) / 100, F -= right x (v . right) x mass x (g x k' + 2 (1 - g)).
// The Slide egg (0x600c4, 0x52c8bd) replaces k' by 0.1 (0x1a3508); the blend below speed 100 still applies.
void GameCraft::playerSideFriction(float substep)
{
    State &state = currentState();
    static const char *keys[5] = {"friction_track_side_factor",
                                  "friction_rough_side_factor",
                                  "friction_loose_side_factor",
                                  "friction_water_side_factor",
                                  "friction_slippery_side_factor"};
    float surfaceSideFactor = st(keys[std::clamp(surface_, 0, 4)]);
    float airbrake = std::max(air_[0], air_[1]);
    if (airLevel_ < airbrake)
    {
        airLevel_ = airbrake;
    }
    if (airbrake < airLevel_)
    {
        airLevel_ -= substep * 150.f;
    }
    if (airLevel_ < 0.f)
    {
        airLevel_ = 0.f;
    }
    if (airLevel_ < airbrake)
    {
        airLevel_ = airbrake;
    }
    glm::vec3 right = right_;
    float vs = glm::dot(state.velocity, right);
    float grip = slide_ ? kSlideGrip : (101.f - airLevel_) * 0.00990099f * surfaceSideFactor;
    if (fwdSpeed_ < 100.f && surfaceSideFactor < 2.f)
    {
        float slowBlend = (100.f - fwdSpeed_) * 0.01f;
        grip = slowBlend * grip + (1.f - slowBlend) * 2.f;
    }
    state.force += right * (-vs) * mass_ * grip;
}

void GameCraft::sideFriction()
{   // 0x6f720
    State &state = currentState();
    static const char *keys[5] = {"friction_AI_track_side_factor",
                                  "friction_AI_rough_side_factor",
                                  "friction_AI_loose_side_factor",
                                  "friction_AI_water_side_factor",
                                  "friction_AI_slippery_side_factor"};
    float surfaceSideFactor = st(keys[std::clamp(surface_, 0, 4)]);
    glm::vec3 right = col(state.rotation, 0);
    float vs = glm::dot(state.velocity, right);
    float grip = surfaceSideFactor;
    if (fwdSpeed_ < 100.f && surfaceSideFactor < 2.f)
    {
        float slowShare = (100.f - fwdSpeed_) * 0.01f, fastShare = 1.f - slowShare;
        grip = slowShare * surfaceSideFactor + fastShare + fastShare;
    }
    state.force += right * (-vs) * mass_ * grip;
}

void GameCraft::aero()
{   // 0x60760
    State &state = currentState();
    glm::vec3 v = state.velocity;
    state.force -= up_ * fwdSpeed_ * st0("aerodynamics_downforce");
    if (!onGround_ && !inWater_)
    {
        float speedShare = std::clamp(fwdSpeed_ * 0.01f, 0.f, 1.f);
        float dart = st0("aerodynamics_dartiness"), fc = st0("aerodynamics_flightcontrol");
        float vu = glm::dot(v, up_);
        state.torque -= right_ * vu * dart;
        state.force -= up_ * vu * speedShare * fc;
        float vr = glm::dot(v, right_);
        state.torque += up_ * vr * dart;
        state.force -= right_ * vr * speedShare * fc;
        float rl = std::clamp(right_.y, -0.5f, 0.5f);
        state.torque -= fwd_ * rl * st0("aerodynamics_autolevelling");
        float cod = st("friction_air_cod");
        for (int i = 0; i < 3; ++i)
        {
            state.force[i] -= (v[i] > 0 ? 1.f : -1.f) * v[i] * v[i] * cod * 0.01f;
        }
        return;
    }
    static const char *cod[5] = {"friction_track_cod", "friction_rough_cod", "friction_loose_cod", "friction_water_cod", "friction_slippery_cod"};
    static const char *ramp[5] = {
        "friction_track_codramp", "friction_rough_codramp", "friction_loose_codramp", "friction_water_codramp", "friction_slippery_codramp"};
    int sf = std::clamp(surface_, 0, 4);
    if (fwdSpeed_ < 0.f)
    {
        state.force -= v * st(ramp[sf]);
    }
    float coefficient = st(cod[sf]);
    for (int i = 0; i < 3; ++i)
    {
        state.force[i] -= (v[i] > 0 ? 1.f : -1.f) * v[i] * v[i] * coefficient * 0.01f;
    }
}

void GameCraft::angularDamping()
{   // 0x5f800
    State &state = currentState();
    float damping = anyContact_ ? st("dampening_ground") : st0("dampening_air");
    glm::vec3 sum(0);
    for (int i = 0; i < 3; ++i)
    {
        glm::vec3 axis = col(state.rotation, i);
        sum += glm::cross(glm::cross(axis, state.angularVelocity), axis) * damping;
    }
    state.torque -= sum;
}

void GameCraft::noseDrop(float substep)
{   // 0x60e40
    if (anyContact_ || inWater_)
    {
        noseTimer_ = 0;
        return;
    }
    noseTimer_ += substep;
    float delay = st("nosedrop_delay"), amount = st("nosedrop_amount");
    if (noseTimer_ < delay)
    {
        return;
    }
    float torqueShare = noseTimer_ >= delay + 1.f ? amount : (noseTimer_ - delay) * amount;
    currentState().torque += torqueShare * glm::cross(fwd_, glm::vec3(0, -1, 0));
}

void GameCraft::water(const SimWorld &world)
{   // 0x61340
    State &state = currentState();
    inWater_ = false;
    wakeCorners_ = 0;
    int submerged = 0;
    float buoy = st("water_buoyancy");
    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 corner = shape_[current_][i];
        float depth;
        if (!world.waterDepth(corner, depth) || depth >= 0.f)
        {
            continue;
        }
        ++submerged;
        if (i == 2 || i == 3)
        {
            ++wakeCorners_;
        }
        depth = std::clamp(depth, -2.f, 0.f);
        inWater_ = true;
        float vy = state.velocity.y;
        noteSplash(vy);
        float lift = vy <= 0.f ? (vy - buoy) * depth : -(depth * buoy);
        lift = std::clamp(lift - 2.f * vy, -10000.f, 10000.f);
        state.force.y += lift;
        glm::vec3 arm = corner - state.position;
        state.torque += glm::vec3(arm.z, 0.f, -arm.x) * depth * 5.f * 0.5f;
        float fy = state.rotation[5];
        if (ctrl_[0] > 30.f && fy < 0.5f && fy > -0.5f)
        {
            state.torque -= right_ * st("water_noseup");
        }
    }
    if (submerged > 3)
    {
        surface_ = 3;
    }
}

// 0x71fe0 (player controller): the brake value +0x980 ramps up by dt x brake_gain x 60 while the brake (+0x918 =
// control slot 9) is held, to 100; released -> 0. (Held at 100 below 5 m/s it also runs a timer +0x984 and the
// reverse flag +0x988 / strength +0x98c exist, but nothing in the XBE reads the timer or sets the flag: no reverse.)
void GameCraft::brakeRamp(float dt)
{
    if (ctrl_[9] == 0.f)
    {
        brake_ = 0.f;
        return;
    }
    if (brake_ < 100.f)
    {
        brake_ += dt * st("brake_gain") * 60.f;
    }
    if (brake_ > 100.f)
    {
        brake_ = 100.f;
    }
}

// 0x60450: only with 3+ hover contacts. F -= normalize(v) x brake x brake_amount, each component no larger than
// v x 25; above 0.1 m/s also a nose-up torque 2 x right x brake x engine_noseup.
void GameCraft::brakes()
{
    State &state = currentState();
    if (brake_ <= 0.f || !onGround_)
    {
        return;
    }
    float speed = glm::length(state.velocity);
    if (speed == 0.f)
    {
        return;
    }
    glm::vec3 brakeForce = state.velocity / speed * brake_ * st("brake_amount");
    for (int i = 0; i < 3; ++i)
    {
        float lim = state.velocity[i] * 25.f;
        state.force[i] -= brakeForce[i] > 0.f ? std::min(brakeForce[i], lim) : std::max(brakeForce[i], lim);
    }
    if (fwdSpeed_ > 0.1f)
    {
        state.torque += 2.f * right_ * brake_ * st("engine_noseup");
    }
}

// 0x71bc0 (player): +0x978 / +0x97c follow slots [7] / [8] (+0x910 / +0x914), rising at dt x airbrake_force_gain
// x 60 and falling at dt x airbrake_force_falloff x 60
void GameCraft::airbrakeRamp(float dt)
{
    for (int k = 0; k < 2; ++k)
    {
        float target = ctrl_[7 + k];
        float &airbrake = air_[k];
        if (airbrake < target)
        {
            airbrake = std::min(target, airbrake + dt * st("airbrake_force_gain") * 60.f);
        }
        else
        {
            airbrake = std::max(target, airbrake - dt * st("airbrake_force_falloff") * 60.f);
        }
    }
}

// 0x60230: s = clamp(forward speed, 0, 50); per side F +-= right x (front + rear) x a x s x 0.01 and
// T +-= up x (front - rear) x a x s x 0.01 (+ for +0x978, - for +0x97c). Sideways, so no reverse.
void GameCraft::airbrakes()
{
    State &state = currentState();
    float sp = std::clamp(fwdSpeed_, 0.f, 50.f);
    float front = st("airbrake_force_front"), rear = st("airbrake_force_rear");
    for (int k = 0; k < 2; ++k)
    {
        if (air_[k] <= 0.f)
        {
            continue;
        }
        float sg = k == 0 ? 1.f : -1.f;
        state.force += sg * right_ * ((front + rear) * air_[k] * sp * 0.01f);
        state.torque += sg * up_ * ((front - rear) * air_[k] * sp * 0.01f);
    }
}

void GameCraft::forces(float substep, const SimWorld &world)
{   // 0x5d670 (AI vtable)
    State &state = currentState();
    state.force = state.torque = glm::vec3(0);
    auto clampFT = [&] {
        state.force = clampAbs(state.force, 5e6f);
        state.torque = clampAbs(state.torque, 5e6f);
    };   // 0x5d5e0
    takeBlastPush();
    clampFT();
    thrust();
    clampFT();
    hover(world);
    clampFT();
    gravity();
    clampFT();
    brakes();
    clampFT();   // 0x60450 (the AI zeroes +0x980 every frame, so only the player brakes)
    diveClimb();
    clampFT();
    steering();
    clampFT();
    controller_->sideFriction(*this, substep);
    clampFT();
    airbrakes();
    clampFT();   // 0x60230 (+0x978 / +0x97c stay 0 for AI craft)
    aero();
    clampFT();
    angularDamping();
    clampFT();
    noseDrop(substep);
    clampFT();
    controller_->turbo(*this, substep);
    water(world);
    clampFT();
}

} // namespace argon
