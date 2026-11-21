// What a collision triangle's material tells the craft: the table collision_build_from_cmm (0xd2a80) builds, 12 bytes
// a mesh (collision +0x1428), kept here per triangle.
#pragma once
#include <cstdint>

namespace argon
{

struct TriangleSurface
{
    uint8_t type = 0;           // +0: the material's +0x14: 0 track, 1 rough, 2 loose, 3 water, 4 slippery
    bool resetsCraft = false;   // +4: the material's +0x18 == 1: a hover ray or hull contact on it resets the craft
    // the D3DCOLOR (0xAARRGGBB) of the vertex the hover reads (0x5f0d7 -> 0xa46a0): a strip triangle's third index,
    // a listed triangle's first; the craft's light is made from it (craftsim.hpp GameCraft::groundColour)
    uint32_t colour = 0;
};

} // namespace argon
