// Morph keyframe blending (cmm_morph.hpp): 0x7b7e0, and Cmm::morph.
#include "model/cmm_morph.hpp"

#include <cmath>
#include <cstring>

namespace argon
{

namespace
{

constexpr uint32_t kHasPositions = 1;   // CmmMorph::kind bit: the stream's vertices start with a position

float cyclePhase(float seconds, float cycleSeconds)
{
    const float cycles = std::floor(seconds / cycleSeconds);
    return (seconds - cycles * cycleSeconds) / cycleSeconds;
}

} // namespace

MorphBlend morphBlendAt(float seconds, const CmmMorph &morph)
{
    MorphBlend blend;
    if (morph.streams == 0 || !(morph.streamsPerSecond > 0.f))
    {
        return blend;
    }
    const float cycleSeconds = float(morph.streams) / morph.streamsPerSecond;
    const float frame = float(morph.streams) * cyclePhase(seconds, cycleSeconds);
    const float nearest = std::nearbyint(frame);
    blend.from = nearest >= float(morph.streams) ? 0u : uint32_t(nearest);
    blend.to = blend.from == morph.streams - 1 ? 0u : blend.from + 1;
    blend.weight = std::fmin(frame - float(blend.from), 1.f);
    return blend;
}

void Cmm::morph(const CmmMesh &mesh, float seconds, Vertex *out) const
{
    if (!(mesh.morph.kind & kHasPositions) || mesh.morph.streams == 0)
    {
        return;
    }
    const MorphBlend blend = morphBlendAt(seconds, mesh.morph);
    const size_t streamBytes = size_t(mesh.numVerts) * mesh.morph.stride;
    const uint8_t *from = data.data() + mesh.morph.firstStreamOfs + blend.from * streamBytes;
    const uint8_t *to = data.data() + mesh.morph.firstStreamOfs + blend.to * streamBytes;
    for (uint32_t v = 0; v < mesh.numVerts; ++v)
    {
        float fromValues[3], toValues[3];
        std::memcpy(fromValues, from + size_t(v) * mesh.morph.stride, sizeof fromValues);
        std::memcpy(toValues, to + size_t(v) * mesh.morph.stride, sizeof toValues);
        for (int axis = 0; axis < 3; ++axis)
        {
            out[v].pos[axis] = (1.f - blend.weight) * fromValues[axis] + blend.weight * toValues[axis];
        }
    }
}

} // namespace argon
