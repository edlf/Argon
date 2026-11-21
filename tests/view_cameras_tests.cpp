// Argon: tests for the frame's cameras (render/view_cameras.hpp: the main view, the split screen's views, Mirror
// Tracks and Ultra Focal Length). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "render/view_cameras.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>

#include "logic/free_camera.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/race_eggs.hpp"

namespace
{
using argon::test::checks;
using argon::test::failures;

const argon::gfx::WindowSize kWindow{1280, 720};

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

bool same(const glm::mat4 &a, const glm::mat4 &b)
{
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!near(a[column][row], b[column][row]))
            {
                return false;
            }
        }
    }
    return true;
}

// the projection's own width over height: its y scale over its x scale
float projectionAspect(const SceneCamera &camera)
{
    return std::abs(camera.proj[1][1] / camera.proj[0][0]);
}

RaceEggs eggsWith(Egg egg)
{
    bool on[RaceEggs::kCount]{};
    on[size_t(egg)] = true;
    RaceEggs eggs;
    eggs.switchOn(on);
    return eggs;
}

ViewShot shotAt(const glm::vec3 &eye, float fov)
{
    return ViewShot{eye, eye + glm::vec3(0, 0, 10), glm::vec3(0, 1, 0), fov};
}

// a free camera, a drive camera's shot, the eggs and the session the cameras read
struct Scene
{
    FreeCamera freeCamera;
    ViewShot viewShot = shotAt(glm::vec3(5, 6, 7), 45.f);
    RaceEggs eggs;
    MultiplayerSession multiplayer;

    ViewCameras cameras() const
    {
        return ViewCameras(ViewCameraSources{freeCamera, viewShot, eggs, multiplayer});
    }
};

ViewMoment raceOf(bool shotOwnsView)
{
    ViewMoment moment;
    moment.shotOwnsView = shotOwnsView;
    moment.raceViewsShown = true;
    moment.playerShots[0] = shotAt(glm::vec3(100, 0, 0), ViewCameras::kChaseFovDeg);
    return moment;
}

void testTheFreeCameraOwnsTheViewWithoutAShot()
{
    const Scene scene;
    const SceneCamera camera = scene.cameras().sceneCamera(ViewMoment{}, 16.f / 9.f);
    CHECK(same(camera.view, scene.freeCamera.view()) && camera.fovDeg == scene.freeCamera.fov);
}

void testAShotOwnsTheViewAndItsFov()
{
    const Scene scene;
    const SceneCamera camera = scene.cameras().sceneCamera(raceOf(true), 16.f / 9.f);
    const glm::mat4 expected = glm::lookAt(scene.viewShot.eye, scene.viewShot.look, scene.viewShot.up);
    CHECK(same(camera.view, expected) && camera.fovDeg == 45.f);
}

void testMirrorTracksNegatesTheProjectionsX()
{
    Scene plain;
    Scene mirrored;
    mirrored.eggs = eggsWith(Egg::MirrorTracks);
    const float x = plain.cameras().sceneCamera(ViewMoment{}, 1.5f).proj[0][0];
    CHECK(near(mirrored.cameras().sceneCamera(ViewMoment{}, 1.5f).proj[0][0], -x));
}

void testOneViewOutsideARace()
{
    Scene scene;
    scene.multiplayer.start(2);
    CHECK(scene.cameras().viewCount(ViewMoment{}) == 1);
}

void testAViewAHumanPlayerInARace()
{
    Scene scene;
    scene.multiplayer.start(3);
    CHECK(scene.cameras().raceViews(raceOf(false), kWindow).size() == 3);
}

void testTheSecondPlayerRidesItsChaseCamera()
{
    Scene scene;
    scene.multiplayer.start(2);
    const SceneCamera camera = scene.cameras().raceView(raceOf(false), 1, kWindow).camera;
    CHECK(near(ViewCameras::hudCameraOf(camera).eye.x, 100.f) && camera.fovDeg == ViewCameras::kChaseFovDeg);
}

void testUltraFocalLengthWidensTheChaseCameras()
{
    Scene scene;
    scene.eggs = eggsWith(Egg::UltraFocalLength);
    scene.multiplayer.start(2);
    CHECK(near(scene.cameras().raceView(raceOf(false), 1, kWindow).camera.fovDeg, ViewCameras::kChaseFovDeg * 1.8f));
}

void testUltraFocalLengthLeavesTheMainViewsFov()
{
    Scene scene;
    scene.eggs = eggsWith(Egg::UltraFocalLength);
    CHECK(scene.cameras().sceneCamera(raceOf(true), 1.f).fovDeg == 45.f);
}

void testEachSplitViewTakesItsAreasAspect()
{
    Scene scene;
    scene.multiplayer.start(2);
    const RaceView top = scene.cameras().raceView(raceOf(false), 0, kWindow);
    CHECK(near(projectionAspect(top.camera), viewAspect(kWindow, argon::split::viewOf(2, 0))));
}

void testTheHudCameraLooksAlongTheView()
{
    const Scene scene;
    const HudCamera hud = ViewCameras::hudCameraOf(scene.cameras().sceneCamera(raceOf(true), 1.f));
    CHECK(near(hud.eye.z, 7.f) && near(hud.forward.z, 1.f));
}
}  // namespace

int main()
{
    testTheFreeCameraOwnsTheViewWithoutAShot();
    testAShotOwnsTheViewAndItsFov();
    testMirrorTracksNegatesTheProjectionsX();
    testOneViewOutsideARace();
    testAViewAHumanPlayerInARace();
    testTheSecondPlayerRidesItsChaseCamera();
    testUltraFocalLengthWidensTheChaseCameras();
    testUltraFocalLengthLeavesTheMainViewsFov();
    testEachSplitViewTakesItsAreasAspect();
    testTheHudCameraLooksAlongTheView();
    std::printf("view_cameras_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
