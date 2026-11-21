// Argon: the race's rain (docs\effects\rain.md "Rain", rain.cpp in the game: the rain object 0x90880, one per
// view at 0x51cce8 + 0xd00 i, 0x5200e8 for one player). Each frame (0x91890) it scatters 1000 grey streaks in a 32 m
// box ahead of the camera, slides and dries 800 water drops on the lens (each a little lens: the view behind it
// magnified x 4 and turned over) and drifts a haze of raincloud.dds over the view.
#pragma once
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace argon::rain
{

// the race load (0x800b3): rain on Kalau (track id 1), alt_bracken (0xd) or everywhere with Always Rain (0x52c8b8)
bool raceRains(int trackId, bool alwaysRain);

// the game's rand (0xf96ea, the C runtime's): 15 bits a call
class GameRand
{
  public:
    explicit GameRand(uint32_t seed = 1) : state_(seed)
    {
    }
    int next();

  private:
    uint32_t state_;
};

// the view in the game's 640 x 480 pixels (+0xce0..+0xcec)
struct ViewRect
{
    glm::vec2 origin{0.f}, size{640.f, 480.f};

    glm::vec2 centre() const
    {
        return origin + size * 0.5f;
    }
    bool contains(glm::vec2 point) const;
};

// the view camera as the rain reads it: eye (+0x36c), velocity in m/s (+0x32c) and the view's back axis (+0x338)
struct RainCamera
{
    glm::vec3 eye{0.f}, velocity{0.f}, back{0.f, 0.f, 1.f};

    float forwardSpeed() const
    {
        return -glm::dot(velocity, back);
    }
};

// the race camera's rain state each frame (camera_set_eye 0xae640, the race view 0x93e20): the race rains (0x52afa2),
// the camera is under a roof (+0x350) or its eye under the water (0xce6b0 depth <= 0, +0x34e)
struct Weather
{
    bool raining = false, covered = false, underwater = false;
};

// 0xbcfa0: a spray quad (the wake's puffs and droplet sheets) wets the lens when its largest axis distance from the
// eye is under 2 m (0x1a3534)
bool sprayReachesLens(glm::vec3 eye, glm::vec3 sprayPos);

// a drop on the lens (+0xd4 / +0xd8 lists)
struct LensDrop
{
    glm::vec2 pos{-1000.f, 0.f};   // pixels
    float size = 0.f;              // radius / 64 pixels; dries by 0.0002 a frame, gone at 0
    glm::vec2 vel{0.f};            // pixels a frame
    float angleDeg = 0.f;          // the way it runs (its tail trails behind)
    float stretch = 0.f;           // 0..2: how far the tail reaches
    glm::vec2 shake{0.f};          // whole pixels, this frame only
};

// one drop drawn (0x910e0): its centre and 8 rim points, each with the echo texel (pixels) it shows
struct DropVertex
{
    glm::vec2 pos, texel;
};
using DropFan = std::array<DropVertex, 9>;

class LensDrops
{
  public:
    LensDrops(int count, ViewRect view);
    void clear();   // 0x90d60: every drop gone
    // 0x91890: slide = the camera's forward speed x 2e-5; up to newDropLimit dry drops come back when spawning
    void step(float slide, int newDropLimit, bool spawning, GameRand &rand);
    // 0x917e0 / the surfacing loop 0x94090: up to limit dried or strayed drops come back (0x90740)
    void wetLens(int limit, GameRand &rand);
    const std::vector<LensDrop> &drops() const
    {
        return drops_;
    }
    DropFan fan(const LensDrop &drop) const;

  private:
    void slideDrop(LensDrop &drop, float slide, GameRand &rand) const;
    bool expired(const LensDrop &drop) const;
    void respawn(LensDrop &drop, GameRand &rand) const;

    std::vector<LensDrop> drops_;
    ViewRect view_;
};

// a streak: the head drawn in the streak alpha, the tail fully transparent
struct Streak
{
    glm::vec3 head, tail;
};

// 0x90f00: the streaks for this frame, scattered +-16 m around 16 m ahead of the camera
void scatterStreaks(std::vector<Streak> &streaks, const RainCamera &camera, GameRand &rand);

// the raincloud.dds haze over the view (0x92270): it scrolls and its brightness wanders (+0x14) every frame
class RainCloud
{
  public:
    void step(GameRand &rand);
    uint32_t colour(float slide, float strength) const;   // ARGB
    glm::vec4 texels() const;                             // u0, v0, u1, v1

  private:
    float scroll_ = 0.f;    // +0xcf0
    float flicker_ = 1.f;   // +0x14: 0.3..1
};

class Rain
{
  public:
    static constexpr int kDrops = 800, kStreaks = 1000;   // 0x77508, one player

    explicit Rain(ViewRect view = {});
    // camera_set_eye + 0x93e20: the strengths +4 / +8 / +0xc; diving clears the lens (+0x34d), surfacing wets it all
    // (+0x34f)
    void setWeather(const Weather &weather);
    void splash();                         // 0xbcfa0 -> 0x917e0(20): a spray quad near the eye, 21 drops come back
    void step(const RainCamera &camera);   // one 60 Hz frame of 0x91890

    const std::vector<Streak> &streaks() const
    {
        return streaks_;
    }
    uint8_t streakAlpha() const;
    const LensDrops &lensDrops() const
    {
        return drops_;
    }
    bool cloudShown() const;
    uint32_t cloudColour() const
    {
        return cloud_.colour(slide_, cloudStrength_);
    }
    glm::vec4 cloudTexels() const
    {
        return cloud_.texels();
    }

  private:
    bool rainsOnLens() const;

    Weather weather_;
    float streakStrength_ = 0.f, cloudStrength_ = 0.f;   // +4 (70), +8 (90)
    int newDropLimit_ = 0;                               // +0xc (12)
    float slide_ = 0.f;                                  // +0x18
    GameRand rand_;
    LensDrops drops_;
    RainCloud cloud_;
    std::vector<Streak> streaks_;
};

}  // namespace argon::rain
