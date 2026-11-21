// Argon: the game's fixed-function D3D lighting of a cmm mesh (d3d_lighting.hpp).
#include "d3d_lighting.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{

namespace
{

constexpr uint32_t kOwnColour = 0x4;
constexpr uint32_t kSetup2Unlit = 0x20;
constexpr uint32_t kOwnColourIsLight = 0x100;
constexpr uint32_t kKeepSceneLight = 0x200;
constexpr uint32_t kSetLights = 0x2000;
constexpr uint32_t kSetup1Lit = 0x8000;

bool isLit(const SurfaceSetup &surface)
{
    if (!surface.hasBlock)
    {
        return false;
    }
    if (surface.setup == 1)
    {
        return (surface.flags & kSetup1Lit) != 0;
    }
    return surface.setup == 2 && (surface.flags & kSetup2Unlit) == 0;
}

void applyLightFlags(uint32_t flags, SurfaceLighting &lighting)
{
    if ((flags & kSetLights) == 0)
    {
        return;
    }
    if (flags & kOwnColourIsLight)
    {
        lighting.ambient = LightSource::Black;
        lighting.light0 = LightSource::Black;
    }
    else if ((flags & kKeepSceneLight) == 0)
    {
        lighting.light0 = LightSource::Black;
    }
}

void applyOwnColour(const SurfaceSetup &surface, SurfaceLighting &lighting)
{
    if ((surface.flags & kOwnColour) == 0 || !surface.hasBoneRecord)
    {
        return;
    }
    if (surface.flags & kOwnColourIsLight)
    {
        lighting.light0 = LightSource::Own;
    }
    else
    {
        lighting.ambient = LightSource::Own;
    }
}

int toByte(float channel)
{
    return int(std::lround(std::clamp(channel, 0.f, 1.f) * 255.f));
}

int groundByte(uint32_t ground, int channel)
{
    return int((ground >> (16 - 8 * channel)) & 0xffu);
}

} // namespace

SurfaceLighting surfaceLighting(const SurfaceSetup &surface)
{
    SurfaceLighting lighting;
    lighting.lit = isLit(surface);
    if (!surface.hasBlock)
    {
        return lighting;
    }
    applyLightFlags(surface.flags, lighting);
    if (surface.setup == 1 || surface.setup == 2)
    {
        applyOwnColour(surface, lighting);
    }
    return lighting;
}

SceneLights craftLights(uint32_t ground, const SceneLights &track)
{
    SceneLights craft;
    for (int channel = 0; channel < 3; ++channel)
    {
        const int ambient = std::clamp((groundByte(ground, channel) / 2 + toByte(track.ambient[channel])) / 2, 0, 255);
        craft.ambient[channel] = float(ambient) / 255.f;
    }
    const int sumChannel[3] = {0, 1, 1};
    for (int channel = 0; channel < 3; ++channel)
    {
        const int from = sumChannel[channel];
        const int mean = (groundByte(ground, from) + toByte(track.light0[from])) / 2;
        craft.light0[channel] = float(std::clamp(mean, 0, toByte(track.light0[channel]))) / 255.f;
    }
    return craft;
}

} // namespace argon
