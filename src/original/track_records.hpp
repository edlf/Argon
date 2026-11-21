// The game's binary track files (data\tracks\<t>\*.dat) byte for byte. Each file is a header then an array of
// records. Name prefixes as in cmm_records.hpp; "unread": holds data in the files, but nothing in the XBE reads it
// (no reference to the field). Field meanings: docs\tracks\README.md, docs\water\README.md.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

#pragma pack(push, 1)

// locator.dat (locator_dat_fixup 0x000af270): the start grid, player_start0..8
struct LocatorHeader
{
    uint32_t count;
    uint32_t staleLocators;
    uint32_t unused08[2];
};

struct Locator
{
    char name[0x40];
    float world[16];              // D3D row-vector; must match position / rotation / scale
    uint32_t unused80;
    float position[3];
    float rotationDegrees[3];     // matrix = Rx Ry Rz
    float scale[3];               // 1 in every file
    uint8_t unusedA8[0x18];
};

// sprite.dat (sprite_dat_fixup 0x000af320 -> quadsprite_AddCMMSprites 0x000bdf40): glow / flare quads
struct QuadSpriteHeader
{
    uint32_t count;
    uint32_t staleSprites;
    uint32_t unused08[4];
};

struct QuadSprite
{
    int16_t frame;                // facing (x 3 = quadsprite.xvu's a0): 0 world, 1 upright, 2 camera (sprite_pool.hpp)
    uint16_t unused02;
    uint32_t flags;               // bit 0 additive (else alpha blend); bit 1 set in many files but not read
    float position[3];
    float cornerOffsets[4][3];    // corners v3, v2, v0, v1
    uint32_t cornerColours[4];    // D3DCOLOR v0..v3
    float cornerUvs[8];           // v0 (u +0x54, v +0x70), v1 (+0x5c, +0x68), v2 (+0x6c, +0x58), v3 (+0x64, +0x60)
    uint8_t unused74[0x10];
};

// water.dat: boxes whose top face is the water surface
struct WaterHeader
{
    uint32_t count;
    uint32_t unused04;
    float waveAmplitude;          // vertex shader c4.y
    uint32_t unused0c[4];
};

struct WaterBox
{
    uint32_t flags;               // bit 0 physics only (no surface drawn)
    uint32_t cubeMap;             // cube slot: 0 default, 1 waterref1, 2 waterref2 (3..7 custom1..5)
    uint32_t unused08[2];
    float min[3];
    float max[3];
};

// prop.dat (track_load_prop_dat 0x000b03f0): fog and lighting. The game copies 0x1178 bytes of it to the global
// 0x390fa8, which is only read for the fog, ambient and sun fields.
struct TrackProperties
{
    int32_t fogColour[3];         // 0..255
    float fogDensity;
    int32_t unreadLightColour[3]; // reset to 0x40 with the ambient by 0xaf360
    int32_t unread1c;             // 1 in every file
    int32_t unread20;             // 1 / 0
    char trackName[0x40];
    uint8_t unused64[0xc0];
    int32_t unread124;            // 1 in every file
    int32_t unread128;            // 1 / 0
    uint32_t unused12c;
    int32_t unread130;            // 1 in every file
    int32_t unread134;            // 1 in every file
    uint32_t unused138;
    int32_t unread13c;            // 0 / 20 / 50
    int32_t unread140;            // 500..40000, a distance?
    int32_t unread144;            // 60 / 70, a field of view?
    float unreadWaveAmplitude;    // the same values as water.dat's WaterHeader::waveAmplitude
    int32_t ambient[3];           // 0..255
    int32_t sunColour[3];         // 0..255
    float sunVector[3];           // normalised by the game
    int32_t fogMode;
    float fogRange;
    uint8_t unused178[0xe88];     // the files are 4096 bytes
};

// visdata.vis (track loader 0xafa66, race track draw 0xafcf0): the sectors, then per sector its mesh indices (u32, into
// model.cmm's meshes) followed by its neighbour sector indices (u32)
struct VisHeader
{
    uint32_t count;
    uint32_t staleSectors;
    uint8_t unused08[0x10];       // zeros or leftovers
};

struct VisSector
{
    float position[3];
    uint32_t neighbourCount;
    uint32_t meshCount;
    uint32_t staleMeshes;
    uint32_t staleNeighbours;
};

#pragma pack(pop)

static_assert(sizeof(VisHeader) == 0x18 && sizeof(VisSector) == 0x1c);
static_assert(offsetof(VisSector, neighbourCount) == 0xc && offsetof(VisSector, meshCount) == 0x10);
static_assert(sizeof(LocatorHeader) == 0x10);
static_assert(sizeof(Locator) == 0xc0 && offsetof(Locator, world) == 0x40 && offsetof(Locator, position) == 0x84);
static_assert(offsetof(Locator, rotationDegrees) == 0x90 && offsetof(Locator, scale) == 0x9c);
static_assert(sizeof(QuadSpriteHeader) == 0x18);
static_assert(sizeof(QuadSprite) == 0x84 && offsetof(QuadSprite, cornerColours) == 0x44 && offsetof(QuadSprite, cornerUvs) == 0x54);
static_assert(sizeof(WaterHeader) == 0x1c && sizeof(WaterBox) == 40);
static_assert(sizeof(TrackProperties) == 0x1000 && offsetof(TrackProperties, trackName) == 0x24);
static_assert(offsetof(TrackProperties, unread124) == 0x124 && offsetof(TrackProperties, unreadWaveAmplitude) == 0x148);
static_assert(offsetof(TrackProperties, ambient) == 0x14c && offsetof(TrackProperties, sunVector) == 0x164);
static_assert(offsetof(TrackProperties, fogRange) == 0x174);

} // namespace argon::original
