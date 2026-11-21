// Argon: translucent-pass draws lifted towards the eye by a hair (docs\argon\differences.md "Translucent layers on
// their ground"). The Xbox compares a 24-bit W-buffer with LESSEQUAL, so a translucent layer laid on the ground under it
// (Draco's grass on its rock, mesh 200 on 160, sharing most vertices but triangulated differently) ties with it and,
// drawn later, wins. Argon's float depth resolves the 0.1 mm the two cross by, so the winner flipped per triangle and
// the layer shimmered. Scaling the draw's clip-space z scales its depth (z / w) by the same factor, so the layer wins
// again. A mesh laid over the others (surface flag 0x20000, drawn last: Draco's sand on its grass) goes twice as far.
#pragma once
#include "model/material.hpp"

#include <glm/glm.hpp>

namespace argon
{

constexpr float kTranslucentDepthLift = 1.f / 4096.f;   // of the distance: 1.2 mm at 5 m, 12 mm at 50 m

constexpr float depthLiftOf(const MeshState &state)
{
    if (!state.translucentPass)
    {
        return 0.f;
    }
    return state.laidOver ? 2.f * kTranslucentDepthLift : kTranslucentDepthLift;
}

// reversed depth: larger is nearer, so the scale moves the draw towards the eye
inline glm::mat4 liftedDepth(glm::mat4 mvp, float lift)
{
    for (int column = 0; column < 4; ++column)
    {
        mvp[column][2] *= 1.f + lift;
    }
    return mvp;
}

}  // namespace argon
