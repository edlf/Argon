// Argon: the frame's cameras (render\view_cameras.hpp; camera_set_projection 0xae5c0, camera_set_fov 0xae550, the
// split screen's views 0x7efd0).
#include "render/view_cameras.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "logic/free_camera.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/race_eggs.hpp"
#include "render/projection.hpp"

namespace
{
constexpr float kUltraFocalWidening = 1.8f;
}  // namespace

glm::mat4 ViewCameras::projection(float fovDeg, float aspect) const
{
    glm::mat4 proj = argon::reversedPerspective(glm::radians(fovDeg), aspect, kNear, kFar);
    if (sources_.raceEggs.on(Egg::MirrorTracks))
    {
        proj[0][0] = -proj[0][0];
    }
    return proj;
}

SceneCamera ViewCameras::sceneCamera(const ViewMoment &moment, float aspect) const
{
    const ViewShot &shot = sources_.viewShot;
    SceneCamera camera;
    camera.fovDeg = moment.shotOwnsView ? shot.fov : sources_.freeCamera.fov;
    camera.proj = projection(camera.fovDeg, aspect);
    camera.view = moment.shotOwnsView ? glm::lookAt(shot.eye, shot.look, shot.up) : sources_.freeCamera.view();
    return camera;
}

SceneCamera ViewCameras::chaseCamera(const ViewShot &shot, float aspect) const
{
    SceneCamera camera;
    camera.fovDeg = sources_.raceEggs.on(Egg::UltraFocalLength) ? shot.fov * kUltraFocalWidening : shot.fov;
    camera.proj = projection(camera.fovDeg, aspect);
    camera.view = glm::lookAt(shot.eye, shot.look, shot.up);
    return camera;
}

int ViewCameras::viewCount(const ViewMoment &moment) const
{
    return moment.raceViewsShown ? sources_.multiplayer.players() : 1;
}

RaceView ViewCameras::raceView(const ViewMoment &moment, int player, const argon::gfx::WindowSize &window) const
{
    const argon::split::ScreenRect area = argon::split::viewOf(viewCount(moment), player);
    const float aspect = viewAspect(window, area);
    if (player == 0)
    {
        return {sceneCamera(moment, aspect), area};
    }
    return {chaseCamera(moment.playerShots[size_t(player - 1)], aspect), area};
}

std::vector<RaceView> ViewCameras::raceViews(const ViewMoment &moment, const argon::gfx::WindowSize &window) const
{
    std::vector<RaceView> views;
    const int count = viewCount(moment);
    views.reserve(size_t(count));
    for (int player = 0; player < count; ++player)
    {
        views.push_back(raceView(moment, player, window));
    }
    return views;
}

HudCamera ViewCameras::hudCameraOf(const SceneCamera &camera)
{
    return HudCamera{camera.proj * camera.view, glm::vec3(glm::inverse(camera.view)[3]), -glm::vec3(camera.view[0][2], camera.view[1][2], camera.view[2][2])};
}
