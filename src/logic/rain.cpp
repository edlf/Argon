// Argon: the race's rain (rain.hpp, docs\effects\rain.md "Rain")
#include "logic/rain.hpp"

#include <algorithm>
#include <cmath>

namespace argon::rain
{
namespace
{
constexpr float kDegToRad = 0.017453292f;
constexpr float kNewDropSize = 0.03f, kNewDropSizeRange = 0.08f;   // 0x1a3530, 0x1a3544
constexpr float kDryPerFrame = 0.0002f;                             // 0x1a36a8
constexpr float kRimRadius = 64.f, kRimTexelRadius = 256.f;         // 0x1a36b8, 0x1a3710
constexpr float kSlideGain = 0.6f;                                  // 0x1a3558
// 0x1b3c90: the rim's reach per point, x stretch: the point behind the drop's run (4) is its tail
constexpr std::array<float, 8> kRimShape = {0.f, -0.15f, -0.4f, 0.15f, 1.f, 0.15f, -0.4f, -0.15f};

int lowByte(GameRand &rand)
{
    return rand.next() & 0xff;
}

float wholePixels(float v)
{
    return std::nearbyint(v);   // fistp, round to nearest
}
}  // namespace

bool raceRains(int trackId, bool alwaysRain)
{
    const int kKalau = 1, kAltBracken = 0xd;
    return trackId == kKalau || trackId == kAltBracken || alwaysRain;
}

// the sprite pass's list A (0xbcfa0) measures max(|dx|, |dy|, |dz|) from the eye
bool sprayReachesLens(glm::vec3 eye, glm::vec3 sprayPos)
{
    const float kReach = 2.f;   // 0x1a3534
    const glm::vec3 distance = glm::abs(sprayPos - eye);
    return std::max({distance.x, distance.y, distance.z}) < kReach;
}

int GameRand::next()
{
    state_ = state_ * 214013u + 2531011u;
    return int((state_ >> 16) & 0x7fff);
}

bool ViewRect::contains(glm::vec2 point) const
{
    const glm::vec2 end = origin + size;
    return point.x >= origin.x && point.x <= end.x && point.y >= origin.y && point.y <= end.y;
}

LensDrops::LensDrops(int count, ViewRect view) : drops_(size_t(count)), view_(view)
{
}

void LensDrops::clear()
{
    std::fill(drops_.begin(), drops_.end(), LensDrop{});
}

void LensDrops::step(float slide, int newDropLimit, bool spawning, GameRand &rand)
{
    int spawned = 0;
    for (LensDrop &drop : drops_)
    {
        if (drop.size > kNewDropSizeRange)
        {
            slideDrop(drop, slide, rand);
        }
        else
        {
            drop.stretch = drop.angleDeg = 0.f;
        }
        if (drop.size > 0.f)
        {
            drop.size -= kDryPerFrame;
        }
        if (expired(drop) && spawning && spawned < newDropLimit)
        {
            respawn(drop, rand);
            ++spawned;
        }
        const float shake = slide * kSlideGain;
        drop.shake.x = wholePixels(shake * float(lowByte(rand) - 0x80));
        drop.shake.y = wholePixels(shake * float(lowByte(rand) - 0x80));
    }
}

void LensDrops::wetLens(int limit, GameRand &rand)
{
    int wet = 0;
    for (LensDrop &drop : drops_)
    {
        if (wet >= limit)
        {
            return;
        }
        if (expired(drop))
        {
            respawn(drop, rand);
            ++wet;
        }
    }
}

// a big drop runs away from the view's centre, faster the faster the camera goes (0x91b40..0x91d50); the game's
// sideways nudge is (rand / 255 - 128) x slide x 0.6 (always about -128: kept as the game has it)
void LensDrops::slideDrop(LensDrop &drop, float slide, GameRand &rand) const
{
    const glm::vec2 slideOffset = (drop.pos - view_.centre()) * slide * kSlideGain;
    if ((rand.next() & 0xf) < 4)
    {
        drop.vel = glm::clamp(slideOffset * 10.f, glm::vec2(-2.f), glm::vec2(2.f));
    }
    else if ((rand.next() & 3) == 0)
    {
        drop.vel.x += (float(lowByte(rand)) / 255.f - 128.f) * slide * kSlideGain;
        drop.vel.y += (float(lowByte(rand)) / 255.f - 128.f) * slide * kSlideGain;
    }
    if ((rand.next() & 0xf) < 13)
    {
        drop.pos += drop.vel;
    }
    drop.angleDeg = std::atan2(drop.vel.x, drop.vel.y) / kDegToRad;
    drop.stretch = std::min((std::abs(slideOffset.x) + std::abs(slideOffset.y)) * kSlideGain, 2.f);
}

// 0x91820: dried up or off the view
bool LensDrops::expired(const LensDrop &drop) const
{
    return drop.size <= kNewDropSize || !view_.contains(drop.pos);
}

// 0x90740: anywhere on the view, radius 0.03..0.11 (x 64 pixels); its old run speed stays
void LensDrops::respawn(LensDrop &drop, GameRand &rand) const
{
    drop.pos.x = float(rand.next() & 0x1ff) * view_.size.x / 511.f + view_.origin.x;
    drop.pos.y = float(rand.next() & 0x1ff) * view_.size.y / 511.f + view_.origin.y;
    drop.size = float(lowByte(rand)) / 255.f * kNewDropSizeRange + kNewDropSize;
}

// 0x910e0: the rim point k at (k x 45 + the run angle) degrees, out by (shape x stretch + 1) x size x 64; its texel
// is as far the other way x 4: the drop shows the view around it magnified and turned over
DropFan LensDrops::fan(const LensDrop &drop) const
{
    const glm::vec2 centre = drop.pos + drop.shake;
    DropFan fan;
    fan[0] = {centre, centre};
    for (size_t k = 0; k < kRimShape.size(); ++k)
    {
        const float angle = (float(k) * 45.f + drop.angleDeg) * kDegToRad;
        const glm::vec2 dir = glm::vec2(std::sin(angle), std::cos(angle)) * (kRimShape[k] * drop.stretch + 1.f) * drop.size;
        fan[k + 1] = {centre + dir * kRimRadius, centre - dir * kRimTexelRadius};
    }
    return fan;
}

// head = centre + h + (0..3.2 m in x and z), tail = centre - h, h = velocity x -0.015 + (0.5, -2, 0.5): the head
// 2 m lower, leaning back against the camera's motion
void scatterStreaks(std::vector<Streak> &streaks, const RainCamera &camera, GameRand &rand)
{
    const glm::vec3 ahead = camera.eye - camera.back * 16.f;
    const glm::vec3 half = camera.velocity * -0.015f + glm::vec3(0.5f, -2.f, 0.5f);
    for (Streak &streak : streaks)
    {
        glm::vec3 centre = ahead;
        for (int axis = 0; axis < 3; ++axis)
        {
            centre[axis] += float((rand.next() & 0xfff) - 0x800) / 128.f;
        }
        const float spreadX = float(lowByte(rand)) * 0.0125f;
        const float spreadZ = float(lowByte(rand)) * 0.0125f;
        streak.head = centre + half + glm::vec3(spreadX, 0.f, spreadZ);
        streak.tail = centre - half;
    }
}

// end of 0x91890: the haze scrolls 0.06 a frame (wrapping at 1024); its brightness steps 0.04 up or down at random,
// kept within 0.3..1
void RainCloud::step(GameRand &rand)
{
    scroll_ -= 0.06f;
    if (scroll_ < 0.f)
    {
        scroll_ += 1024.f;
    }
    const bool rising = (rand.next() & 1) == 0 ? flicker_ < 1.f : flicker_ <= 0.3f;
    flicker_ += rising ? 0.04f : -0.04f;
}

// 0x92270: grey 0xa0 with alpha (flicker x 4000 x slide + 20) x strength / 100: about 18 standing, 40 at 300 m/s
uint32_t RainCloud::colour(float slide, float strength) const
{
    const auto dim = int32_t(std::nearbyint((flicker_ * 4000.f * slide + 20.f) * strength * -0.01f));
    return 0xa0a0a0u - (uint32_t(dim) << 24);
}

glm::vec4 RainCloud::texels() const
{
    const float u = scroll_ * 0.125f, v = scroll_ * 0.25f;
    return {u, v, u + 0.25f, v + 0.25f};
}

Rain::Rain(ViewRect view) : drops_(kDrops, view), streaks_(size_t(kStreaks))
{
}

// camera_set_eye (0xae640): out of the water on a raining track the streaks at 70, the haze at 90 and 12 new drops
// a frame, otherwise all 0; going under clears the lens, coming up brings back every dried drop at once (the race
// view 0x93e20 applies both before the frame's 0x91890). A dry track keeps that wet lens until it dries.
void Rain::setWeather(const Weather &weather)
{
    if (weather.underwater && !weather_.underwater)
    {
        drops_.clear();
    }
    else if (!weather.underwater && weather_.underwater)
    {
        drops_.wetLens(kDrops, rand_);
    }
    weather_ = weather;
    const bool rains = rainsOnLens();
    streakStrength_ = rains ? 70.f : 0.f;
    cloudStrength_ = rains ? 90.f : 0.f;
    newDropLimit_ = rains ? 12 : 0;
}

// 0x917e0 is passed 20 but stops only once it has brought back more than that
void Rain::splash()
{
    const int kSplashDrops = 21;
    drops_.wetLens(kSplashDrops, rand_);
}

// the streaks stay as they were under a roof (the game draws its last buffer again)
void Rain::step(const RainCamera &camera)
{
    slide_ = std::clamp(std::max(camera.forwardSpeed() * 1e-4f, 0.f) * 0.2f, -2000.f, 2000.f);
    if (!weather_.covered)
    {
        scatterStreaks(streaks_, camera, rand_);
    }
    drops_.step(slide_, newDropLimit_, rainsOnLens() && !weather_.covered, rand_);
    cloud_.step(rand_);
}

// the camera's +0x34c: raining and the eye out of the water
bool Rain::rainsOnLens() const
{
    return weather_.raining && !weather_.underwater;
}

uint8_t Rain::streakAlpha() const
{
    return uint8_t(std::nearbyint(streakStrength_ * 0.5f));
}

bool Rain::cloudShown() const
{
    return cloudStrength_ > 0.f && rainsOnLens() && !weather_.covered;
}

}  // namespace argon::rain
