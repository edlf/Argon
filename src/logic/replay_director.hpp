// Argon: the replay cameras' director, the game's CReplay (D:\neon_src\Misc\Source\replay.cpp; docs\replays\README.md
// "Replay cameras"). config\camera_<t>.txt's cameras each score the followed craft every frame (vtable +4); the
// current camera keeps the view until its hold time has run out, then the best-scoring one takes over ("Changed to cam
// %d with score %.1f", 0xd6340) and writes the "replay0output" tripod that camera0 follows in its mode 0 ("replay on").
// The four kinds as the game's classes: static (vtable 0x198830), moving (0x1988a8), ship (0x198858), heli (0x198880).
// Pure: no window, GPU or game state.
#pragma once
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "track/trackdata.hpp"

namespace argon::replay
{

// the followed craft (the director's input dataport): position, velocity, axes, how far round the lap (0..100 %)
struct FollowedCraft
{
    glm::vec3 pos{0.f}, vel{0.f};
    glm::vec3 right{1.f, 0.f, 0.f}, up{0.f, 1.f, 0.f}, forward{0.f, 0.f, 1.f};
    float lapPercent = 0.f;
};

// a racer as the race manager lists it for the KERB cameras: its position (0x596e48[i] +8) and speed (the length of
// its velocity, 0x596e6c[i] +8)
struct PassingCraft
{
    glm::vec3 pos{0.f};
    float speed = 0.f;
};

// what the director sees each frame: the followed craft and every racer
struct ReplayScene
{
    FollowedCraft followed;
    std::vector<PassingCraft> racers;
};

// the output tripod ("replay0output"): +8 the eye, +0x20 the point looked at, +0x38 the field of view
struct CameraShot
{
    glm::vec3 eye{0.f}, look{0.f, 0.f, 1.f};
    float fov = 60.f;
};

constexpr float kOffScore = -100.f;   // 0x1a344c: out of its lap window (or facing the wrong way)
constexpr float kHeldScore = 100.f;   // 0x1a3450: a camera still held; a moving one in its window

// KERB (static cameras, 0xd7824..0xd7a57): racers within 10 m of the eye (each (10 - d) x 0.05 x speed x 0.05) set how
// much the camera shakes; the level chases that (at most 0.6) and, while above 0, the look point jitters around the
// camera's target by sines of a phase that runs 3 / s: (sin 15p, cos 13p, sin 10p) x level x 0.02
class KerbShake
{
  public:
    static float wanted(const glm::vec3 &eye, const std::vector<PassingCraft> &racers);
    void reset();
    void step(float wantedLevel, float dt);
    bool shaking() const
    {
        return level > 0.f;
    }
    glm::vec3 offset() const;

  private:
    float level = 0.f;   // +0xcc
    float phase = 0.f;   // +0xc8
};

class DirectorCamera
{
  public:
    explicit DirectorCamera(const TrackCamera &config) : config(config)
    {
    }
    virtual ~DirectorCamera() = default;
    virtual float score(const FollowedCraft &craft) = 0;   // vtable +4
    virtual void enter(CameraShot &shot) = 0;              // +0x10
    virtual void update(CameraShot &shot, const ReplayScene &scene, float dt) = 0;   // +0x14
    virtual void leave() = 0;                              // +0x18
    bool held() const
    {
        return hold > 0.f;
    }

  protected:
    bool inLapWindow(float lapPercent) const;   // panrange, wrapping past 100

    TrackCamera config;
    float hold = 0.f;   // +0x58: counts down while the camera is shown
};

class ReplayDirector
{
  public:
    // the cameras of the track; speedClass 0..4 (global_game_speed) sets how far ahead the tracking cameras look
    void reset(const std::vector<TrackCamera> &cameras, int speedClass);
    CameraShot step(const ReplayScene &scene, float dt);   // 0xd6340, once a frame
    int current() const
    {
        return current_;
    }
    bool empty() const
    {
        return cameras.empty();
    }

  private:
    int bestCamera(const FollowedCraft &craft);   // scoring moves the ship cameras' rest on
    void switchTo(int camera);

    std::vector<std::unique_ptr<DirectorCamera>> cameras;
    CameraShot shot;
    int current_ = -1;
    int firstStatic = 0;
};

}  // namespace argon::replay
