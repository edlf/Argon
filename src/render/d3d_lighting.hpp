// Argon: the game's fixed-function D3D lighting of a cmm mesh (docs\models\surface-block.md "Surface block").
//
// The device keeps one material (0xa6ad7: diffuse = ambient = specular = 1, emissive 0, power 4) and D3D's
// default material sources (0x117628 table: diffuse = vertex colour, specular = colour 2, ambient and emissive = the
// material); specular is off (0xa6997). So a lit vertex is clamp(D3DRS_AMBIENT + vertex colour x light 0 x N.L),
// alpha the vertex alpha. Per mesh, mesh_apply_surface_state (0xba4f0) and the surface setups 0xba370 / 0xba1d0 pick
// lighting on or off and may replace the ambient or light 0 for that draw; cmm_draw_mesh puts them back after it
// (0xbae10, lighting on again: 0xa8700(1)).
#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace argon
{

enum class LightSource : uint8_t
{
    Scene,   // the track's (prop.dat) or the craft's (craftLights)
    Own,     // the surface lighting block's colour x the bone's intensity (+0x138) x tint (+0x104)
    Black
};

struct SurfaceLighting
{
    bool lit = false;
    LightSource ambient = LightSource::Scene;
    LightSource light0 = LightSource::Scene;
};

struct SurfaceSetup
{
    bool hasBlock = false;    // mesh flag 0x20
    uint32_t setup = 0;       // block +0x04
    uint32_t flags = 0;       // block +0x0c
    bool hasBoneRecord = false;   // mesh flag 0x100 on a bone: 0xba4f0's third argument (cmm_draw_mesh 0xb720f)
};

// lighting on: setup 2 unless flag 0x20 (0xba1d0); setup 1 with flag 0x8000, which leaves the device's lighting as
// cmm_draw_mesh left it after the mesh before, on (0xba49c); setup 1 without it off (every mesh of the game's .cmm
// files has a surface block; without one the port leaves it unlit). Flag 0x2000 (0xba5a7): with 0x100
// the ambient and light 0 black, else light 0 black unless 0x200. Then flag 4 with a bone record (0xba370 / 0xba1d0):
// the own colour becomes the ambient (0xa8410), with 0x100 light 0 (0xa82d0).
SurfaceLighting surfaceLighting(const SurfaceSetup &surface);

struct SceneLights
{
    glm::vec3 ambient{0.f}, light0{0.f};   // 0..1
};

// a craft's lights (0x59bc0, the ship draw): per channel ambient = (ground / 2 + ambient) / 2, light 0 =
// min((ground + sun) / 2, sun); the blue of light 0 is the green sum's (min((ground g + sun g) / 2, sun b)), as the game
// has it. ground: GameCraft::groundColour (D3DCOLOR), track: the prop.dat ambient and sun.
SceneLights craftLights(uint32_t ground, const SceneLights &track);

} // namespace argon
