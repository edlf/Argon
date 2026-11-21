// Argon: the race's effect passes over the scene SceneRecorder draws: the water (water_draw 0xce1c0), the ship
// shadows on it, and after the translucent pass the spray, the weapon and track sprites (0xbd8f0), the ship trails
// and the pickup starbursts (docs\effects\README.md).
#pragma once
#include "effects/wake_spray.hpp"
#include "effects/weapon_sprites.hpp"
#include "gfx/command_list.hpp"
#include "logic/particles.hpp"
#include "render/gpu_model.hpp"
#include "render/effect_buffers.hpp"
#include "render/ship_shadows.hpp"
#include "render/scene_toggles.hpp"
#include "render/scene_views.hpp"

class SceneRecorder;

// what the effect passes draw and with what, for one view
struct EffectSources
{
    EffectBuffers &buffers;                  // the effects' pipelines, textures and vertex buffers
    const ShipShadows &shadows;
    const argon::gfx::Pipeline &waterPipe;
    float screenHeight;                      // the particles' size on screen
    const SceneToggles &toggles;
    const argon::WakeSpray &spray;
    const argon::WeaponSprites &weaponSprites;
    const argon::Particles &particles;
    const Track &track;          // its water mesh and sprites
    const HudCamera &camera;     // the sprites face its eye
    float clock;                 // the effects' clock (App::simClock)
    int frame, viewSlot;         // the frame in flight, and its view's buffers
};

class EffectPasses
{
  public:
    EffectPasses(const EffectSources &sources, const SceneRecorder &scene);   // at the scene's current view-projection
    void drawWater(double seconds);
    void drawShadowReceivers();   // after the water, whose depth hides the shadows under it
    void drawEffects(const glm::mat4 &view, float fovDeg, double seconds);

  private:
    void drawSpray();
    void drawTrailSprites();
    void drawTrackSprites();
    void drawTrackSpriteList(argon::gfx::Pipeline pipe, argon::gfx::DrawRange range);
    void drawTrail(double seconds);
    void drawParticles();

    EffectSources sources_;
    const SceneRecorder &scene_;
    argon::gfx::CommandList &commands_;
    glm::mat4 viewProj_;
};
