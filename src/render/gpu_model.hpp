// Argon: a model as the renderer keeps it: its draws, buffers, textures, bones, water and animation state (a track, its
// sky, a craft, a pickup, an explosion, a front-end model). render\model_builder.hpp makes and releases them.
#pragma once
#include "craft/craft_flaps.hpp"
#include "gfx/device.hpp"
#include "gfx/texture.hpp"
#include "model/cmm.hpp"
#include "model/material.hpp"
#include "render/camera_facing.hpp"
#include "render/d3d_lighting.hpp"
#include "render/scene_toggles.hpp"
#include "render/texture_bind_cache.hpp"
#include "track/collision_surface.hpp"
#include "track/trackdata.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

using Texture = argon::gfx::Texture;   // a texture as the renderer keeps it (gfx\texture.hpp)

struct Draw
{
    uint32_t firstIndex = 0, indexCount = 0;
    int32_t vertexOffset = 0;
    int tex = -1;          // stage-0 texture (index into Track::textures), -1 = none
    int group = G_VISUAL;
    glm::mat4 model{1.f};
    int mesh = 0;
    argon::MeshState st;      // game blend / depth / cull state
    argon::gfx::TextureSet texSet;   // the 4 stage textures
    uint32_t matIndex = 0;                     // index into Track::matBuf (= firstInstance)
    argon::MeshTextures textures;              // the texture slots and rates the mesh binds (the bind cache's input)
    bool fx = false;       // ship/pickup effect part (shield, weapon fx) hidden when drawn as a prop
    int bone = -1;         // the mesh's bone (weapon projectiles draw their bone's meshes)
    bool tintLit = false;             // lit by the surface's own colour (flag 4): the instance tint shows
    glm::vec3 bmin{0.f}, bmax{0.f};   // world bounds (the translucent pass's sort key, 0xafcf0)
    bool sortByPoint = false;         // mesh flag 0x80: sorted by the eye's distance to sortPoint instead
    glm::vec3 sortPoint{0.f};
    bool occlTest = false;            // surface flag 0x40000: occlusion-tested flare (0xb6e60)
    float occlRadius = 0.f;
    int occlPoints = 0;
    bool occlCopy = false;            // surface flag 0x80000: intensity copied from the parent bone (0xb6e00 / 0xb6e30)
    bool hudLens = false;             // the "Hud Lens" effect: drawn after a copy of the frame (SceneRecorder)
    bool colorScale = false;          // the "Color Scale No Texture" effect: vertex colour x bone tint x intensity
    argon::SurfaceLighting lighting;     // D3D lighting on, its ambient and light 0 (render\d3d_lighting.hpp)
    bool shadowHull = false;          // a craft's low-poly hull (surface flag 0x400000): drawn by the shadow pass only
    argon::CameraPlacement camera;       // surface flags 0x400 / 0x800 / 0x100000 (render\camera_facing.hpp)
};

struct Track
{
    std::string name;
    // the translucent pass's alpha-test reference at least this (set before loading): the front end's character
    // models are cut at 0.5 (xemu: hard hair edges without the dark fringe a ref of 0 leaves)
    float translucentAlphaRef = 0.f;
    bool posable = false;   // keep the skeleton for other animations to pose (a story scene's actors; set before loading)
    std::vector<std::string> boneName;   // per bone of the model
    std::vector<int> boneParent;
    std::vector<glm::mat4> boneRest;     // model-space rest matrix
    std::vector<glm::mat4> boneRestInverse;   // per bone: inverse(boneRest), made with it
    std::vector<int> jetOf;              // per bone: the "JET" bone it sits under (-1 none), 0x50670
    std::vector<uint8_t> flareOf;        // per bone: under a "JFLARE" bone
    argon::FlapRig flaps;                   // a ship's airbrake flap bones (craft_flaps.hpp)
    int findBone(const std::string &wanted) const
    {
        for (size_t i = 0; i < boneName.size(); ++i)
        {
            if (boneName[i] == wanted)
            {
                return int(i);
            }
        }
        return -1;
    }
    bool underBone(int bone, int root) const
    {   // b is root or below it
        for (int g = 0; bone >= 0 && g < 64; bone = boneParent[size_t(bone)], ++g)
        {
            if (bone == root)
            {
                return true;
            }
        }
        return false;
    }
    argon::gfx::BufferRef vb, ib, lines;
    std::vector<Texture> textures;
    std::vector<Draw> draws;
    uint32_t lineStarts[2]{}, lineAi[2]{}, linePups[2]{}, lineSounds[2]{}, lineIssues[2]{};   // first, count
    std::vector<glm::vec3> collTris;   // solid triangles (world space), for the AI drive
    std::vector<argon::TriangleSurface> collSurf;   // per collision triangle (cmm surface block +0x14 / +0x18)
    int groupCount[G_COUNT]{};
    bool hasShadowHull = false;        // a craft with a shadow hull: the shadow pass draws only that
    int blendedCount = 0;
    size_t triangles = 0;
    glm::vec3 bmin{1e9f}, bmax{-1e9f};
    argon::TrackData data;
    // game water (water.cpp, see docs/water/README.md): one 17 x 17 grid drawn once per 128 m patch
    argon::gfx::BufferRef waterVb, waterIb;            // grid (289 vertices, the shader reads the wave frames by vertex index)
    uint32_t waterIndexCount = 0;
    argon::gfx::BufferRef waterWave;                   // 30 wave frames x 289 vertices x {pos, tangent, binormal, normal} (set 2)
    argon::gfx::MaterialSet waterWaveSet;
    std::array<Texture, argon::kEnvCubeSlots> envCubes;   // the environment cube slots as 6-layer arrays
    Texture waterBump;                  // data\testdata\waterbump000..059 as a 60-layer array
    argon::gfx::TextureSet waterSet;
    struct WaterPatch
    {
        glm::vec3 pos;
        int cube;
    };
    std::array<bool, argon::kEnvCubeSlots> cubeTried{};   // envCubes' load attempted (water and specular-bump meshes)
    std::vector<WaterPatch> waterPatches;
    argon::gfx::BufferRef matBuf;      // GpuMaterial per draw (set 2)
    argon::gfx::MaterialSet matSet;
    std::map<uint64_t, argon::gfx::TextureSet> texSets;   // 4 textures + sampler modes -> set
    // the specular-bump draws (stage 1 an environment cube): while a race track is loaded they take its cube table
    // (ModelBuilder::cubeOwner), else the model's own; their sets are made again when that changes (ModelBuilder::refreshCubes),
    // a borrowed table's in the track's set pool, so the sets go with the track and its cubes
    struct CubeDraws
    {
        argon::gfx::TextureSetDesc desc;
        int slot = 0;                         // the cube table slot (material +0xa8)
        argon::gfx::SetPool pool;             // the model's own pool (its own-cube set)
        argon::gfx::TextureSet ownSet;        // with the model's own cube, made once
        std::vector<uint32_t> draws;          // indices into draws
    };
    std::map<uint64_t, CubeDraws> cubeDraws;   // by texture set key
    uint64_t cubeOwner = ~uint64_t(0);         // ModelBuilder::cubeOwnerKey the sets were made for (none yet)
    int multiStage = 0, animated = 0, envMapped = 0;
    // bone-animated model (front-end characters, docs\models\meshes.md "Animation"): re-posed every frame by ModelBuilder::animate;
    // vb is then one of animVb[] (host visible, one per frame in flight), rewritten with the skinned vertices
    std::shared_ptr<argon::Cmm> anim;
    std::vector<argon::Vertex> animVerts;
    argon::gfx::BufferRef animVb[2];
    void *animMapped[2]{};
    std::vector<float> animWorld;
    std::vector<std::string> warnings;
    double loadSeconds = 0;
};
