// Argon: the GPU side a run keeps from start to shutdown: the backend (device, presenter, ImGui's renderer), the shared
// fallback textures and set pools, the scene constants' buffers, the editor's line and the water pipelines, and the
// passes that own GPU resources (ship shadows, flare queries, the effects' buffers, post AA, order-independent
// translucency). App owns one (App::renderer); the models it loads stay with App.
#pragma once
#include "gfx/backend.hpp"
#include "gfx/texture.hpp"
#include "render/effect_buffers.hpp"
#include "render/flare_queries.hpp"
#include "render/mesh_draw_records.hpp"
#include "render/order_independent_translucency.hpp"
#include "render/post_anti_aliasing.hpp"
#include "render/scene_buffers.hpp"
#include "render/ship_shadows.hpp"
#include "render/upload_batches.hpp"

#include <memory>
#include <string>

struct GLFWwindow;

class Renderer : public UploadBatches
{
  public:
    // the backend the options ask for over the window; throws when it cannot run here
    void start(const argon::gfx::BackendOptions &options, GLFWwindow *window);
    // start, but on Windows Direct3D 12 when Vulkan cannot run here (kind() says which started); throws when neither can
    void startOrFallBack(const argon::gfx::BackendOptions &options, GLFWwindow *window);
    // what the passes draw with: the pipelines, pools, fallback textures, scene buffers (viewSlots of them), passes and
    // gameFolder holds the effects' textures
    void createResources(const std::string &gameFolder, int viewSlots);
    // Post AA, before ImGui stops (the GPU idle)
    void releaseScreenPasses();
    void stopUi();
    // the rest and the backend itself (the models released and ImGui stopped first)
    void destroy();

    bool started() const;
    argon::gfx::BackendKind kind() const;
    argon::gfx::Device &device() const;
    argon::gfx::Presenter &presenter() const;
    argon::gfx::UiRenderer &ui() const;
    void startUi();
    // the device's upload batch (UploadBatches)
    void beginUploadBatch() override;
    void submitUploadBatch() override;
    // every scene set's frame copy (the "Hud Lens" effect samples it; the GPU idle)
    void bindFrameCopy(argon::gfx::TextureRef copy);

    // ---- each frame (App::recordFrame)
    // the mesh-draw records for `frame` started again and the flare queries' last results read
    void beginFrame(argon::gfx::CommandList &commands, int frame);
    // the view's scene constants, and the set the passes bind for it
    void writeSceneConstants(int viewSlot, const SceneUbo &constants);
    argon::gfx::SceneSet sceneSet(int viewSlot) const;
    // the shadow-map passes for `ships`, received in the view's scene set; clearShadows: none this frame
    struct ShadowView
    {
        int frame, viewSlot;
        glm::vec3 sun;
        const argon::CollisionGrid &receivers;
        bool solid;
    };
    void recordShadows(argon::gfx::CommandList &commands, const std::vector<ShadowedShip> &ships, const ShadowView &view);
    void clearShadows();
    // the engine trails' vertices for `frame`; clearTrails: none drawn
    void writeTrails(int frame, const argon::EngineTrails &trails);
    void clearTrails();
    // Argon Options' Post AA and Translucency as the frame starts, and Post AA over the 3D view once it is drawn
    void prepareScreenPasses(PostAntiAliasing postAntiAliasingMode, bool orderIndependent);
    void recordPostAntiAliasing(argon::gfx::CommandList &commands);

    // ---- the effects' textures: data\hud's particle atlases (once), a track's spray
    void loadParticleTextures();
    bool hasSprayTexture(const std::string &track) const;
    void useSprayTexture(const argon::TextureData &picture, const std::string &track);

    argon::gfx::Pipeline linePipe, waterPipe;
    argon::gfx::Texture white, whiteCube;   // the fallbacks: a white pixel, a grey cube for tracks without waterref1.dds
    argon::gfx::SetPool trackPool;          // the loaded track's sets
    argon::gfx::SetPool spareTrackPool;     // the next track's (trackPool's sets may still be in flight)
    argon::gfx::SetPool propPool;           // ships on the start grid, pickups, explosions, menu models (whole session)
    MeshDrawRecords meshDraws;              // the mesh draws' records per frame in flight
    SceneBuffers sceneBuffers;              // the scene constants per frame in flight and view
    ShipShadows shadows;
    FlareQueries flares;
    EffectBuffers effectBuffers;            // the effects' pipelines, textures and vertex buffers
    std::unique_ptr<PostAntiAliasingPass> postAntiAliasing;           // Argon Options' Post AA over the 3D view
    std::unique_ptr<OrderIndependentTranslucency> translucency;       // Argon Options' Translucency: Order Independent

  private:
    void createLineAndWaterPipelines();
    void createFallbackTextures();
    void createSetPools();

    std::unique_ptr<argon::gfx::Backend> backend_;
    argon::gfx::BackendKind kind_ = argon::gfx::BackendKind::Vulkan;
};
