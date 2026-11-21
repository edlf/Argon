// Argon: per frame: rumble, blast pushes, revs and gears, steering smoothing, turbo, the speed cap (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
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

// ------------------------------------------------------------------ per frame
// 0x5b610 (docs\physics\rumble.md "Rumble"): ground contact g (+0x9c8) rises 10 / s on the ground and falls 2 / s in the air
// (0..1); with speed s (+0x9a4) and forward speed f (+0x990): a = (2 (s - f) + s) x 0.005, amp = a^2 x
// rumble_<surface> x 0.2 x g, at most rumblelimit_<surface>. The first motor (+0x9ea) = amp x 300000 + the beam boost
// +0x9dc, over 65000: 65000 and the second gets 20000; each frame it drops to 0 on a coin toss (rand() > 0x3fff). The
// second (+0x9e8) adds the knock +0x8b0 x 1000 and, past 2.2 of hover compression (+0x890), (+0x890 - 1.5) x s x 25.
// The motor words wrap (fistp, low 16 bits). The same frame's random direction and amp drive the internal camera's
// shake, rumblehertz_<surface> pacing it, and the knock kicks it (logic\camera_shake.hpp).
void GameCraft::rumble(float dt)
{
    const glm::vec3 direction = shakeDirection();
    buzzBoost_ = rumble::decayed(buzzBoost_, 4.f, dt);
    const float amp = surfaceRumble(dt);
    shake_.step({dt, amp, surfaceHertz(), direction});
    if (knock_ > 0.f)
    {
        knock_ = rumble::decayed(knock_, 2.f, dt);
        shake_.kick(direction, knock_);
    }
    float first = amp * 300000.f + buzzBoost_, second = 0.f;
    if (first > 65000.f)
    {
        first = 65000.f;
        second = 20000.f;
    }
    second += rumble::knockMotor(knock_);
    if (nextRand() > 0x3fff)
    {
        first = 0.f;
    }
    const uint16_t firstWord = rumble::motorWord(first);
    const uint16_t secondWord = rumble::withCompression(rumble::motorWord(second), depthSum_, speed_);
    decayHitRumble(dt);
    rumA_ = rumble::motorWord(float(firstWord) + hit38_);
    rumB_ = rumble::motorWord(float(secondWord) + hit34_);
}

float GameCraft::surfaceRumble(float dt)
{
    static const char *amp[5] = {"rumble_track", "rumble_rough", "rumble_loose", "rumble_water", "rumble_slippery"};
    static const char *lim[5] = {"rumblelimit_track", "rumblelimit_rough", "rumblelimit_loose", "rumblelimit_water", "rumblelimit_slippery"};
    const int sf = std::clamp(surface_, 0, 4);
    ground_ = std::clamp(onGround_ ? ground_ + dt * 10.f : ground_ - 2.f * dt, 0.f, 1.f);
    const float amplitude = ((speed_ - fwdSpeed_) * 2.f + speed_) * 0.005f;
    return std::min(amplitude * st(amp[sf]) * amplitude * 0.2f * ground_, st(lim[sf]));
}

// 0x5b6ae / 0x5b6d0 / 0x5b6f2: three rand() calls, x / y / z
glm::vec3 GameCraft::shakeDirection()
{
    const float x = shake::randomAxis(nextRand());
    const float y = shake::randomAxis(nextRand());
    return {x, y, shake::randomAxis(nextRand())};
}

// rumblehertz_<surface> (stats +0x2b4 + surface x 4)
float GameCraft::surfaceHertz() const
{
    static const char *hertz[5] = {"rumblehertz_track", "rumblehertz_rough", "rumblehertz_loose", "rumblehertz_water", "rumblehertz_slippery"};
    return st(hertz[std::clamp(surface_, 0, 4)]);
}

// craft_update 0x71490: +0x1138 decays x (1 - 10 dt), +0x1134 x (1 - 5 dt) (dt at most 0.1), both gain the frame's
// contact impulses x 1000 (+0x1134 x the contact's factor: 1 here), 0..65535
void GameCraft::decayHitRumble(float dt)
{
    const float clampedDt = std::clamp(dt, 0.f, 0.1f);
    hit38_ = rumble::decayed(hit38_, 10.f, clampedDt);
    hit34_ = rumble::decayed(hit34_, 5.f, clampedDt);
    hit34_ = std::clamp(hit34_ + hitNow_ * 1000.f, 0.f, 65535.f);
    hit38_ = std::clamp(hit38_ + hitNow_ * 1000.f, 0.f, 65535.f);
    hitNow_ = 0.f;
}

// 0x5dee0 (the physics step's first piece, after the force is cleared): an explosion's push, its y made positive,
// becomes the knock +0x8b0 = its length, brakes the craft (both state blocks' momentum +0x74 / +0xec lose a fraction
// of the velocity) and shoves it sideways for this substep (logic\blast_push.hpp). With a shield up it was dropped at
// the blast (Weapons::pushCraftsFromBlast).
void GameCraft::takeBlastPush()
{
    if (blastPush_ == glm::vec3(0.f))
    {
        return;
    }
    const glm::vec3 push = blast::upward(blastPush_);
    knock_ = glm::length(push);
    turboCooldown_.start();
    brakeByBlast(push);
    currentState().force += blast::sidewaysForce(knock_, right_, controller_->blastSideScale(), nextRand());
    blastPush_ = glm::vec3(0.f);
}

void GameCraft::brakeByBlast(const glm::vec3 &push)
{
    const glm::vec3 velocity = currentState().velocity;
    float fraction = blast::brakingFraction(push, velocity);
    if (blastBrakingDoubled_)
    {
        fraction = blast::doubledBraking(fraction);
    }
    states_[0].momentum -= velocity * fraction;
    states_[1].momentum -= velocity * fraction;
}

// control jitter: each steering input loses (rand & 15) / 15 of the level each frame
void GameCraft::afflictSteering()
{
    if (!weaponEffects_.jittersControls())
    {
        return;
    }
    steerB_ = weapons::jitter(steerB_, weaponEffects_.controlJitter, nextRand());
    steerA_ = weapons::jitter(steerA_, weaponEffects_.controlJitter, nextRand());
}

unsigned GameCraft::nextRand()
{
    randState_ = randState_ * 214013u + 2531011u;
    return (randState_ >> 16) & 0x7fffu;
}

void GameCraft::revsGears(float dt)
{   // 0x6ed90 (reads record 0 for the AI)
    State &state = currentState();
    float sum = 0;
    for (int i = 0; i < 9; ++i)
    {
        revHist_[i] = revHist_[i + 1];
        sum += revHist_[i];
    }
    revHist_[9] = revs_;
    revAvg_ = (sum + revHist_[9]) * 0.111111112f;
    fwdSpeed_ = glm::dot(state.velocity, col(state.rotation, 2));
    speed_ = glm::length(state.velocity);
    if (gear_ < 0)
    {
        revs_ *= 0.99f;
    }
    else
    {
        revs_ = fwdSpeed_ / std::max(1e-3f, stats_->gear(speedClass_, gear_, 0));
    }
    float maxr = st0("engine_maxrevs");
    int ng = int(st0("gears_numgears"));
    if (maxr < revs_ && gear_ < ng - 1 && gear_ != -1)
    {
        gearNext_ = gear_ + 1;
        gear_ = -1;
        shiftTimer_ = st0("gears_neutraltime");
    }
    if (gear_ > 0 && fwdSpeed_ / std::max(1e-3f, stats_->gear(speedClass_, gear_ - 1, 0)) < maxr * 0.85f)
    {
        gearNext_ = gear_ - 1;
        gear_ = -1;
        shiftTimer_ = st0("gears_neutraltime");
    }
    if (gear_ == -1)
    {
        shiftTimer_ -= dt;
        if (shiftTimer_ <= 0)
        {
            gear_ = gearNext_;
        }
    }
}

namespace
{
struct SteerRates
{
    float gain, falloff, dt;   // turn_gain, turn_falloff (record 0's), the step
};

// a smoothed steering slot towards its control: up by gain x target, down by falloff x (100 - target), never past it
float steeredToward(float value, float target, const SteerRates &rates)
{
    if (value < target)
    {
        value = std::min(value + rates.gain * target * 0.6f * rates.dt, target);
    }
    if (value > target)
    {
        value = std::max(value - (100.f - target) * rates.falloff * rates.dt * 0.6f, target);
    }
    return value;
}
}  // namespace

void GameCraft::steerSmooth(float dt)
{   // 0x6f0b0 with record 0's turn_gain / turn_falloff
    const SteerRates rates{st0("turn_gain"), st0("turn_falloff"), dt};
    steerB_ = steeredToward(steerB_, ctrl_[6], rates);
    steerA_ = steeredToward(steerA_, ctrl_[5], rates);
}

// player_pad_input 0x70ac0: pad A (bit 0x10) sets +0x91f only after it has been let go once (+0x920); the turbo
// button +0x9f0 is on while held, the race runs (0x52af9e clear), turbo is left and the craft has been airborne for
// under 0.5 s (+0x938)
void GameCraft::turboInput()
{
    if (pad_.turbo)
    {
        if (turboReleased_)
        {
            turboHeld_ = true;
        }
    }
    else
    {
        turboHeld_ = false;
        turboReleased_ = true;
    }
    turboBtn_ = turboHeld_ && turboAllowed_ && turboSecs_ > 0.f && airTime_ < 0.5f;
}

// craft_turbo 0x5e270 (in the force list): with the button on, turbo left and under 0.1 s airborne: +0x9f3 / +0x9f1
// set, the seconds drain by the step (not with Infinite Turbo), clamped 0..100, and F += turbo_force[speed] x forward
void GameCraft::turbo(float substep)
{
    if (!turboFires())
    {
        return;
    }
    if (!turboInfinite_)
    {
        drainTurbo(substep);
    }
    pushTurbo();
}

// the drone's 0x6f9f0: the cooldown +0x9c4 counts down first and holds the turbo until it is over; Infinite Turbo
// (0x1bbbf1) is not checked, so the seconds always drain
void GameCraft::droneTurbo(float substep)
{
    turboCooldown_.countDown(substep);
    if (!turboCooldown_.over() || !turboFires())
    {
        return;
    }
    drainTurbo(substep);
    pushTurbo();
}

bool GameCraft::turboFires() const
{
    return turboBtn_ && turboAllowed_ && turboSecs_ > 0.f && airTime_ < 0.1f;
}

void GameCraft::drainTurbo(float substep)
{
    turboSecs_ = std::clamp(turboSecs_ - substep, 0.f, 100.f);
}

// +0x9f3 / +0x9f1 set, turbo_force<speed> along the forward axis
void GameCraft::pushTurbo()
{
    turboCap_ = true;
    turboOn_ = true;
    currentState().force += sp("turbo_force") * fwd_;
}

void GameCraft::speedCap()
{   // 0x6f900 / 0x720e0
    float speedRatio = fwdSpeed_ / sp("absolute_max_speed") * 3.6f;
    // after a turbo (+0x9f3) the cap is turbo_max_speed until the speed is back under absolute_max_speed
    if (turboCap_)
    {
        float rt = fwdSpeed_ / sp("turbo_max_speed") * 3.6f;
        if (speedRatio < 1.f)
        {
            turboCap_ = false;
        }
        speedRatio = rt;
    }
    if (speedRatio <= 1.f)
    {
        return;
    }
    State &state = currentState();
    state.momentum /= speedRatio;
    state.velocity /= speedRatio;
}

} // namespace argon
