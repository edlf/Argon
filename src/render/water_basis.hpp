// The water's per-vertex tangent basis for one wave frame (0xcd170; docs/water/README.md "Wave frames").
// Each of the tile's 512 triangles (quad a: a, a + 1, a + 17 and a + 17, a + 1, a + 18; 0xcdac5) adds its shares
// (0xccf40, the same arithmetic as the road effect's 0xa2bb0) over position (8 i, h x scale / 32768, 8 j) and
// uv = position / 16; the edge vertices then add their opposite edge's sums (the tile repeats), tangent and binormal
// are normalised and the third row is tangent x binormal. The vertex program's rows v1 / v2 / v3.
#pragma once
#include "logic/water_waves.hpp"
#include "render/bump_basis.hpp"

#include <vector>

namespace argon
{

// WaterWaves::kVertices squared entries, x fastest
std::vector<BumpBasis> buildWaterBasis(const WaterWaves &waves, int frame);

} // namespace argon
