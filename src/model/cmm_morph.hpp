// Which two morph keyframes a "Keyframe ..." effect blends, and how (draw 0x7b7e0; docs/models/meshes.md "Morph meshes").
// The cycle lasts streams / streamsPerSecond seconds and loops; the phase picks frame f = phase x streams, rounded to
// the nearest key with the FPU's default rounding (fistp: to nearest, ties to even), so the weight runs from -0.5 to
// 0.5 and the blend leans past the nearer key; from = streams - 1 blends into key 0.
#pragma once
#include "model/cmm.hpp"

#include <cstdint>

namespace argon
{

struct MorphBlend
{
    uint32_t from = 0, to = 0;   // keyframes (stream indices)
    float weight = 0.f;          // position = (1 - weight) x from + weight x to (vertex shader c5 = (weight, 1 - weight))
};

MorphBlend morphBlendAt(float seconds, const CmmMorph &morph);

} // namespace argon
