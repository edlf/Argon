// Argon: the replay cameras' director (logic\replay_director.hpp, docs\replays\users.md "Replay cameras").
#include "logic/replay_director.hpp"

#include <algorithm>
#include <cmath>

namespace argon::replay
{
namespace
{

constexpr float kLapPercent = 100.f;
constexpr float kLeadBySpeed[5] = {0.15f, 0.13f, 0.11f, 0.08f, 0.05f};   // 0x1a35cc .. 0x1a36e4 by global_game_speed
constexpr float kTrackingRate = 10.f;                                      // 0x1a35a4
constexpr float kFovRate = 2.f;                                            // the fov's ease, twice dt a frame
constexpr float kNarrowestFov = 5.f, kWidestFov = 90.f;                    // 0x1a3554, 0x1a3500
constexpr float kKerbReach = 10.f, kKerbScale = 0.05f;                     // 0x1a35a4, 0x1a36e4
constexpr float kKerbRise = 10.f, kKerbFall = 5.f, kKerbMostShake = 0.6f;  // 0x1a35a4, 0x1a3554, 0x1a3558
constexpr float kKerbPhaseRate = 3.f, kKerbAmplitude = 0.02f;              // 0x1a351c, 0x1a36d8 (sines x 15 / 13 / 10)

float lapWrapped(float lapPercent)
{
    while (lapPercent > kLapPercent)
    {
        lapPercent -= kLapPercent;
    }
    return lapPercent;
}

float distanceFromEye(const glm::vec3 &eye, const FollowedCraft &craft)
{
    return glm::length(craft.pos - eye);
}

// DYNAMICFOV: the fov eases towards 90 less `shrink` per metre of distance, never under 5
float easedDynamicFov(float fov, float distance, float shrink, float dt)
{
    const float wanted = std::max(kNarrowestFov, kWidestFov - distance * shrink);
    return fov + kFovRate * dt * (wanted - fov);
}

// 0xd76e0 (static): a fixed eye; DYNAMICTRACKING follows the craft (a little ahead of it, by
// the speed class), else the camera looks at its target; KERB shakes it while racers pass close; DYNAMICFOV zooms with
// the distance (90 - 0.7 per m)
class StaticCamera : public DirectorCamera
{
  public:
    StaticCamera(const TrackCamera &config, float lead) : DirectorCamera(config), lead(lead), fov(config.fov)
    {
    }

    // 0xd7b00: ONLYFORWARD / ONLYINFRAME rule out a craft behind / outside the view, the lap window rules out the rest;
    // a held camera scores 100, else 100 x how far inside its range the craft is
    float score(const FollowedCraft &craft) override
    {
        if (outOfView(craft) || !inLapWindow(lapWrapped(craft.lapPercent)))
        {
            return kOffScore;
        }
        if (held())
        {
            return kHeldScore;
        }
        const float distance = distanceFromEye(eye(), craft);
        return distance <= config.range ? (config.range - distance) / config.range * kHeldScore : 0.f;
    }

    // 0xd75f0: shown for its hold time, at least 1 s, at its own fov
    void enter(CameraShot &) override
    {
        hold = std::max(config.hold, kMinHold);
        fov = config.fov;
        shake.reset();
    }

    void update(CameraShot &shot, const ReplayScene &scene, float dt) override
    {
        const FollowedCraft &craft = scene.followed;
        hold -= dt;
        if (config.dynamicTracking)
        {
            shot.look.x += ((craft.pos.x - shot.look.x) + lead * craft.vel.x) * dt * kTrackingRate;
            shot.look.y = craft.pos.y;
            shot.look.z += ((craft.pos.z - shot.look.z) + lead * craft.vel.z) * dt * kTrackingRate;
        }
        else
        {
            shot.look = target();
        }
        if (config.kerb)
        {
            shakeOverTarget(shot, scene.racers, dt);
        }
        if (config.dynamicFov)
        {
            fov = easedDynamicFov(fov, distanceFromEye(eye(), craft), kStaticFovShrink, dt);
        }
        shot.eye = eye();
        shot.fov = fov;
    }

    // while it shakes the look point is the target's, jittered, even on a tracking camera
    void shakeOverTarget(CameraShot &shot, const std::vector<PassingCraft> &racers, float dt)
    {
        shake.step(KerbShake::wanted(eye(), racers), dt);
        if (shake.shaking())
        {
            shot.look = target() + shake.offset();
        }
    }

    void leave() override
    {
        hold = 0.f;   // 0xd7af0
    }

  private:
    static constexpr float kMinHold = 1.f, kStaticFovShrink = 0.7f;   // 0x1a3694
    static constexpr float kInFrame = -0.8f;                          // 0x1a36d4

    glm::vec3 eye() const
    {
        return {config.pos[0], config.pos[1], config.pos[2]};
    }
    glm::vec3 target() const
    {
        return {config.target[0], config.target[1], config.target[2]};
    }

    bool outOfView(const FollowedCraft &craft) const
    {
        if (!config.onlyForward && !config.onlyInFrame)
        {
            return false;
        }
        const glm::vec3 back = glm::normalize(eye() - target());
        const float facing = glm::dot(back, glm::normalize(craft.pos - eye()));
        return (config.onlyForward && facing > 0.f) || (config.onlyInFrame && facing > kInFrame);
    }

    float lead, fov;
    KerbShake shake;
};

// 0xd68a0 (moving): the eye slides between its start and end, to where its line of sight best points at the craft
// (11 tries 0.003 apart a frame, eased 4 / s); the look follows the craft with a slow wobble; DYNAMICFOV 90 - 0.6 per
// m, TIGHTFOV 30 - 1 per m (at least 3) on the craft itself, PRESETFOV from fov1 to fov2 along the slide
class MovingCamera : public DirectorCamera
{
  public:
    MovingCamera(const TrackCamera &config, float lead) : DirectorCamera(config), lead(lead)
    {
    }

    // 0xd6e50: 100 inside its lap window, else ruled out; it is never held
    float score(const FollowedCraft &craft) override
    {
        return inLapWindow(lapWrapped(craft.lapPercent)) ? kHeldScore : kOffScore;
    }

    void enter(CameraShot &) override
    {
        slide = smoothSlide = 0.f;   // 0xd6890
    }

    void update(CameraShot &shot, const ReplayScene &scene, float dt) override
    {
        const FollowedCraft &craft = scene.followed;
        slide = bestSlide(craft);
        smoothSlide += (slide - smoothSlide) * dt * kSlideRate;
        shot.eye = eyeAt(smoothSlide);
        followWithWobble(shot, craft, dt);
        shot.fov = fovFor(shot, craft, dt);
        age += dt;
    }

    void leave() override
    {
    }

  private:
    static constexpr float kSlideStep = 0.003f, kSlideRate = 4.f;   // 0x1a367c, 0x1a3444
    static constexpr float kMovingFovShrink = 0.6f, kTightFov = 30.f, kTightestFov = 3.f, kTightRate = 5.f;

    glm::vec3 eyeAt(float share) const
    {
        return glm::mix(glm::vec3(config.pos[0], config.pos[1], config.pos[2]), glm::vec3(config.pos2[0], config.pos2[1], config.pos2[2]), share);
    }
    glm::vec3 targetAt(float share) const
    {
        return glm::mix(glm::vec3(config.target[0], config.target[1], config.target[2]), glm::vec3(config.target2[0], config.target2[1], config.target2[2]), share);
    }

    float bestSlide(const FollowedCraft &craft) const
    {
        float best = slide, bestFacing = -2.f;
        for (int k = -5; k <= 5; ++k)
        {
            const float slideShare = std::clamp(slide + float(k) * kSlideStep, 0.f, 1.f);
            const float facing = glm::dot(glm::normalize(targetAt(slideShare) - eyeAt(slideShare)), glm::normalize(craft.pos - eyeAt(slideShare)));
            if (facing > bestFacing)
            {
                bestFacing = facing;
                best = slideShare;
            }
        }
        return best;
    }

    // the look point eases to the craft (and a little ahead), the rates wobbling with sines of the camera's age
    void followWithWobble(CameraShot &shot, const FollowedCraft &craft, float dt) const
    {
        const float wobbledLead = std::sin(age * 1.5f) * 0.025f + lead;
        shot.look.x += ((craft.pos.x - shot.look.x) + wobbledLead * craft.vel.x) * (std::sin(age * 1.2f) + kTrackingRate) * dt;
        shot.look.z += ((craft.pos.z - shot.look.z) + wobbledLead * craft.vel.z) * (std::sin(age * 0.9f) + kTrackingRate) * dt;
        shot.look.y = craft.pos.y;
    }

    float fovFor(CameraShot &shot, const FollowedCraft &craft, float dt) const
    {
        float fov = shot.fov;
        const float distance = distanceFromEye(shot.eye, craft);
        if (config.dynamicFov)
        {
            fov = easedDynamicFov(fov, distance, kMovingFovShrink, dt);
        }
        if (config.tightFov)
        {
            fov += (std::max(kTightestFov, kTightFov - distance) - fov) * dt * kTightRate;
            shot.look.x = craft.pos.x;
            shot.look.z = craft.pos.z;
        }
        if (config.presetFov)
        {
            fov = config.fov + (config.fov2 - config.fov) * smoothSlide;
        }
        return fov;
    }

    float lead, slide = 0.f, smoothSlide = 0.f, age = 0.f;
};

// 0xd7110 (ship): on the craft, at offsets in its own space (x forward, y left, z up); scores 5 when its 25 s rest
// after being shown has run out (0xd7020), held 1.5 s
class ShipCamera : public DirectorCamera
{
  public:
    using DirectorCamera::DirectorCamera;

    float score(const FollowedCraft &) override
    {
        wanted = std::max(wanted - kWantDecay, 0.f);
        if (rest <= 0.f)
        {
            wanted = kWanted;
        }
        else
        {
            rest -= kRestStep;
        }
        return std::clamp(wanted, 0.f, kWanted);
    }

    void enter(CameraShot &) override
    {
        hold = kHold;   // 0xd70f0
    }

    void update(CameraShot &shot, const ReplayScene &scene, float dt) override
    {
        hold -= dt;
        shot.eye = onCraft(scene.followed, config.pos);
        shot.look = onCraft(scene.followed, config.target);
        shot.fov = config.fov;
    }

    void leave() override
    {
        rest = kRest;   // 0xd7380
    }

  private:
    static constexpr float kWanted = 5.f, kWantDecay = 0.1f, kRest = 15.f, kRestStep = 0.01f, kHold = 1.5f;

    static glm::vec3 onCraft(const FollowedCraft &craft, const std::array<float, 3> &offset)
    {
        return craft.pos + craft.forward * offset[0] - craft.right * offset[1] + craft.up * offset[2];
    }

    float wanted = 0.f, rest = 0.f;
};

// heli (vtable 0x198880): scores 0 and does nothing (0xd6530 / 0xd6520)
class HeliCamera : public DirectorCamera
{
  public:
    using DirectorCamera::DirectorCamera;
    float score(const FollowedCraft &) override
    {
        return 0.f;
    }
    void enter(CameraShot &) override
    {
    }
    void update(CameraShot &, const ReplayScene &, float) override
    {
    }
    void leave() override
    {
    }
};

std::unique_ptr<DirectorCamera> cameraFor(const TrackCamera &config, float lead)
{
    if (config.type == "moving")
    {
        return std::make_unique<MovingCamera>(config, lead);
    }
    if (config.type == "ship")
    {
        return std::make_unique<ShipCamera>(config);
    }
    if (config.type == "heli")
    {
        return std::make_unique<HeliCamera>(config);
    }
    return std::make_unique<StaticCamera>(config, lead);
}

}  // namespace

// 0xd7842..0xd78ed: every racer of the race manager (0x596df8 of them) closer to the eye than 10 m
float KerbShake::wanted(const glm::vec3 &eye, const std::vector<PassingCraft> &racers)
{
    float level = 0.f;
    for (const PassingCraft &racer : racers)
    {
        const float distance = glm::length(racer.pos - eye);
        if (distance < kKerbReach)
        {
            level += (kKerbReach - distance) * kKerbScale * racer.speed * kKerbScale;
        }
    }
    return level;
}

void KerbShake::reset()
{
    level = 0.f;   // 0xd75f0 clears +0xc8 / +0xcc
    phase = 0.f;
}

// 0xd78f5..0xd79a3, kept as the game has it: rising, the level jumps by the wanted level less 10 dt x itself; falling,
// by the wanted level less 5 dt x itself; then 0 .. 0.6, and 5 / s more while still under the wanted level
void KerbShake::step(float wantedLevel, float dt)
{
    if (wantedLevel > level)
    {
        level += wantedLevel - dt * level * kKerbRise;
    }
    if (wantedLevel < level)
    {
        level += wantedLevel - dt * level * kKerbFall;
    }
    level = level < 0.f ? 0.f : std::min(level, kKerbMostShake);
    if (level < wantedLevel)
    {
        level += dt * kKerbFall;
    }
    if (shaking())
    {
        phase += dt * kKerbPhaseRate;
    }
}

glm::vec3 KerbShake::offset() const
{
    const float size = level * kKerbAmplitude;
    return glm::vec3(std::sin(phase * 15.f), std::cos(phase * 13.f), std::sin(phase * 10.f)) * size;
}

// panrange a b: inside a .. b; with b at or below a the window wraps past 100 and only b < p < a is outside
bool DirectorCamera::inLapWindow(float lapPercent) const
{
    if (config.panTo <= config.panFrom)
    {
        return !(lapPercent < config.panFrom && config.panTo < lapPercent);
    }
    return lapPercent >= config.panFrom && lapPercent <= config.panTo;
}

void ReplayDirector::reset(const std::vector<TrackCamera> &configs, int speedClass)
{
    const float lead = kLeadBySpeed[std::clamp(speedClass, 0, 4)];
    cameras.clear();
    firstStatic = -1;
    for (const TrackCamera &config : configs)
    {
        if (firstStatic < 0 && config.type == "static")
        {
            firstStatic = int(cameras.size());
        }
        cameras.push_back(cameraFor(config, lead));
    }
    firstStatic = std::max(firstStatic, 0);
    current_ = -1;
    shot = CameraShot{};
}

// 0xd6340: the first static camera to start with; the current one is scored, then all; the best takes over once the
// current one is no longer held; the current one then places the shot
CameraShot ReplayDirector::step(const ReplayScene &scene, float dt)
{
    if (cameras.empty())
    {
        return shot;
    }
    if (current_ < 0)
    {
        current_ = firstStatic;
        shot.look = scene.followed.pos;
    }
    cameras[size_t(current_)]->score(scene.followed);
    const int best = bestCamera(scene.followed);
    if (best != current_ && !cameras[size_t(current_)]->held())
    {
        switchTo(best);
    }
    cameras[size_t(current_)]->update(shot, scene, dt);
    return shot;
}

int ReplayDirector::bestCamera(const FollowedCraft &craft)
{
    int best = -1;
    float bestScore = -1000.f;
    for (size_t i = 0; i < cameras.size(); ++i)
    {
        const float score = cameras[i]->score(craft);
        if (score > bestScore)
        {
            bestScore = score;
            best = int(i);
        }
    }
    return best;
}

void ReplayDirector::switchTo(int camera)
{
    cameras[size_t(current_)]->leave();
    current_ = camera;
    cameras[size_t(current_)]->enter(shot);
}

}  // namespace argon::replay
