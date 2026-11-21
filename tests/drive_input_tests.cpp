// Argon: tests for the drive views (logic\drive_camera.hpp) and what a gamepad's state means (input\pad_mapping.hpp).
// No window or GPU: run with test.bat (or ctest in build\).
#include "check.hpp"
#include "input/pad_mapping.hpp"
#include "logic/drive_camera.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-3f;
}

// a ship at `at` heading along +z, level
glm::mat4 shipAt(const glm::vec3 &at)
{
    glm::mat4 ship(1.f);
    ship[3] = glm::vec4(at, 1.f);
    return ship;
}

argon::TrackCamera tvCamera(const std::string &type, float panFrom, float panTo)
{
    argon::TrackCamera camera;
    camera.type = type;
    camera.panFrom = panFrom;
    camera.panTo = panTo;
    return camera;
}

// ---- the drive views

void testTripodViewsMapToTheCraftsTripods()
{
    DriveCamera camera;
    camera.choose(DriveCamera::kCockpit);
    const int cockpit = camera.tripodIndex();
    camera.choose(1);
    const int close = camera.tripodIndex();
    camera.choose(DriveCamera::kTv);
    CHECK(cockpit == 3 && camera.internalTripod() == false && close == 0 && camera.tripodIndex() == 2);
}

void testTvPicksTheFirstCameraCoveringTheLap()
{
    DriveCamera camera;
    const std::vector<argon::TrackCamera> cameras{tvCamera("moving", 50.f, 80.f), tvCamera("heli", 0.f, 40.f), tvCamera("heli", 0.f, 90.f)};
    ViewShot shot;
    camera.tvShot(cameras, 20.f, shipAt({0, 0, 0}), shot);
    CHECK(camera.tvCamera() == 1);
}

void testTvKeepsItsCameraWhileItCovers()
{
    DriveCamera camera;
    const std::vector<argon::TrackCamera> cameras{tvCamera("heli", 0.f, 90.f), tvCamera("heli", 0.f, 40.f)};
    ViewShot shot;
    camera.tvShot(cameras, 20.f, shipAt({0, 0, 0}), shot);
    camera.tvShot(cameras, 60.f, shipAt({0, 0, 0}), shot);
    CHECK(camera.tvCamera() == 0);
}

void testTvWithoutACoveringCameraShowsNone()
{
    DriveCamera camera;
    const std::vector<argon::TrackCamera> cameras{tvCamera("heli", 0.f, 40.f), tvCamera("ship", 0.f, 100.f)};
    ViewShot shot;
    CHECK(!camera.tvShot(cameras, 60.f, shipAt({0, 0, 0}), shot) && camera.tvCamera() == -1);
}

void testStaticTvCameraCoversOnlyWithinItsRange()
{
    DriveCamera camera;
    argon::TrackCamera fixed = tvCamera("static", 0.f, 100.f);
    fixed.range = 50.f;
    ViewShot shot;
    CHECK(!camera.tvShot({fixed}, 10.f, shipAt({0, 0, 80}), shot) && camera.tvShot({fixed}, 10.f, shipAt({0, 0, 30}), shot));
}

void testMovingTvCameraPansWithTheLap()
{
    DriveCamera camera;
    argon::TrackCamera moving = tvCamera("moving", 0.f, 50.f);
    moving.pos = {0, 0, 0};
    moving.pos2 = {10, 0, 0};
    moving.fov = 40.f;
    moving.fov2 = 80.f;
    ViewShot shot;
    camera.tvShot({moving}, 25.f, shipAt({0, 0, 0}), shot);
    CHECK(near(shot.eye, {5, 0, 0}) && std::abs(shot.fov - 60.f) < 1e-3f);
}

void testChosenViewStartsTheTvAgain()
{
    DriveCamera camera;
    ViewShot shot;
    camera.tvShot({tvCamera("heli", 0.f, 100.f)}, 10.f, shipAt({0, 0, 0}), shot);
    camera.choose(DriveCamera::kTv);
    CHECK(camera.tvCamera() == -1);
}

void testShipMountedCameraIsInShipSpace()
{
    DriveCamera camera;
    argon::TrackCamera top = tvCamera("ship", 0.f, 100.f);
    top.pos = {-4, 0, 1};   // x forward, y left, z up: "Top behind"
    top.target = {10, 0, 0};
    camera.choose(DriveCamera::kFirstShipMounted);
    const ViewShot shot = camera.shipMountedShot({tvCamera("heli", 0.f, 1.f), top}, shipAt({0, 0, 0}));
    CHECK(near(shot.eye, {0, 1, -4}) && near(shot.look, {0, 0, 10}));
}

void testShipMountedWithoutCamerasLooksFromBehind()
{
    const DriveCamera camera;
    CHECK(near(camera.shipMountedShot({}, shipAt({0, 0, 0})).eye, {0, 2, -8}));
}

void testChaseEasesTowardsBehindAndAbove()
{
    const glm::vec3 wanted(0, 6.f, -15.f);
    const ViewShot settled = DriveCamera::chaseShot(shipAt({0, 0, 0}), wanted, 1.f / 60.f, 70.f);
    const ViewShot moving = DriveCamera::chaseShot(shipAt({0, 0, 0}), {0, 0, 0}, 1.f / 60.f, 70.f);
    CHECK(near(settled.eye, wanted) && moving.eye.z < 0.f && moving.eye.z > -15.f);
}

// ---- the gamepad

GLFWgamepadstate restingPad()
{
    GLFWgamepadstate state{};
    state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = -1.f;
    state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.f;
    return state;
}

void testStickInsideTheDeadZoneDoesNotSteer()
{
    GLFWgamepadstate state = restingPad();
    state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = 0.3f;
    argon::GameCraft::Pad pad;
    argon::input::addGamepad(pad, state);
    CHECK(pad.steer == 0.f);
}

void testFullStickSteersFully()
{
    GLFWgamepadstate state = restingPad();
    state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] = -1.f;
    argon::GameCraft::Pad pad;
    argon::input::addGamepad(pad, state);
    CHECK(std::abs(pad.steer + 1.f) < 1e-5f);
}

void testDpadUpDives()
{
    GLFWgamepadstate state = restingPad();
    state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] = GLFW_PRESS;
    argon::GameCraft::Pad pad;
    argon::input::addGamepad(pad, state);
    CHECK(pad.dive == 1.f && pad.climb == 0.f);
}

void testLeftTriggerBrakesOnTheSideSteeredTowards()
{
    GLFWgamepadstate state = restingPad();
    state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = GLFW_PRESS;
    state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = 1.f;
    argon::GameCraft::Pad pad;
    argon::input::addGamepad(pad, state);
    CHECK(pad.airRight == 1.f && pad.airLeft == 0.f);
}

void testMirrorSwapsLeftAndRight()
{
    argon::GameCraft::Pad pad;
    pad.steer = 0.5f;
    pad.airLeft = 1.f;
    argon::input::mirrorSteering(pad);
    CHECK(pad.steer == -0.5f && pad.airRight == 1.f && pad.airLeft == 0.f);
}

void testStartCountsAsTheMenusA()
{
    GLFWgamepadstate state = restingPad();
    state.buttons[GLFW_GAMEPAD_BUTTON_START] = GLFW_PRESS;
    CHECK(argon::input::menuButtonsHeld(state).a);
}

void testHeldButtonPressesOnce()
{
    argon::input::PressEdges edges;
    menu::MenuInput held;
    held.b = true;
    const bool first = edges.step(held).b;
    const bool second = edges.step(held).b;
    CHECK(first && !second);
}

void testRestingPadHoldsNothing()
{
    GLFWgamepadstate pulled = restingPad();
    pulled.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = 0.f;
    CHECK(!argon::input::anyButtonHeld(restingPad()) && argon::input::anyButtonHeld(pulled));
}
}  // namespace

int main()
{
    testTripodViewsMapToTheCraftsTripods();
    testTvPicksTheFirstCameraCoveringTheLap();
    testTvKeepsItsCameraWhileItCovers();
    testTvWithoutACoveringCameraShowsNone();
    testStaticTvCameraCoversOnlyWithinItsRange();
    testMovingTvCameraPansWithTheLap();
    testChosenViewStartsTheTvAgain();
    testShipMountedCameraIsInShipSpace();
    testShipMountedWithoutCamerasLooksFromBehind();
    testChaseEasesTowardsBehindAndAbove();
    testStickInsideTheDeadZoneDoesNotSteer();
    testFullStickSteersFully();
    testDpadUpDives();
    testLeftTriggerBrakesOnTheSideSteeredTowards();
    testMirrorSwapsLeftAndRight();
    testStartCountsAsTheMenusA();
    testHeldButtonPressesOnce();
    testRestingPadHoldsNothing();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
