// The game's .cmm model records (version 300) byte for byte, as cmm_fixup_pointers (XBE 0x000b6070) walks them.
// The file is a raw dump of the game's in-memory structs. Field name prefixes:
//   stale    a pointer from the tool that wrote the file; the loader rebuilds it, the file's value is never read
//   runtime  written by the game at load or run time; the file's value is never read
//   unused   zero in every file on the disc and never read by the loader
//   unknown  holds data in the files, reader not found yet
//   unread   holds data in the files, and nothing in the game reads it
// Layout, file order and field meanings: docs\models\README.md. Ghidra project neon_stock has the same types under /QR
// (tools\cmm\ghidra\label_cmm.py). tools\cmm\record_survey.py lists every field's values over the disc.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

constexpr uint32_t kCmmVersion = 300;

namespace cmm_flags
{
constexpr uint32_t kHasTextureTable = 0x1000;   // CmmHeader::flags
constexpr uint32_t kRuntimeModelClock = 0x80000;   // meshes animate on the model's clock (model +0x5e4)
} // namespace cmm_flags

namespace mesh_flags
{
constexpr uint32_t kTriangleStrip = 0x1;
constexpr uint32_t kMorph = 0x2;
constexpr uint32_t kHasSurfaceBlock = 0x20;
constexpr uint32_t kSortPoint = 0x80;
constexpr uint32_t kUsesBoneState = 0x100;      // its bone record's hidden flag, clock and intensity apply
constexpr uint32_t kWeightedNormals = 0x800;
constexpr uint32_t kPrebuiltPushBuffer = 0x8000;   // drawn by running CmmMesh::runtimePushBuffer
constexpr uint32_t kHidden = 0x20000;
constexpr uint32_t kSkinned = 0x40000;
constexpr uint32_t kRuntimePushBufferable = 0x200000;   // set at load: static, no UV animation
constexpr uint32_t kIdentityTransform = 0x44000000;    // drawn with an identity world matrix (0xb7920)
} // namespace mesh_flags

namespace surface_flags
{
constexpr uint32_t kDrawn = 0x1;
constexpr uint32_t kCollidable = 0x2;
constexpr uint32_t kSceneAmbient = 0x4;         // the lighting colour becomes the ambient (0x100: light 0 instead)
constexpr uint32_t kNoDepthWrite = 0x8;
constexpr uint32_t kTwoSided = 0x10;
constexpr uint32_t kOwnFog = 0x40;              // CmmSurfaceLighting's fog instead of the track's
constexpr uint32_t kNoFog = 0x80;
constexpr uint32_t kNoDepthTest = 0x10000;
constexpr uint32_t kLaidOver = 0x20000;         // sorted with key 0, last in its bucket (0xb02d1, 0xb7efb): Draco's sand
constexpr uint32_t kOcclusionTested = 0x40000;
constexpr uint32_t kParentIntensity = 0x80000;
constexpr uint32_t kFollowsCamera = 0x400;      // its world position is an offset from the eye (0xb7e88)
constexpr uint32_t kFacesCamera = 0x800;        // turned to the eye (0xb7eca / 0xb78a1 -> 0xb4e70)
constexpr uint32_t kBillboard = 0x100000;       // slid towards the view axis by its factor (0xb7765, 0xbb270)
constexpr uint32_t kSortedTranslucent = 0x800000;   // render list bucket 3, sorted by distance (0xb7920)
constexpr uint32_t kShadowHull = 0x400000;      // the one mesh the ship shadow pass draws (0xb6ea8, 0xbe230)
constexpr uint32_t kAlphaTest = 0x2000000;
} // namespace surface_flags

namespace bone_flags
{   // CmmBone::flags, rebuilt by neon_model_load_skeleton (0xb9540); the file's value is lost
constexpr uint16_t kHidden = 0x1;
constexpr uint16_t kDetached = 0x2;             // keeps its own world matrix (a launched weapon)
constexpr uint16_t kHasNextSibling = 0x8;
constexpr uint16_t kIdentityLocal = 0x40;
constexpr uint16_t kOwnClock = 0x80;            // animates on CmmBone::clock
constexpr uint16_t kHasChildren = 0x100;
constexpr uint16_t kCopiesBone = 0x800;         // copies the matrices of bone CmmBone::copiedBone
constexpr uint16_t kForeignTracks = 0x1000;     // animation track from CmmBone::runtimeTrackSource
} // namespace bone_flags

#pragma pack(push, 1)

struct CmmHeader
{
    uint32_t version;
    uint32_t meshCount;
    uint32_t textureCount;        // tex<i>.dds / lotex<i>.dds
    uint32_t flags;               // cmm_flags (0x10: a track without visdata.vis, 0xafa58; also set in about half the other files)
    uint32_t unused10;
    uint32_t staleMeshes;
    int32_t materialCount;
    uint32_t staleMaterials;
    int32_t boneCount;
    uint32_t staleBones;
    int32_t rootBone;             // a bone without parent, where the animation starts (0xb9210); -1 without bones
    uint32_t textureTable;        // only tested for non-zero
    uint32_t effectCount;
    uint32_t staleEffects;
    uint32_t unused38[2];
    int32_t animationTrackCount;
    uint32_t staleAnimationTracks;
};

struct CmmMesh
{
    int32_t bone;                 // CmmBone::index of its bone; -1 = static world geometry
    uint32_t uvSetCount;          // the vertex type's UV pairs
    uint32_t vertexCount;
    uint32_t flags;               // mesh_flags
    uint32_t runtimeNextOnBone, runtimePreviousOnBone;   // the bone's mesh list
    uint32_t staleVertices;
    uint32_t staleMorphOrSkin;
    uint32_t staleSurfaceBlock;
    uint32_t runtimeIndexBuffer;
    uint32_t faceCount;
    uint32_t staleIndices;
    uint32_t unused30;
    uint32_t stageCount;
    int16_t textures[4];          // texture index per stage
    int32_t runtimeVertexShader;  // -1 effect shader built, 0 = its effect is unavailable (not drawn)
    uint32_t runtimePushBuffer;
    int16_t material;
    int16_t unused4a;
    float sortPoint[3];           // with kSortPoint: the point the race sorts the see-through mesh by
    uint32_t runtime58;
    uint32_t runtimeBounds;       // its entry in the model's vertex bounds (0xb9270)
    uint32_t runtimeFixedFunctionSetup;
    uint32_t unknown64;           // pointer-like values in the track models
    uint8_t unused68[0x40];
    uint8_t cubeMap;              // specular-bump effects: cube slot 0 default, 1 waterref1, 2 waterref2, 3..7 custom
    uint8_t unusedA9[2];
    uint8_t lodBand;              // 0xb7920 draws it: 0 always; 1 / 2 / 3 when its bone is nearer than the model's
                                  // first / between its first and second / second and third LOD distance (model
                                  // +0x5ec / +0x5f0 / +0x5f4); 4 within [lodNear, lodFar)
    float lodNear;
    float lodFar;                 // 600 on the power-ups
    int16_t runtimeIndex;
    uint8_t unusedB6[0x0a];
    int16_t unknownC0;            // -1 or an index (up to 240)
    int16_t vertexType;           // 0..7, skinned: bone count + 6
    uint32_t unusedC4;
};

struct CmmStage
{
    uint32_t flags;               // 1 uv scroll, 8 env map, 0x40 clamp U, 0x80 clamp V, 0x100 args swapped
    uint32_t unused04;
    int32_t texture;              // not read: the game uses CmmMesh::textures
    uint32_t texCoordIndex;       // the stage's own number in every file; set for stages 2 / 3 and env maps only
    uint32_t colourOp;            // game combine code (tss_color_op 0xa8cf0)
    uint32_t alphaOp;
    float framesPerSecond;        // copied to the texture slot as it is (0xba716): 0 = frame 0
    float scrollU, scrollV;
    float offsetU, offsetV;
    float unread2c[2];            // small values in 23 materials (Kalau, Mount, Kutoo's craft); no reader
};

struct CmmMaterial
{
    uint32_t stageCount;          // not read: CmmMesh::stageCount is used
    uint32_t unused04;
    int32_t unread08;             // -1 in every file, not read
    CmmStage stages[4];
    int32_t unreadDc;             // -1 in every file, not read
    int32_t effect;               // effect record index, -1 = fixed function
    uint32_t unusedE4[2];
};

struct CmmBone
{
    float local[16];              // D3D row-vector, translation in row 3
    float world[16];              // local x parent world (0xb8630); saved as last computed, 0 on some bones
    float hierarchyWorld[16];     // local x the parents' hierarchyWorld; "world as saved" in older notes
    float runtimeMatrixC0[16];
    int16_t animationTrack;       // -1 = none
    uint16_t flags;               // bone_flags
    uint8_t tint[4];              // runtime D3DCOLOR, 0xffffffff at load
    uint32_t occlusionQuery;      // runtime, 999999 = no test yet
    uint32_t runtimeTintedColour; // tint x intensity, for the occlusion-tested meshes
    float unknown110[3];          // tiny values (~1e-6)
    float rowScale[3];
    uint32_t runtimeFirstMesh;
    int32_t meshCount;
    uint32_t runtime130;
    int32_t index;                // the bone's own index; CmmMesh::bone refers to it
    float intensity;              // runtime, 1.0 at load, set by the occlusion test
    uint32_t runtime13c;
    char name[0x30];
    uint8_t unused170[0x10];
    int16_t parent, firstChild, nextSibling, previousSibling;
    float clock;                  // runtime per-bone clock (kOwnClock)
    int16_t runtimeAnimationTrack;   // copy of animationTrack made at load
    uint16_t unused18e;
    int32_t copiedBone;           // kCopiesBone
    uint32_t runtimeTrackSource;  // kForeignTracks
    uint8_t unused198[8];
};

// Every surface block (one per mesh with kHasSurfaceBlock) starts with this; `size` is the block's length.
struct CmmSurfaceHeader
{
    uint32_t size;                // 0x60 setup 1 (track models), 0x30 setup 2
    uint32_t setup;               // 1 / 2: extra per-mesh setup (0xba370 / 0xba1d0)
    uint32_t blend;               // packed blend descriptor
    uint32_t flags;               // surface_flags
};

// collision_build_from_cmm (0xd29c0) copies +0x14 / +0x18 of every block; in a setup 2 block those bytes are the
// lighting's fog colour and density.
struct CmmSurfaceCollision
{
    uint32_t unknown10;           // 0xd in almost every block
    uint32_t surfaceType;         // 0 track, 1 rough, 2 loose, 3 water, 4 slippery
    uint32_t craftReset;          // 1: a craft touching it is put back on the track (TriangleSurface)
    uint32_t unknown1c;           // 0, 3..6
};

// Read by the setup functions 0xba370 / 0xba1d0 and cmm_draw_mesh (0xb6e60).
struct CmmSurfaceLighting
{
    uint32_t colour;              // D3DCOLOR: with kSceneAmbient the scene ambient (or light 0)
    uint32_t fogColour;           // kOwnFog: D3DCOLOR, density and range for neon_setup_fog
    float fogDensity;
    float occlusionRadius;        // kOcclusionTested: radius and point count of the visibility test
    uint32_t occlusionPointCount;
    float billboardFactor;        // kBillboard: the share of its offset across the view axis it keeps (0xbb270)
    uint32_t fogMode;             // kOwnFog: the fog setter 0xa8450's mode (0 off, 1 / 2 exp, 3 linear)
    float fogRange;
};

struct CmmSurfaceBlock1
{
    CmmSurfaceHeader header;
    CmmSurfaceCollision collision;
    uint8_t unused20[0x20];
    CmmSurfaceLighting lighting;
};

struct CmmSurfaceBlock2
{
    CmmSurfaceHeader header;
    CmmSurfaceLighting lighting;
};

constexpr size_t lightingOffset(uint32_t setup)
{
    return setup == 1 ? offsetof(CmmSurfaceBlock1, lighting) : setup == 2 ? offsetof(CmmSurfaceBlock2, lighting) : 0;
}

// A mesh's vertices without skin (vertex types 0..7) are one of these, followed by the type's 1..4 UV pairs.
struct CmmUvPair
{
    float u, v;
};

// vertex types 0..3
struct CmmPlainVertex
{
    float position[3];
    float normal[3];
    uint32_t colour;              // D3DCOLOR 0xAARRGGBB
};

// vertex types 4..7: no normal (the viewer builds one from the triangles)
struct CmmPackedVertex
{
    float position[3];
    uint32_t colour;              // D3DCOLOR 0xAARRGGBB, prelit colour and alpha
};

constexpr bool isPackedVertexType(int vertexType)
{
    return vertexType >= 4;
}

constexpr int uvPairCount(int vertexType)
{
    return (vertexType & 3) + 1;
}

constexpr size_t vertexSize(int vertexType)
{
    const size_t head = isPackedVertexType(vertexType) ? sizeof(CmmPackedVertex) : sizeof(CmmPlainVertex);
    return head + size_t(uvPairCount(vertexType)) * sizeof(CmmUvPair);
}

// A skinned vertex (kSkinned): boneCount positions (each in its bone's space), then this, then boneCount weights.
struct CmmSkinnedVertexShared
{
    float normal[3];
    uint32_t colour;              // D3DCOLOR 0xAARRGGBB
    CmmUvPair uv;
};

// what each of the vertex's bones adds: a position before CmmSkinnedVertexShared, a weight after it
struct CmmSkinnedVertexBone
{
    float position[3];
    float weight;
};

constexpr size_t skinnedSharedOffset(size_t boneCount)
{
    return boneCount * sizeof(CmmSkinnedVertexBone::position);
}

constexpr size_t skinnedWeightsOffset(size_t boneCount)
{
    return skinnedSharedOffset(boneCount) + sizeof(CmmSkinnedVertexShared);
}

constexpr size_t skinnedVertexSize(size_t boneCount)
{
    return sizeof(CmmSkinnedVertexShared) + boneCount * sizeof(CmmSkinnedVertexBone);
}

struct CmmMorphHeader
{
    float streamsPerSecond;       // keyframes a second (0x7b7e0); 3.0 in the one morphing model (explodefirelarge)
    uint32_t kind;
    uint32_t streamCount;
    uint32_t staleStreams;        // streamCount x 8 bytes follow: stream pointer, runtime vertex buffer
};

struct CmmSkinHeader
{
    uint32_t unused00;
    uint32_t boneCount;           // 2..8
    uint32_t bones[8];
    uint32_t staleVertices;
    uint32_t runtimeVertexBuffer;
    uint32_t unused30[2];
};

// cmm_fixup_pointers (0xb67a0) copies type and frame count into the model's texture slot (+0x1c / +0x20, rate +0x24 =
// 60 until a stage sets it); the game compares the type only with its render-target types 0x60..0x63 (0xcbb40), so
// the file's 0 / 3 / 4 / 6 / 8 are the tool's tags: 3 on the bump effects' normal maps, 4 / 6 / 8 on animated ones
struct CmmTextureEntry
{
    uint32_t type;                // 3 normal map, 4/6/8 animated (tags; see above)
    uint32_t frameCount;          // > 1: tex<i>.dds holds that many DDS files, frame = clock x stage rate % count
    uint32_t unread08[2];         // varied values, not read by the game
};

// FUN_000aa5f0 looks the name up in the effect registry (0x292d5c + 0x418, 0x5c-byte entries registered by 0x7db10);
// the index goes to the model's effect slot (model +0x3a4 + 0x48 x record). The registry's vertex shader is created
// once (create_vertex_shader 0xaaa10) from the first model's program and declaration
struct CmmEffect
{
    char name[0x40];              // "Specular Bump", ...
    uint32_t unused40;
    uint32_t flags;               // bit 0: use the effect (else the mesh is not drawn, 0xb6ca0); 1 in every file
    uint32_t unused48[2];
    uint32_t programSize;         // its vertex shader program follows the records
    uint32_t staleProgram;        // rebuilt by cmm_fixup_pointers (0xb6859)
    uint8_t runtime058[0x144];
    uint32_t runtimeDeclaration[0x100];   // the prepare's vertex declaration, copied by 0xaa810
    uint8_t unused59c[0x20];
};

namespace animation_keys
{
constexpr uint32_t kPosition = 0x1;
constexpr uint32_t kScale = 0x2;
constexpr uint32_t kRotation = 0x4;
} // namespace animation_keys

struct CmmAnimationTrack
{
    uint32_t unused00;
    uint32_t keyFlags;            // animation_keys
    uint32_t keyCount;
    float keysPerSecond;
    char boneName[0x40];
    uint32_t stalePositionKeys, staleScaleKeys, staleRotationKeys;
    uint8_t unused5c[0x24];
};

// a .cms animation (0xb5270): this header, then trackCount CmmAnimationTrack records and their keys
struct CmsHeader
{
    float seconds;
    int32_t trackCount;
    uint32_t staleTracks;
    uint32_t staleSelf;
    uint8_t unused10[0x20];
};

#pragma pack(pop)

static_assert(sizeof(CmsHeader) == 0x30);

static_assert(sizeof(CmmHeader) == 0x48);
static_assert(offsetof(CmmHeader, rootBone) == 0x28 && offsetof(CmmHeader, effectCount) == 0x30);
static_assert(offsetof(CmmHeader, animationTrackCount) == 0x40);
static_assert(sizeof(CmmMesh) == 200);
static_assert(offsetof(CmmMesh, faceCount) == 0x28 && offsetof(CmmMesh, textures) == 0x38);
static_assert(offsetof(CmmMesh, runtimeVertexShader) == 0x40 && offsetof(CmmMesh, runtimeBounds) == 0x5c);
static_assert(offsetof(CmmMesh, material) == 0x48 && offsetof(CmmMesh, sortPoint) == 0x4c);
static_assert(offsetof(CmmMesh, cubeMap) == 0xa8 && offsetof(CmmMesh, lodBand) == 0xab && offsetof(CmmMesh, lodFar) == 0xb0);
static_assert(offsetof(CmmMesh, runtimeIndex) == 0xb4 && offsetof(CmmMesh, vertexType) == 0xc2);
static_assert(sizeof(CmmStage) == 0x34 && offsetof(CmmStage, offsetV) == 0x28);
static_assert(sizeof(CmmMaterial) == 0xec);
static_assert(offsetof(CmmMaterial, stages) == 0x0c && offsetof(CmmMaterial, effect) == 0xe0);
static_assert(sizeof(CmmBone) == 0x1a0);
static_assert(offsetof(CmmBone, world) == 0x40 && offsetof(CmmBone, hierarchyWorld) == 0x80);
static_assert(offsetof(CmmBone, animationTrack) == 0x100 && offsetof(CmmBone, runtimeTintedColour) == 0x10c);
static_assert(offsetof(CmmBone, rowScale) == 0x11c && offsetof(CmmBone, meshCount) == 0x12c);
static_assert(offsetof(CmmBone, index) == 0x134 && offsetof(CmmBone, intensity) == 0x138);
static_assert(offsetof(CmmBone, name) == 0x140 && offsetof(CmmBone, parent) == 0x180);
static_assert(offsetof(CmmBone, clock) == 0x188 && offsetof(CmmBone, copiedBone) == 0x190);
static_assert(sizeof(CmmSurfaceHeader) == 0x10 && sizeof(CmmSurfaceCollision) == 0x10 && sizeof(CmmSurfaceLighting) == 0x20);
static_assert(sizeof(CmmSurfaceBlock1) == 0x60 && offsetof(CmmSurfaceBlock1, lighting) == 0x40);
static_assert(sizeof(CmmSurfaceBlock2) == 0x30 && offsetof(CmmSurfaceBlock2, lighting) == 0x10);
static_assert(offsetof(CmmSurfaceLighting, occlusionRadius) == 0x0c && offsetof(CmmSurfaceLighting, fogRange) == 0x1c);
static_assert(sizeof(CmmUvPair) == 8);
static_assert(sizeof(CmmPlainVertex) == 0x1c && offsetof(CmmPlainVertex, colour) == 0x18);
static_assert(sizeof(CmmPackedVertex) == 0x10 && offsetof(CmmPackedVertex, colour) == 0x0c);
static_assert(vertexSize(0) == 0x24 && vertexSize(3) == 0x3c && vertexSize(4) == 0x18 && vertexSize(7) == 0x30);
static_assert(sizeof(CmmSkinnedVertexShared) == 0x18 && offsetof(CmmSkinnedVertexShared, uv) == 0x10);
static_assert(sizeof(CmmSkinnedVertexBone) == 0x10);
static_assert(skinnedVertexSize(2) == 0x38 && skinnedVertexSize(8) == 0x98);
static_assert(skinnedSharedOffset(2) == 0x18 && skinnedWeightsOffset(2) == 0x30);
static_assert(sizeof(CmmMorphHeader) == 0x10);
static_assert(sizeof(CmmSkinHeader) == 0x38 && offsetof(CmmSkinHeader, staleVertices) == 0x28);
static_assert(sizeof(CmmTextureEntry) == 0x10);
static_assert(sizeof(CmmEffect) == 0x5bc);
static_assert(offsetof(CmmEffect, flags) == 0x44 && offsetof(CmmEffect, programSize) == 0x50);
static_assert(offsetof(CmmEffect, runtimeDeclaration) == 0x19c);
static_assert(sizeof(CmmAnimationTrack) == 0x80 && offsetof(CmmAnimationTrack, stalePositionKeys) == 0x50);

} // namespace argon::original
