// Records one frame's 3D scene into a command buffer: the track, sky, ships, pickups, shots and shields; the water
// and effects through EffectPasses (effect_passes.hpp), the editor's markings (editor_markings.hpp).
#pragma once
#include <optional>
#include "logic/shot_flight.hpp"
#include "render/camera_facing.hpp"
#include "render/d3d_lighting.hpp"
#include "render/gpu_model.hpp"
#include "render/mesh_draw_records.hpp"
#include "render/push_constants.hpp"
#include "render/scene_sources.hpp"
#include "render/texture_bind_cache.hpp"
#include "gfx/command_list.hpp"
#include "ui/fe_scene.hpp"

// How a model is placed and which of its meshes draw.
struct ModelPlacement
{
    glm::mat4 instance{1.f};
    bool isSky = false;                   // no depth test / write
    bool isProp = false;                   // ship / pickup: effect parts hidden, the layer toggles ignored
    int onlyBone = -1;                    // >= 0: just that bone's meshes (and its children's), world = boneMatrix x mesh
    glm::mat4 boneMatrix{1.f};
    float jetSize = -1.f;                 // the ship's jet size (-1: not a ship)
    const argon::FlapState *flaps = nullptr; // the ship's airbrake flaps (null: rest pose)
    int occlusionInstance = 0;            // instance id for the flare occlusion keys (ship grid slot + 1)
    const std::vector<float> *bonePose = nullptr;   // the model's bone worlds (Cmm::pose) instead of rest
    std::optional<glm::vec3> tint;        // the bone tint (instance +0x104 / 256) of lit meshes (0xba1d0)
    std::optional<argon::SceneLights> lights;   // a racing craft's own ambient and light 0 (0x59bc0); none: the track's
    int turnedBone = -1;                  // >= 0: that bone's meshes (and its children's) turned by boneTurn in its space
    glm::mat4 boneTurn{1.f};
    bool cameraListed = true;             // the render list's camera step applies (off in the front end: docs\effects\README.md)

    static ModelPlacement forSky();
    static ModelPlacement forProp(const glm::mat4 &instance);
    static ModelPlacement forBone(int bone, const glm::mat4 &boneMatrix);
};

class SceneRecorder
{
  public:
    SceneRecorder(SceneHost &host, SceneRaceView &race, const SceneSources &sources, argon::gfx::CommandList &commands);

    void useViewProj(const glm::mat4 &viewProj)
    {
        viewProj_ = viewProj;
    }
    // the camera the camera-placed meshes (surface flags 0x400 / 0x800 / 0x100000) are placed by
    void useViewPoint(const glm::mat4 &view)
    {
        viewPoint_ = argon::ViewPoint::of(view);
    }
    void setDepthCull(const argon::gfx::DepthCull &state) const;   // writes only where it tests
    void pushMatrices(const glm::mat4 &mvp, const glm::mat4 &model) const;
    std::optional<argon::SceneLights> craftLightsOf(int slot) const;
    void setViewport(const argon::gfx::Rect &area) const;
    void bindVertexBuffer(argon::gfx::BufferRef buffer) const;
    argon::gfx::CommandList &commands() const
    {
        return commands_;
    }
    const glm::mat4 &viewProj() const
    {
        return viewProj_;
    }

    void drawModel(Track &model, int pass, const ModelPlacement &placement);
    void drawModelBothPasses(Track &model, const ModelPlacement &placement);

    void drawWorld(const glm::mat4 &view, float fovDeg);
    void drawFrontEndModels(const FeScene &scene, float aspect);

    struct SortedDraw
    {
        float key;        // translucentSortKey
        uint32_t index;   // into the model's draws
    };

  private:
    struct MeshCall
    {
        Track &model;
        const Draw &draw;
        uint32_t index;
        const ModelPlacement &placement;
    };
    // which of the translucent pass's draws a drawModel records (render\order_independent_translucency.hpp)
    enum class TranslucentDraws
    {
        All,                // the game's order
        SolidParts,         // the alpha-blended ones' solid pixels, into the scene
        OrderIndependent,   // the alpha-blended ones' other pixels, into the OIT targets
        OitDepth,           // the alpha-blended ones' depth after the composite
        Remaining           // the others, after the OIT composite
    };

    void bindModelBuffers(Track &model) const;
    bool sortsTranslucentDraws(const Track &model, int pass, const ModelPlacement &placement) const;
    void sortTranslucentDraws(const Track &model);
    float translucentSortKey(const Draw &draw) const;
    bool isMeshHidden(const MeshCall &mesh) const;
    const argon::MeshState &meshState(const Draw &d) const;
    bool cullsBackFaces(const argon::MeshState &st) const;
    void bindMeshState(const MeshCall &mesh);
    void bindTextures(argon::gfx::TextureSet set);
    bool drawsInPass(const Draw &draw, int pass) const;
    static bool blendsOrderIndependently(const Draw &d);
    bool usesOrderIndependence() const;
    argon::gfx::Pipeline pipelineOf(const argon::MeshState &st) const;
    void drawTranslucentPass(float seconds);
    void drawTranslucentDraws(TranslucentDraws which, float seconds);
    void drawMesh(const MeshCall &mesh, int pass);
    MeshDraw meshDrawOf(const MeshCall &mesh) const;
    glm::vec4 boundStageRates() const;

    float occlusionIntensity(const MeshCall &mesh);
    float parentOcclusion(const MeshCall &mesh) const;
    float testedOcclusion(const MeshCall &mesh);
    void queueOcclusionQuery(const MeshCall &mesh, uint64_t key);

    glm::mat4 baseWorld(const MeshCall &mesh) const;
    glm::mat4 meshWorld(const MeshCall &mesh) const;
    glm::mat4 listedWorld(const MeshCall &mesh) const;
    int jetBoneOf(const MeshCall &mesh) const;
    bool isTurnedBone(const MeshCall &mesh) const;
    static ModelPlacement frontEndPlacement(const FeScene &scene, int model);
    int flapSideOf(const MeshCall &mesh) const;
    std::optional<glm::vec3> jetTint(const MeshCall &mesh) const;
    std::optional<glm::vec3> boneTint(const MeshCall &mesh) const;
    glm::vec3 beamTint(const argon::weapons::Shot &s) const;
    glm::mat4 shaderModel(const MeshCall &mesh, glm::mat4 world, float occlusion) const;

    void findVisibleTrackMeshes();
    size_t trackMeshCount() const;
    void drawSky();
    void drawProps(int pass, float seconds);
    void drawStoryActors(int pass);
    void drawGridShips(int pass);
    void drawGhosts(int pass);
    void drawPickups(int pass, float seconds);
    void drawShots(int pass, float seconds);
    ModelPlacement shotPlacement(const Track &ship, int bone, const glm::mat4 &shot, float seconds, int pass);
    const std::vector<float> &propPose(const argon::Cmm &anim, float seconds, int pass);
    void drawShields(int pass);
    void drawShield(int slot, int pass);
    void drawExplosions(int pass);
    void drawHudLenses();

    SceneHost &host_;
    SceneRaceView &race_;
    SceneSources sources_;
    argon::gfx::CommandList &commands_;
    glm::mat4 viewProj_{1.f};
    argon::ViewPoint viewPoint_;
    argon::gfx::Pipeline boundPipe_;
    argon::gfx::TextureSet boundTextures_;
    argon::TextureBindCache bindCache_;   // the game's per-stage texture binds (0x50e374), for the animated frames
    const std::vector<bool> *visibleTrackMeshes_ = nullptr;   // in a race: the meshes the nearest visibility sector lists
                                                            // (0xafcf0, SceneSources::sectorMeshes); null: all
    int hiddenTrackBone_ = -1;   // the track's NET_GLOW until a craft is knocked out (0x61f20)
    struct DeferredLens
    {
        Track *model;
        uint32_t index;
        ModelPlacement placement;
    };
    std::vector<DeferredLens> lenses_;   // the world's Hud Lens meshes, drawn after the frame copy
    bool deferLenses_ = false;
    TranslucentDraws translucentDraws_ = TranslucentDraws::All;
    std::vector<SortedDraw> sortedDraws_;   // the track's translucent pass, far to near (its storage reused by each view)
    std::vector<std::vector<float>> propPoses_;   // pass 0's poses of the animated props, in draw order (propPose)
    size_t nextPropPose_ = 0;
};
