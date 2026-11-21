// Port of the game's craft physics + AI pilot: setup, who flies it, the start and respawn, the frame. The rest
// is split by part (craft_forces / craft_motion / craft_engine / ai_pilot / ai_graph / craft_stats / sim_world /
// craft_tripod.cpp); addresses are default.xbe functions, see craftsim.hpp and docs\physics\README.md.
#include "craft/craftsim.hpp"
#include "craft/row_major.hpp"
#include "craft/craft_controllers.hpp"
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

namespace argon
{


const GameCraft::PlayerController GameCraft::PlayerController::shared;
const GameCraft::DroneController GameCraft::DroneController::shared;
const GameCraft::Controller &GameCraft::playerController = GameCraft::PlayerController::shared;
const GameCraft::Controller &GameCraft::droneController = GameCraft::DroneController::shared;

void GameCraft::flyByPad(const Pad &pad)
{
    controller_ = &playerController;
    pad_ = pad;
}

// the AI's 2 s progress check starts when the AI takes the craft over: one still running from the countdown on the
// grid made every drone "stuck" (and respawned) within 2 s of GO
void GameCraft::flyByAi()
{
    if (controller_ != &droneController)
    {
        restartProgressCheck();
    }
    controller_ = &droneController;
    pad_ = {};
}

void GameCraft::restartProgressCheck()
{
    checkTimer_ = 0;
    checkHistory_.fill(pos_);
}

bool GameCraft::flownByPad() const
{
    return controller_ == &playerController;
}

void GameCraft::dropPlayerBrakes()
{
    brake_ = 0.f;
    air_[0] = air_[1] = airLevel_ = 0.f;
}

void GameCraft::testTurboButton()
{
    turboBtn_ = turboTest_ && turboAllowed_ && turboSecs_ > 0.f && airTime_ < 0.5f;
}

float GameCraft::sp(const char *base) const
{
    char key[64];
    std::snprintf(key, sizeof key, "%s%d", base, speedClass_ + 1);
    return st(key);
}

const char *GameCraft::surfaceName() const
{
    static const char *names[] = {"Track", "Rough", "Loose", "Water", "Slippery", "Loop"};
    return names[std::clamp(surface_, 0, 5)];
}

// ------------------------------------------------------------------ setup (0x70340 / 0x6fdc0 / loader)
void GameCraft::init(const CraftStats &stats, int pilot, int speedClass, float aiPowerPct, const glm::mat4 &start, const AiGraph &graph, int startNode,
                     const SimWorld &world)
{
    *this = GameCraft{};
    stats_ = &stats;
    pilot_ = std::clamp(pilot, 0, 15);
    speedClass_ = std::clamp(speedClass, 0, 4);
    aiPower_ = aiPowerPct;
    // box 4 x 2 x 6, corner order as 0x70340 builds it (0-3 bottom, 2/3 = rear)
    const float hx = 2.f, hy = 1.f, hz = 3.f;
    box_ = {glm::vec3(hx, -hy, hz),
            glm::vec3(-hx, -hy, hz),
            glm::vec3(hx, -hy, -hz),
            glm::vec3(-hx, -hy, -hz),
            glm::vec3(hx, hy, hz),
            glm::vec3(-hx, hy, hz),
            glm::vec3(hx, hy, -hz),
            glm::vec3(-hx, hy, -hz)};
    // mass = mass_density * volume (48), inertia of the box (0x6fdc0)
    mass_ = st("mass_density") * 48.f;
    if (mass_ <= 0)
    {
        mass_ = 0.96f;
    }
    invMass_ = 1.f / mass_;
    float dx = 2 * hx, dy = 2 * hy, dz = 2 * hz;
    float Ixx = (dz * dz + dy * dy) * mass_ * 0.0833333358f, Iyy = mass_ * 4.33333349f, Izz = (dx * dx + dy * dy) * mass_ * 0.0833333358f;
    IbInv_ = glm::vec3(1.f / Ixx, 1.f / Iyy, 1.f / Izz);
    // start pose: locator matrix rows = right, up, forward, position (D3D row vectors)
    State &state = states_[0];
    glm::vec3 r(start[0]), u(start[1]), f(start[2]);
    state.position = glm::vec3(start[3]);
    for (int i = 0; i < 3; ++i)
    {
        state.rotation[i * 3 + 0] = r[i];
        state.rotation[i * 3 + 1] = u[i];
        state.rotation[i * 3 + 2] = f[i];
    }
    states_[1] = state;
    settleOnStart(world);
    cacheAxes();
    gain_ = sp("engine_gain");
    node_ = lastRestart_ = std::clamp(startNode, 0, std::max(0, int(graph.nodes.size()) - 1));
    look_.fill(node_);
    lastFork_ = ForkPassed{};
    routeList_.reset();
    checkHistory_.fill(states_[0].position);
}

glm::mat4 GameCraft::matrix() const
{
    const State &state = currentState();
    glm::vec3 right = col(state.rotation, 0), up = col(state.rotation, 1), forward = col(state.rotation, 2);
    return glm::mat4(glm::vec4(right, 0), glm::vec4(up, 0), glm::vec4(forward, 0), glm::vec4(state.position, 1));
}

void GameCraft::updateShape(int si)
{   // 0xd1620
    const State &state = states_[si];
    for (int i = 0; i < 8; ++i)
    {
        shape_[si][i] = state.position + mulv(state.rotation, box_[i]);
    }
}

void GameCraft::cacheAxes()
{   // 0x70430
    const State &state = currentState();
    right_ = col(state.rotation, 0);
    up_ = col(state.rotation, 1);
    fwd_ = col(state.rotation, 2);
    pos_ = state.position;
}

// the race setup's start placement (0x5a270): a segment from the locator 30 down (0xd4770, the track collision);
// on a hit the craft sits 2 above the hit point, else at the locator itself. So the grid starts on its hover pads,
// not in the air (the respawn's 7.5 above the ground is dropToGround's)
void GameCraft::settleOnStart(const SimWorld &world)
{
    constexpr float kProbeDepth = 30.f, kAboveGround = 2.f;
    State &state = states_[0];
    SegmentHit below;
    if (world.grid && world.grid->segment(state.position, state.position - glm::vec3(0, kProbeDepth, 0), below))
    {
        state.position.y -= below.along * kProbeDepth - kAboveGround;
    }
    states_[1] = states_[0];
    updateShape(0);
    updateShape(1);
}

void GameCraft::dropToGround(const SimWorld &world)
{
    for (State &state : states_)
    {
        GroundHit ground;
        if (world.grid && world.grid->rayDown(glm::vec3(state.position.x, state.position.y + 2.f, state.position.z), 22.f, ground))
        {
            float dropHeight = 7.5f;   // the game's drop height; kept under roofs/overhangs (viewer safeguard)
            const glm::vec3 g(state.position.x, ground.y + 0.1f, state.position.z);
            SegmentHit roof;
            if (world.grid->segment(g, g + glm::vec3(0, 10.f, 0), roof))
            {
                dropHeight = std::min(dropHeight, std::max(2.5f, roof.along * 10.f - 2.f));
            }
            state.position.y = ground.y + dropHeight;
        }
    }
    states_[1] = states_[0];
    updateShape(0);
    updateShape(1);
}

void GameCraft::placeFromReplay(const glm::mat4 &pose, const glm::vec3 &velocity)
{
    State &state = currentState();
    for (int i = 0; i < 3; ++i)
    {
        state.rotation[i * 3 + 0] = pose[0][i];
        state.rotation[i * 3 + 1] = pose[1][i];
        state.rotation[i * 3 + 2] = pose[2][i];
    }
    state.position = glm::vec3(pose[3]);
    state.velocity = velocity;
    state.momentum = velocity * mass_;
    nextState() = state;
    updateShape(0);
    updateShape(1);
    cacheAxes();
    fwdSpeed_ = glm::dot(state.velocity, col(state.rotation, 2));
    speed_ = glm::length(state.velocity);
    clearCockpitJoltAndShake();
}

// while a replay plays (0x52afac) the internal camera's update 0x5ba10 returns at once (0x5ba1d), so the tripod that
// 0x5aa10 builds afresh each step (+0xca0 at 0x5ac5e, +0xcac at 0x5acaf) gains neither the jolt nor the shake
void GameCraft::clearCockpitJoltAndShake()
{
    jolt_ = shake::ContactJolt();
    shake_ = shake::CameraShake();
}

void GameCraft::respawn(const AiGraph &graph, const SimWorld &world)
{   // stand-in for the restart point (0x5cb30 copies the race slot's 0x597330 + slot x 0x88)
    int node = lastRestart_;
    int nx = routeNext(graph, node);
    glm::vec3 from = graph.nodes[node].pos, ahead = graph.nodes[nx].pos - from;
    ahead.y = 0;
    ahead = glm::length(ahead) > 1e-3f ? glm::normalize(ahead) : glm::vec3(0, 0, 1);
    glm::vec3 u(0, 1, 0), r = glm::normalize(glm::cross(u, ahead));
    State state;
    state.position = from + glm::vec3(0, 2.f, 0);
    for (int i = 0; i < 3; ++i)
    {
        state.rotation[i * 3 + 0] = r[i];
        state.rotation[i * 3 + 1] = u[i];
        state.rotation[i * 3 + 2] = ahead[i];
    }
    states_[0] = states_[1] = state;
    dropToGround(world);
    node_ = node;
    airTime_ = 0;
    gear_ = 0;
    revs_ = 0;
    ++respawns_;
}

void GameCraft::frame(float dt, const SimWorld &world, const AiGraph &graph)
{
    if (!stats_ || dt <= 0)
    {
        return;
    }
    dt = std::min(dt, 0.1f);
    soundContacts_ = SoundContacts();
    dust_.clear();
    cacheAxes();
    trackProgress(graph, dt);
    turboOn_ = false;
    jolt_.beginFrame();
    controller_->control(*this, dt, graph);
    revsGears(dt);
    steerSmooth(dt);
    controller_->setEngine(*this);
    // 0xcf630: substeps sized from the velocity along the body axes (4..20 ms)
    float done = 0;
    while (done < dt)
    {
        State &state = currentState();
        float forwardRate = std::abs(glm::dot(state.velocity, col(state.rotation, 2))) * 0.333333343f;
        float sideRate = std::abs(glm::dot(state.velocity, col(state.rotation, 0))) * 0.5f;
        float upRate = std::abs(glm::dot(state.velocity, col(state.rotation, 1))) * 1.2f;
        float fastest = std::clamp(std::max({forwardRate, sideRate, upRate, 0.f}), 0.1f, 1000.f);
        float substep = 1.f / fastest;
        if (substep < 0.004f)
        {
            substep = 0.004f;
        }
        substep = std::clamp(std::min(substep, dt - done), 0.f, 0.02f);
        if (substep <= 0)
        {
            break;
        }
        forces(substep, world);                 // vtable[0]: 0x5d670
        integrate(substep);                     // 0xcedd0
        updateShape(current_ ^ 1);            // 0xd1620
        collide(world);                   // 0xd3c70 / 0xd5140
        updateShape(current_ ^ 1);
        current_ ^= 1;                        // swap state blocks
        done += substep;
    }
    cacheAxes();
    speedCap();
    tripodUpdate(dt);
    rumble(dt);
    airTime_ = (anyContact_ || inWater_) ? 0.f : airTime_ + dt;
    requestResets(dt);
    if (resetRequested_ && !retired_)
    {
        respawn(graph, world);   // 0x5cb30
    }
    resetRequested_ = false;
}

// what sets craft +5 besides the reset triangles: BACK, upside down, fallen, the drone's progress check
void GameCraft::requestResets(float dt)
{
    const bool backPressed = pad_.reset && !resetHeld_;
    resetHeld_ = pad_.reset;
    resetRequested_ |= backPressed && controller_ == &playerController;
    resetRequested_ |= upsideDownTooLong(dt);
    resetRequested_ |= currentState().position.y < -400.f;   // 0x5ddf8
    resetRequested_ |= controller_->respawnsWhenStuck() && stoppedProgressing(dt);
}

// 0x5b265 / 0x5ddcf: 5 s with the up axis below the horizon off the ground, then upside down (up.y < -0.8) out of water
bool GameCraft::upsideDownTooLong(float dt)
{
    upsideDownTime_ = (up_.y < 0.f && !onGround_) ? upsideDownTime_ + dt : 0.f;
    return up_.y < -0.8f && !inWater_ && upsideDownTime_ > 5.f;
}

// drone_update 0x6df3f: every 2 s the position joins a three-sample history (+0x1718 / +0x1724 / +0x1730); under
// 5 m from the one 4 s older asks for the reset and holds the next check off for 5 s more (+0x1714 = -5)
bool GameCraft::stoppedProgressing(float dt)
{
    checkTimer_ += dt;
    if (checkTimer_ <= 2.f)
    {
        return false;
    }
    checkTimer_ -= 2.f;
    checkHistory_ = {checkHistory_[1], checkHistory_[2], currentState().position};
    if (glm::length(checkHistory_[0] - checkHistory_[2]) >= 5.f)
    {
        return false;
    }
    checkTimer_ = -5.f;
    return true;
}

void GameCraft::retire(const glm::vec3 &at)
{
    State state;
    state.position = at;
    states_[0] = states_[1] = state;
    pos_ = at;
    retired_ = true;
}

} // namespace argon
