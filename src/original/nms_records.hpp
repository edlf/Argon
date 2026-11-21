// The game's .nms movie sequences byte for byte (seq_start_copy 0x96230). File order: header, objects, the type-8
// objects' embedded CMS animations (cmm_records.hpp CmsHeader, its track records and keys; in object order), the
// stored curves of each object (curve header + keys), a track count, then per track the track record and its
// segments. Name prefixes as in cmm_records.hpp. Field meanings: docs\sequences\README.md.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

constexpr int kNmsCurveSlots = 10;

#pragma pack(push, 1)

struct NmsHeader
{
    char trackName[0x100];
    char trackFolder[0x104];
    int32_t objectCount;
    uint8_t unknown208[8];
    float lightDirection[3];      // the scene's light 1 (seq_start_copy 0x96308 -> 0xa7ea0), checked by 0xb4f80
    uint32_t lightColour;         // D3DCOLOR
    uint8_t unknown220[0xff0];
};

struct NmsObject
{
    char name[0x40];
    int32_t type;                 // 0 group, 1 scene stage, 2 / 10 model, 4 camera, 5 interest, 6 position, 7 master
                                  // dopesheet, 8 actor, 9 speech
    uint32_t unknown44;
    int32_t partner;              // a camera position's / interest's partner
    int32_t resourceIndex;
    int32_t parent, lastChild, previousSibling, nextSibling;   // -1 when out of range
    uint8_t unknown60[0x100];
    char fileName[0x100];         // types 2 / 9 / 10
    uint8_t runtime260[0x110];
    float scale[3];
    float rotationDegrees[3];
    float translation[3];
    uint32_t runtimeModel;        // type 8: its embedded animation
    float fovDegrees;
    uint8_t unknown39c[0x10];
    float runtimeLength;          // the last key time, at least 0.1 (types 8 / 9: as stored)
    float runtimeCurveTime;       // the time its curves were last taken at (a camera's: its cut's)
    uint8_t unknown3b4[0xc];
    uint32_t curveStored[kNmsCurveSlots];   // non-zero when the slot's curve follows; slots 0..8 drive scale ..
                                            // translation, 9 the fov
    uint8_t unknown3e8[0x408];
};

struct NmsCurveHeader
{
    uint32_t unknown00;
    int32_t keyCount;
    uint32_t unknown08;
    uint32_t staleKeys;
    float runtimeFirstKeyTime, runtimeLastKeyTime;
    uint8_t unknown18[8];
};

struct NmsKey
{
    float time, value, weight;
    uint32_t unused;
};

struct NmsTrack
{
    uint32_t unknown00;           // the 4 bytes before each track
    char name[0x40];
    int32_t segmentCount;
    uint32_t unknown48[2];
    float length;
    int32_t kind;                 // 1 cameras, 2 actor animations, 3 speech
    uint32_t staleSegments;
    int32_t object;
    uint8_t unknown60[0x44];
};

struct NmsSegment
{
    float start, end;
    int32_t object;               // -1 none
    float sourceStart, sourceEnd;
    uint32_t flags;               // actor tracks: 1 loops, 2 turns the head towards lookAt (0x99360)
    int32_t lookAt;               // an object
    float lookStrength;           // the head's share of the turn (0x1f0288)
};

#pragma pack(pop)

static_assert(sizeof(NmsHeader) == 0x1210 && offsetof(NmsHeader, objectCount) == 0x204);
static_assert(offsetof(NmsHeader, lightDirection) == 0x210 && offsetof(NmsHeader, lightColour) == 0x21c);
static_assert(sizeof(NmsObject) == 0x7f0 && offsetof(NmsObject, type) == 0x40 && offsetof(NmsObject, parent) == 0x50);
static_assert(offsetof(NmsObject, partner) == 0x48 && offsetof(NmsObject, fileName) == 0x160);
static_assert(offsetof(NmsObject, scale) == 0x370 && offsetof(NmsObject, runtimeModel) == 0x394);
static_assert(offsetof(NmsObject, fovDegrees) == 0x398 && offsetof(NmsObject, runtimeLength) == 0x3ac);
static_assert(offsetof(NmsObject, curveStored) == 0x3c0 && offsetof(NmsObject, runtimeCurveTime) == 0x3b0);
static_assert(sizeof(NmsCurveHeader) == 0x20 && offsetof(NmsCurveHeader, keyCount) == 4 && sizeof(NmsKey) == 16);
static_assert(offsetof(NmsCurveHeader, runtimeFirstKeyTime) == 0x10);
static_assert(sizeof(NmsTrack) == 0xa4 && offsetof(NmsTrack, name) == 0x04 && offsetof(NmsTrack, segmentCount) == 0x44);
static_assert(offsetof(NmsTrack, length) == 0x50 && offsetof(NmsTrack, kind) == 0x54 && offsetof(NmsTrack, object) == 0x5c);
static_assert(sizeof(NmsSegment) == 0x20 && offsetof(NmsSegment, object) == 0x08 && offsetof(NmsSegment, sourceEnd) == 0x10);

} // namespace argon::original
