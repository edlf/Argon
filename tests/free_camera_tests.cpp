// Argon: tests for the viewer's free camera (logic\free_camera.hpp). Run with test.bat / test.sh.
#include "check.hpp"
#include "logic/free_camera.hpp"

#include <cmath>

namespace
{

using argon::test::checks;
using argon::test::failures;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

void yawZeroLooksDownNegativeZ()
{
    FreeCamera camera;
    camera.pitch = 0.f;

    const glm::vec3 ahead = camera.forward();

    CHECK(near(ahead, {0.f, 0.f, -1.f}));
}

void lookAtFromFacesTheTarget()
{
    FreeCamera camera;

    camera.lookAtFrom({10.f, 0.f, 0.f}, {0.f, 0.f, 5.f});

    CHECK(near(camera.position, {10.f, 0.f, 5.f}) && near(camera.forward(), {0.f, 0.f, -1.f}));
}

void aDragTurnsOnlyWhileHeld()
{
    FreeCamera camera;
    camera.yaw = 0.f;
    camera.followCursor({100.0, 100.0}, false);

    camera.followCursor({200.0, 100.0}, true);    // the press: nothing to turn by yet
    camera.followCursor({300.0, 100.0}, true);    // held: 100 pixels to the right

    CHECK(std::abs(camera.yaw - 0.35f) < 1e-5f);
}

void theDragStopsAtTheLimitStraightUp()
{
    FreeCamera camera;
    camera.followCursor({0.0, 0.0}, true);

    camera.followCursor({0.0, -100000.0}, true);

    CHECK(camera.pitch == 1.55f);
}

void theKeysMoveAlongTheViewFasterWithShift()
{
    FreeCamera camera;
    camera.position = {0.f, 0.f, 0.f};
    camera.pitch = 0.f;
    camera.speed = 10.f;

    camera.move({1.f, 0.f, 0.f}, 0.5f, true);

    CHECK(near(camera.position, {0.f, 0.f, -25.f}));
}

}  // namespace

int main()
{
    yawZeroLooksDownNegativeZ();
    lookAtFromFacesTheTarget();
    aDragTurnsOnlyWhileHeld();
    theDragStopsAtTheLimitStraightUp();
    theKeysMoveAlongTheViewFasterWithShift();
    std::printf("free_camera_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
