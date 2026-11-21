// Argon: the flares' occlusion test rule (docs\effects\rendering.md "Flare occlusion", docs\models\README.md surface flag 0x40000):
// cmm_draw_mesh 0xb6e60 draws the first N of 256 points before the mesh inside a visibility test (0xa64f0) and sets
// its bone's intensity (+0x138) from the previous test's result (0xa6750); 0 skips the draw (0xb71f8).
#pragma once
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace argon::flare
{
constexpr int kTestPoints = 256;

// 0xa63b0: the points the tests draw, uniform in a unit sphere by rejection from the game's rand
std::vector<glm::vec3> testPoints();

// 0xb709e: a bone that has no test yet (+0x108 = 999999, set at load by 0xb9540) reads 0 visible points, so a
// flare is not drawn before its first result
constexpr float kUntestedIntensity = 0.f;

// the query's count as points: samples / samples per pixel with precise queries, else any sample = all points
inline float visiblePoints(uint64_t samplesPassed, int samplesPerPixel, int pointCount)
{
    if (samplesPerPixel > 0)
    {
        return float(samplesPassed) / float(samplesPerPixel);
    }
    return samplesPassed ? float(pointCount) : 0.f;
}

// 0xb70ea: min(1, 2 x visible / N): full with half the points visible
inline float intensity(float visible, int pointCount)
{
    return std::min(1.f, 2.f * visible / float(std::max(1, pointCount)));
}

// 0xb7109..0xb7187 (a tested mesh drawn through an effect): each byte of the bone record's tinted colour +0x10c is
// its tint byte (+0x104) x the intensity, rounded (fistp, round to nearest even). The "Color Scale No Texture" draw
// (0x7a4b0) passes it as c4 = byte / 255, which its vertex program multiplies the vertex colour by
inline float tintedChannel(uint8_t tint, float intensity)
{
    return std::nearbyint(float(tint) * intensity) / 255.f;
}
} // namespace argon::flare
