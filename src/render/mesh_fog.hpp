// Argon: a mesh's fog as the surface setup sets it (0xba1d0 at 0xba2ac / 0xba370 at 0xba44c, then the fog setter
// 0xa8450; docs\models\surface-block.md "Surface block"). The final combiner then mixes every mesh towards the fog colour,
// fog.a x r0 + (1 - fog.a) x fog colour, additive ones too (read live: tools\xemu\combiners.py).
#pragma once
#include <cstdint>

#include "original/cmm_records.hpp"

namespace argon::fog
{

// the D3D fog table modes 0xa8450 sets: 0 off, mode 2 becomes EXP as mode 1 does, 3 LINEAR
enum class Curve : int32_t
{
    Off = 0,
    Exp = 1,
    Linear = 3
};

enum class Source : int32_t
{
    Track,   // the track's fog (prop.dat: 0x298da8 colour, +4 density, +8 range, +0xc mode)
    Own      // the mesh's lighting block (surface flag 0x40)
};

struct MeshFog
{
    Source source = Source::Track;
    Curve curve = Curve::Exp;     // with Source::Own; the track's comes from its own mode
    uint32_t colour = 0;          // D3DCOLOR, with Source::Own
    float density = 0.f, range = 0.f;
};

// 0xa8450's mode test: above 3 the call changes nothing (the last fog stays); kept as off here
constexpr Curve curveOf(uint32_t mode)
{
    if (mode == 1 || mode == 2)
    {
        return Curve::Exp;
    }
    return mode == 3 ? Curve::Linear : Curve::Off;
}

// surface flag 0x80: fog off; 0x40: the lighting block's mode, colour, density and range; else the track's. A mesh
// without a surface block keeps the track's (the setup is not run for it)
constexpr MeshFog meshFog(uint32_t surfaceFlags, uint32_t mode, uint32_t colour, float density, float range)
{
    if (surfaceFlags & original::surface_flags::kNoFog)
    {
        return MeshFog{Source::Own, Curve::Off, 0, 0.f, 0.f};
    }
    if (surfaceFlags & original::surface_flags::kOwnFog)
    {
        return MeshFog{Source::Own, curveOf(mode), colour, density, range};
    }
    return MeshFog{};
}

}  // namespace argon::fog
