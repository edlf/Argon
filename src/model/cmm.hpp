// Quantum Redshift .cmm model reader (C++ port of tools/cmm/cmm.py).
// See docs/models/README.md for the layout.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "files/game_path.hpp"   // readFile, for the model's callers
#include "model/bone_aim.hpp"
#include "original/cmm_records.hpp"

namespace argon
{

struct Vertex
{        // converted to right-handed (z mirrored)
    float pos[3];
    float nrm[3];
    float uv[4][2];    // UV sets 0..3 (missing sets repeat the last one)
    uint8_t rgba[4];   // vertex colour (D3DCOLOR for vtype 0..3, white otherwise)
};

// One texture stage of a CmmMaterial (0x34 bytes at material +0x0c + s*0x34).
// Read by mesh_apply_material_stages (XBE 0x000ba660).
struct CmmStage
{
    uint32_t flags = 0;    // 1 uv scroll, 8 env map (reflection texcoords), 0x40 clamp U, 0x80 clamp V,
                           // 0x100 swap args (CURRENT op TEXTURE instead of TEXTURE op CURRENT)
    int tex = -1;          // texture index (same as the mesh's list)
    uint32_t texcoord = 0; // UV set
    uint32_t colorCode = 0, alphaCode = 0;   // game combine codes (see stageOp())
    float rate = 60.f;     // animation frames per second
    float scrollU = 0, scrollV = 0, offU = 0, offV = 0;
};

// Morph streams (mesh flag 2, CmmMorphHeader): keyframes of the mesh's vertices, drawn by the "Keyframe ..." effects
// (0x7ac80..0x7b7e0; docs\models\meshes.md "Morph meshes"). Only objects\explodefirelarge has them, and the game never loads it.
struct CmmMorph
{
    uint32_t kind = 0;             // 1 positions, 3 + normals, 5 + colours, 7 all three, 4 colours only
    uint32_t streams = 0;          // keyframes
    float streamsPerSecond = 0.f;  // header +0x00
    uint32_t stride = 0;           // bytes per vertex in a stream
    size_t firstStreamOfs = 0;     // the streams follow each other
};

struct CmmMesh
{
    int index = 0;
    int bone = -1;              // -1 = static world geometry
    uint32_t numVerts = 0, flags = 0, numFaces = 0;
    int vtype = 0;
    std::vector<int16_t> tex;   // texture index per stage
    uint32_t blendDesc = 0;     // +0x08 of the surface block: packed blend state (see docs\models\README.md)
    uint32_t surfaceFlags = 0;  // CmmSurfaceHeader::flags (original::surface_flags)
    uint32_t surfaceSetup = 0;  // +0x04: 1 / 2 = extra per-mesh setup (0xba370 / 0xba1d0)
    uint32_t surfaceColour = 0; // CmmSurfaceLighting::colour; with flag 4 it becomes the scene ambient (0xa8410)
    uint32_t surfaceType = 0;   // +0x14: 0 track, 1 rough, 2 loose, 3 water, 4 slippery (craft friction tables)
    uint32_t surfaceReset = 0;  // +0x18: 1 = a craft touching it is put back on the track (0xd2a80 table +4)
    // surface flag 0x40000 (flares): occlusion test before the draw (0xb6e60 -> 0xa64f0), N points in a sphere of
    // this radius (CmmSurfaceLighting, 0xbb240 / 0xbb210)
    float occlRadius = 0.f;
    uint32_t occlPoints = 0;
    float billboardFactor = 1.f;   // surface flag 0x100000 (CmmSurfaceLighting, 0xbb270; render\camera_facing.hpp)
    // surface flag 0x40 (own fog): the lighting block's fog for 0xa8450 instead of the track's (0xba2bd / 0xba45d)
    uint32_t fogMode = 0, fogColour = 0;
    float fogDensity = 0.f, fogRange = 0.f;
    bool hasSurface = false;    // mesh flag 0x20
    bool collidable = false;    // surfaceFlags & 2
    bool drawn = true;          // surfaceFlags & 1: without it the race skips the mesh (0xb6e60 at 0xb6f71)
    bool skinned = false;       // flag 0x40000: weighted vertices (skinned on load in the rest pose)
    std::vector<int> skinBones; // skin header +0x08: the bones the vertices are weighted to (vtype = count + 6)
    uint32_t skinStride = 0;    // per vertex: count x pos (bone space), normal, D3DCOLOR, uv, count x weight
    int material = -1;          // +0x48 CmmMaterial index
    float sortPoint[3]{};       // +0x4c: with flag 0x80 the point the race sorts it by (eye distance, 0xafcf0)
    int effect = -1;            // material +0xe0: index of the model's effect record (-1 = fixed function)
    int cubeIndex = 0;          // +0xa8: environment cube slot for the specular-bump effects (argon::kEnvCubeSlots)
    CmmMorph morph;             // flag 2
    size_t vertOfs = 0, idxOfs = 0;
};

struct CmmTexInfo
{
    uint32_t type = 0, frames = 1;
};   // texture table: type 3 bump, 4/6/8 animated

struct CmmBone
{
    std::string name;
    float local[16];   // D3D row-vector matrix as stored
    int parent = -1;
    int track = -1;            // +0x100: s16 index of its animation track (header +0x40 records), -1 = none
    float scale[3]{1, 1, 1};   // +0x11c: scales the sampled rotation's rows (0xb8630)
};

// Animation track (header +0x40/+0x44: records of 0x80 bytes, after the effect records and their programs; see
// docs\models\meshes.md "Animation"): +0x04 flags (1 position keys, 4 rotation keys, 2 scale keys), +0x08 key count, +0x0c keys
// per second, +0x10 the bone's name. Keys follow all records, per record: positions (12 bytes each), rotations
// (16: Euler x, y, z degrees, turned into quaternions at load by 0xb6070), scales (12).
struct CmmTrack
{
    uint32_t flags = 0, keys = 0;
    float fps = 20.f;
    std::string name;
    std::vector<float> pos, quat, scl;   // 3 / 4 (x, y, z, w) / 3 floats per key
};

// what drives one bone when posing (0xb8630): a track sampled at its own clock, looping or played once (bone flag
// 0x400); no track: the bone's stored local
struct BoneKeys
{
    const CmmTrack *track = nullptr;
    float seconds = 0.f;
    bool once = false;
};

// a pose from other tracks and aims (a story scene's actor): per bone, in bone order
struct PoseDrivers
{
    std::vector<BoneKeys> keys;
    std::vector<BoneAim> aims;
};

struct Cmm
{
    std::vector<uint8_t> data;
    uint32_t version = 0, numTextures = 0, flags = 0;
    std::vector<CmmMesh> meshes;
    std::vector<CmmBone> bones;
    std::vector<CmmTexInfo> texInfo;     // per texture (defaults if the file has no table)
    std::vector<CmmTrack> tracks;        // animation tracks (header +0x40)
    std::vector<std::string> effects;    // effect record names (header +0x30 records of 0x5bc after the texture table)
    size_t materialsOfs = 0;
    int numMaterials = 0;

    // up to 4 stages for a mesh (empty if the mesh has no valid material)
    std::vector<CmmStage> stages(const CmmMesh &mesh) const;
    original::CmmMaterial materialRecord(int index) const;

    void load(const std::string &path);   // throws when it cannot be read or is not a CMM the loader knows
    // geometry of one mesh, right-handed; indices are a triangle list
    void geometry(const CmmMesh &mesh, std::vector<Vertex> &verts, std::vector<uint32_t> &idx) const;
    // bone world matrix (local chain), right-handed, column-major (glm layout)
    void boneWorld(int bone, float out[16]) const;
    // animated: some bone has a track
    bool animated() const;
    // every bone's world matrix (column-major, glm layout) at time t seconds (0xb8630: keys sampled at t x fps,
    // each track looping on its own key count, linear / slerp between neighbouring keys)
    void pose(float seconds, std::vector<float> &world16) const;
    // the same with each bone driven as given (per bone, in bone order; missing or empty: the stored local)
    void pose(const std::vector<BoneKeys> &drivers, std::vector<float> &world16) const;
    // ... and each bone with an aim turned towards its point after its world (0xb8630, flags 0x2000 / 0x4000)
    void pose(const std::vector<BoneKeys> &drivers, const std::vector<BoneAim> &aims, std::vector<float> &world16) const;
    std::vector<BoneKeys> ownTracks(float t) const;   // the model's own tracks, looping, at t
    void ownTracks(float seconds, std::vector<BoneKeys> &drivers) const;
    // a skinned mesh's vertices for bone worlds (pose() layout); normals renormalised
    void skin(const CmmMesh &mesh, const float *world16, Vertex *out) const;
    // a morph mesh's positions at t seconds (cmm_morph.hpp); the other vertex fields are left as they are
    void morph(const CmmMesh &mesh, float seconds, Vertex *out) const;
};

} // namespace argon
