// The water's wave heightfield, as the physics reads it (docs/water/README.md "Physics").
// 0xcd4f0 builds 30 wave frames of 17 x 17 heights (shorts) once per track; 0xce6b0 answers "how deep is this point"
// for the craft (hover rays, buoyancy) against the water.dat boxes, with the waves added to the box top.
#pragma once
#include "track/trackdata.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace argon
{

class WaterWaves
{
  public:
    static constexpr int kFrames = 30;         // one every 8 game frames: a 4 s loop
    static constexpr int kVertices = 17;       // per side of a 128 m tile, 8 m apart
    static constexpr float kTileSize = 128.f;
    static constexpr float kGridStep = 8.f;              // metres between vertices (0x1a3460 / 16; 0x1a3468 = 1 / 8)
    static constexpr float kShortToUnit = 1.f / 32768.f; // a stored short to a unit height (0x1a345c)

    // waveScale: water.dat's header float (0xcdefd -> 0x1a84f8; 0 = flat water)
    explicit WaterWaves(float waveScale = 0.f);

    // the stored short of frame f at grid vertex (i along x, j along z):
    // 5000 sin(45 i + t) + 5000 cos(45 j + t) + 2000 sin(t - 90 i) + 2000 cos(t - 90 j), t = 12 f degrees, rounded
    int16_t height(int frame, int i, int j) const;
    float scale() const
    {
        return scale_;
    }

    // 0xce6b0's wave: metres above the box top at (x, z) after `seconds` of the game clock
    float offsetAt(float x, float z, float seconds) const;

  private:
    float scale_;
    std::vector<int16_t> heights_;   // kFrames x kVertices x kVertices, x fastest
};

// 0xce6b0: the first box whose x / z range holds p (min <= p < max) with p.y >= its bottom; depth = p.y - (top +
// wave), the wave 0 for physics-only boxes (flags bit 0). false when no box holds p.
bool waterDepthAt(const std::vector<WaterBox> &boxes, const WaterWaves &waves, const glm::vec3 &point, float seconds, float &depth);

} // namespace argon
