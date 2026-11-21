// Argon: the race end's music fades (audio\music_fade.hpp)
#include "audio/music_fade.hpp"

#include <cmath>

namespace argon
{

namespace
{
constexpr float kMbPerSecond = 10000.f;   // 0x1a35a8
constexpr float kFloorMb = -9600.f;       // 0x1a3968
constexpr float kDipRiseMb = 9000.f;      // 0x1a3964
constexpr double kDipTurn = 0.5;          // 0x1a3504

// the game's two compares in order (0x95419 .. 0x95441): under the floor -> the floor, else over the setting -> the
// setting (at setting -10000 mB the floor wins once the ramp is under it)
float clampToFloorThenSetting(float mb, float settingMb)
{
    if (mb < kFloorMb)
    {
        return kFloorMb;
    }
    if (mb > settingMb)
    {
        return settingMb;
    }
    return mb;
}

float rampDownMb(double elapsed)
{
    return -float(elapsed) * kMbPerSecond;
}

float rampUpMb(double elapsed)
{
    return float(elapsed) * kMbPerSecond - kDipRiseMb;
}

float dipMb(double elapsed, float settingMb)
{
    if (elapsed > MusicFade::kSeconds)
    {
        return settingMb;
    }
    const float mb = elapsed > kDipTurn ? rampUpMb(elapsed) : rampDownMb(elapsed);
    return clampToFloorThenSetting(mb, settingMb);
}
}  // namespace

float musicFadeMb(MusicFadeShape shape, double elapsed, float settingMb)
{
    if (shape == MusicFadeShape::Dip)
    {
        return dipMb(elapsed, settingMb);
    }
    return clampToFloorThenSetting(rampDownMb(elapsed), settingMb);
}

float musicFadeGain(float levelMb, float settingMb)
{
    return std::pow(10.f, (levelMb - settingMb) / 2000.f);
}

void MusicFade::start(MusicFadeShape fadeShape, double now)
{
    running = true;
    shape = fadeShape;
    startedAt = now;
}

void MusicFade::stop()
{
    running = false;
}

bool MusicFade::over(double now) const
{
    return running && elapsed(now) > kSeconds;
}

double MusicFade::elapsed(double now) const
{
    return now - startedAt;
}

float MusicFade::levelMb(double now, float settingMb) const
{
    return musicFadeMb(shape, elapsed(now), settingMb);
}

}  // namespace argon
