// Argon: what SceneRecorder (render\scene_recorder.hpp) draws the scene from: the state it reads (SceneSources) and
// what it asks App for (SceneHost: the race's placements, the frame's facts, the countdown and the editor's markings).
#pragma once
#include "render/frame_clocks.hpp"
#include "effects/craft_exhausts.hpp"
#include "effects/wake_spray.hpp"
#include "effects/weapon_sprites.hpp"
#include "gfx/gfx_types.hpp"
#include "logic/particles.hpp"
#include "logic/pickups.hpp"
#include "render/scene_views.hpp"
#include "util/found.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace argon::gfx
{
class Device;
class Presenter;
}  // namespace argon::gfx
class DriveCamera;
class FrameCopies;
class GameModes;
class ModelBuilder;
class PropModels;
class Race;
class CraftPoses;
class EffectBuffers;
class FlareQueries;
class MeshDrawRecords;
class OrderIndependentTranslucency;
class RaceEggs;
class SceneBuffers;
class SceneRecorder;
class ShipShadows;
class StartGrid;
class StoryScene;
class Weapons;
struct SceneToggles;
struct Track;

// in a race, per view: the track meshes its nearest visibility sector shows, made again only when the sector, the
// track (its serial) or its draw count changes (SceneRecorder::findVisibleTrackMeshes)
struct TrackSectorMeshes
{
    uint64_t trackSerial = 0;
    size_t sector = 0, drawCount = 0;
    std::vector<bool> visible;
};

struct SceneSources
{
    // the GPU side it records with (App::renderer's parts)
    argon::gfx::Device &device;
    const argon::gfx::Presenter &presenter;     // the window's size
    const argon::gfx::SetPool &trackPool;       // the track's cube sets
    const argon::gfx::Pipeline &waterPipe;      // the effects' water
    MeshDrawRecords &meshDraws;
    const SceneBuffers &sceneBuffers;
    FlareQueries &flares;
    OrderIndependentTranslucency &translucency;
    const ShipShadows &shadows;
    EffectBuffers &effectBuffers;
    const SceneToggles &toggles;
    Track &track, &sky;
    const HudCamera &camera;
    const float &clock;              // the effects' clock (App::simClock)
    const int &frame, &view;         // the frame in flight and the view being recorded
    const uint64_t &trackSerial;     // a new track: the cached sectors and cubes made again
    std::span<TrackSectorMeshes> sectorMeshes;   // per view
    const std::vector<int> &hiddenMeshes;        // --hide-mesh
    // the race
    const Race &race;
    GameModes &modes;
    StartGrid &startGrid;
    const DriveCamera &driveView;
    const RaceEggs &raceEggs;
    const Weapons &weapons;
    const bool &driving, &chaseCam, &weaponsOn;
    // what draws with it
    PropModels &props;
    ModelBuilder &models;
    const argon::Pickups &pickups;
    const std::map<std::string, Found<Track>> &pickupFor, &pickupTakenFor;
    const argon::WeaponSprites &weaponSprites;
    const argon::WakeSpray &spray;
    const argon::Particles &particles;
    const argon::CraftExhausts &exhausts;
    const std::unique_ptr<StoryScene> &story;
    FrameCopies &frameCopies;
    const CraftPoses &poses;   // where the crafts are drawn
};

// the frame the scene records into, its clocks, and what it draws on the race's behalf (app\scene_frame.hpp)
class SceneHost : public FrameClocks
{
  public:
    virtual int viewSlot() const = 0;                   // the scene constants' slot of the view being recorded
    virtual argon::gfx::Winding sceneFrontFace() const = 0;
    virtual argon::gfx::Winding modelFrontFace() const = 0;
    virtual void loadSprayTexture() = 0;              // the track's, the first time it is drawn
    virtual void animatePickupModels(float seconds) = 0;
    virtual void drawCountdown(SceneRecorder &recorder) = 0;
    virtual void drawEditorMarkings(const SceneRecorder &recorder) const = 0;

  protected:
    ~SceneHost() = default;
};

// what the scene asks about the race it draws (App)
class SceneRaceView
{
  public:
    virtual bool slotShown(int slot) const = 0;
    virtual Found<Track> shownGhostModel(int ghost) = 0;
    virtual int hiddenNetGlowBone() const = 0;
    virtual bool frontEndOpen() const = 0;
    virtual bool modeHasPickups() const = 0;

  protected:
    ~SceneRaceView() = default;
};
