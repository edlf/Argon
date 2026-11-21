// Argon: the frame's cameras: the main view's (the free camera, or the shot the drive camera, the loading screen's
// flyby or a story scene sets, with its roll and fov), and in a split-screen race (0x7efd0) one view per human player
// on its own part of the screen, players 2..4 on their chase cameras. Mirror Tracks (0x52c8bc) negates every
// projection's x; Ultra Focal Length widens the chase cameras. What decides the views comes in a ViewMoment each time.
#pragma once
#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "logic/drive_camera.hpp"
#include "logic/split_screen.hpp"
#include "render/scene_views.hpp"

class FreeCamera;
class MultiplayerSession;
class RaceEggs;

struct SceneCamera
{
    glm::mat4 view{1.f}, proj{1.f};
    float fovDeg = 70.f;
};

// a camera and its part of the screen
struct RaceView
{
    SceneCamera camera;
    argon::split::ScreenRect area;
};

// what decides this frame's views
struct ViewMoment
{
    bool shotOwnsView = false;                 // the main view on ViewCameraSources::viewShot, not the free camera
    bool raceViewsShown = false;               // a race on screen: a view per human player
    std::array<ViewShot, 3> playerShots{};     // players 2..4: their chase cameras between the last two steps
};

struct ViewCameraSources
{
    const FreeCamera &freeCamera;
    const ViewShot &viewShot;           // the drive camera's, the intro's or a flyby's shot
    const RaceEggs &raceEggs;           // Mirror Tracks, Ultra Focal Length
    const MultiplayerSession &multiplayer;
};

class ViewCameras
{
  public:
    static constexpr float kNear = 0.3f, kFar = 30000.f;
    static constexpr float kChaseFovDeg = 60.f;   // every tripod is created with fov 60 (+0xcc4)

    explicit ViewCameras(const ViewCameraSources &sources) : sources_(sources)
    {
    }

    // the main view's camera at that aspect
    SceneCamera sceneCamera(const ViewMoment &moment, float aspect) const;
    // one view a human player in a race, else the one on the whole screen
    int viewCount(const ViewMoment &moment) const;
    // a player's camera on its part of the window, its projection at that part's own aspect
    RaceView raceView(const ViewMoment &moment, int player, const argon::gfx::WindowSize &window) const;
    std::vector<RaceView> raceViews(const ViewMoment &moment, const argon::gfx::WindowSize &window) const;
    // what the HUD's projected markers, the speed blur and the sky read of a view
    static HudCamera hudCameraOf(const SceneCamera &camera);

  private:
    SceneCamera chaseCamera(const ViewShot &shot, float aspect) const;
    glm::mat4 projection(float fovDeg, float aspect) const;

    ViewCameraSources sources_;
};
