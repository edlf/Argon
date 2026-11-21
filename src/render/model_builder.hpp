// Argon: a read model (model\model_source.hpp) made into draws on the GPU (render\gpu_model.hpp), and the rest of
// its GPU life: the environment cubes, the game's water, the animated models' posing and skinning, releasing it.
#pragma once
#include "files/dds.hpp"
#include "model/model_source.hpp"
#include "model/weapon_bones.hpp"
#include "render/gpu_model.hpp"

#include <array>
#include <string>
#include <vector>

// the four texture stages a draw binds (set 0) and how each is sampled
struct StageTextures
{
    std::array<const Texture *, 4> textures{};
    std::array<argon::gfx::SamplerMode, 4> samplers{};
};

argon::gfx::TextureSet createStageSet(argon::gfx::Device &device, argon::gfx::SetPool pool, const StageTextures &stages);

// an animated model's pose: its clock, and the frame in flight whose vertex buffer the skinned vertices go to
struct ModelClock
{
    float seconds = 0.f;
    int frame = 0;
};

class ModelBuilder
{
  public:
    // raceTrack: the loaded race track, whose cube table the specular-bump effects of every model take (cubeOwner);
    // dumpMaterials: --dump-materials, one line per mesh with its stages
    struct Setup
    {
        argon::gfx::Device &device;
        const Texture &white;
        Track &raceTrack;
        std::string gameFolder;
        bool dumpMaterials = false;
    };
    void create(const Setup &setup);

    // use.pool: the track's pool for the track and sky (reset on a track change), the props' for ships and pickups
    // (kept); use.prop marks the ship effect parts (shields, weapon fx) so they can be hidden
    bool load(Track &model, const std::string &folder, const ModelUse &use);
    bool build(Track &model, ModelSource &source, const ModelUse &use);
    // build's two halves, which a track load (track\track_load.hpp) calls a slice at a time: the next texture (model
    // textures follow source.textures), then the draws, buffers and material set
    void uploadTexture(Track &model, const ModelSource &source);
    bool buildMeshes(Track &model, ModelSource &source, const ModelUse &use);
    void release(Track &model);

    // The game's water (water.cpp): 128 m patches over the water.dat boxes whose flag a & 1 is clear
    // (0xcdc20 / 0xcdb60), 30 precomputed wave frames (0xcd4f0), drawn with the ported waterbumpfresnal
    // vertex shader and texm3x3vspec pixel shader (shaders/water.vert / water.frag). Boxes with a & 1 are
    // physics only: their visible water is the track's own meshes. bump: readWaterBump (track_source.hpp)
    void buildWater(Track &track, const argon::TextureData &bump, argon::gfx::SetPool pool);
    // env: readTrackCube (no levels = none)
    void setCube(Track &track, int slot, const argon::TextureData &env);
    // before the model's draws are recorded: its specular-bump sets for the race track loaded now (raceSerial, its
    // sets in racePool) or, with none, for its own cubes
    void refreshCubes(Track &model, argon::gfx::SetPool racePool, uint64_t raceSerial);

    // 0xb8630 per frame: pose the skeleton at the model's clock, skin the weighted meshes, move the meshes that ride
    // on a bone (Track::anim)
    void animate(Track &model, ModelClock clock);
    // the same with its bones driven by other animations and turned by aims (a story scene's actors)
    void animateWith(Track &model, const argon::PoseDrivers &drivers, ModelClock clock);

  private:
    // what a material binds: its stage textures, the key its texture set is shared by, the cube table slot of a
    // specular-bump effect (-1: none)
    struct MaterialBinding
    {
        StageTextures stages;
        uint64_t key = 0;
        int cubeSlot = -1;
    };
    struct Material
    {
        argon::GpuMaterial gpu{};
        MaterialBinding binding;
    };
    // a model being turned into draws: its source and the materials being filled
    struct Build
    {
        ModelSource &source;
        const ModelUse &use;
        std::vector<argon::GpuMaterial> materials;
    };
    struct MeshBuild
    {
        const argon::Cmm &cmm;
        const argon::CmmMesh &mesh;
        const Draw &draw;
        bool isSky;
    };

    const Texture *cube(Track &track, int slot);
    Track &cubeOwner(Track &model);
    uint64_t cubeOwnerKey(Track &model, uint64_t raceSerial);
    void registerCubeDraw(Track &model, const MaterialBinding &binding, argon::gfx::SetPool pool);

    void setBones(Track &model, const argon::Cmm &cmm);
    void addDraw(Track &model, Build &build, const argon::CmmMesh &mesh);
    void bindMaterial(Track &model, Build &build, const argon::CmmMesh &mesh, Draw &draw);
    Material buildMaterial(Track &model, const MeshBuild &mesh);
    void applyEffect(Track &model, const MeshBuild &mesh, size_t stageCount, Material &material);
    void dumpMaterial(const Track &model, const MeshBuild &mesh, const std::vector<argon::CmmStage> &stages) const;
    void uploadBuffers(Track &model, Build &build);
    void uploadReskinnedVertices(Track &model, const std::vector<argon::Vertex> &vertices);
    void uploadWaterWaves(Track &track);
    void uploadWaterGrid(Track &track);
    void skinPosed(Track &model, ModelClock clock);

    argon::gfx::Device *device_ = nullptr;
    const Texture *white_ = nullptr;
    Track *raceTrack_ = nullptr;
    std::string game_;
    bool dumpMaterials_ = false;
    argon::WeaponBones weaponBones_ = argon::WeaponBones::fromWeaponFile({});
};
