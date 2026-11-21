// Argon: one frame's commands between the presenter's beginFrame and endFrame: the shadow maps and the movie's frame
// before the scene pass, then each view on its part of the screen with its camera (render\view_cameras.hpp), their
// post-process anti-aliasing and ImGui in the pass, the transition's copy after it. On the front end the views draw
// nothing themselves: the page's models draw from the shell's ImGui callbacks (FrontEndModels), after the
// anti-aliasing, so only MSAA smooths them. App builds one a frame (App::frameSources).
#pragma once
#include <memory>
#include <vector>

#include "gfx/command_list.hpp"
#include "render/scene_views.hpp"
#include "render/view_cameras.hpp"
#include "ui/fe_scene.hpp"

class FeShell;
class FrameCopies;
class ModelBuilder;
class MoviePlayer;
class RaceEggs;
class Renderer;
class SceneHost;
class SceneRaceView;
class SceneRecorder;
class StoryScene;
struct ModelClock;
struct SceneSources;
struct SceneToggles;
struct Track;
namespace argon
{
class EngineTrails;
}

// the front end's page models, drawn from the shell's ImGui callbacks with this frame's scene recorder
struct FrontEndModels
{
    SceneRecorder *recorder = nullptr;
    FeScene scene;
    float aspect = 1.f;
    std::vector<FeScene> boxes;
};

struct FrameSources
{
    Renderer &renderer;
    const int &frame;                              // the frame in flight
    const SceneToggles &toggles;
    const Track &track;                            // its fog colour, the clear
    const RaceEggs &raceEggs;                      // Atlantis Mode's black clear
    const std::unique_ptr<StoryScene> &story;      // a story scene's light
    const std::unique_ptr<MoviePlayer> &movie;     // its newest frame uploaded
    FrameCopies &frameCopies;                      // the transition's copy of the finished frame
    FeShell &fe;                                   // the page's models and lights
    ModelBuilder &models;                          // their poses
    const argon::EngineTrails &engineTrails;
    bool trailsShown;                              // the trails drawn this frame: on, driving, the race stepping
    const ViewCameras &cameras;
    SceneHost &scene;                              // the frame the scene records into (app\scene_frame.hpp)
    SceneRaceView &raceView;                       // what the scene recorder asks about the race (App)
    int &view;                                     // the view being recorded: SceneHost::viewSlot reads it
    HudCamera &hudCamera;                          // the first view's camera, for what draws after the views
    FrontEndModels &menuModels;                    // set while the frame records, for the shell's callbacks
};

// what only App can do for the frame
class FrameHost
{
  public:
    virtual void recordShadows(argon::gfx::CommandList &commands) = 0;   // the ship shadows' maps
    virtual SceneSources sceneSources() = 0;                            // what the frame's SceneRecorder draws from
    virtual ViewMoment viewMoment() const = 0;
    virtual bool frontEndScreenShown() const = 0;                        // a menu page over the scene, not a flyby

  protected:
    ~FrameHost() = default;
};

class FrameRecorder
{
  public:
    FrameRecorder(const FrameSources &sources, FrameHost &host) : sources_(sources), host_(host)
    {
    }
    void record();

  private:
    void recordTrails();
    void recordRaceViews(SceneRecorder &recorder, const ViewMoment &moment);
    void recordView(SceneRecorder &recorder, const RaceView &view, bool split);
    void writeSceneConstants(const glm::mat4 &viewMatrix);
    void writeFrontEndSceneConstants(const FeScene &scene);
    void prepareFrontEndModels(SceneRecorder &recorder);
    argon::gfx::Colour sceneClearColour() const;
    argon::gfx::SceneClear scenePassClear(int viewCount) const;

    FrameSources sources_;
    FrameHost &host_;
};
