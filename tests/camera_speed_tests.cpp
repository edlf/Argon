// Argon: tests for the speed blur's speed (render\camera_speed.hpp). Run with test.bat / test.sh.
#include "check.hpp"
#include "render/camera_speed.hpp"

#include <cmath>

namespace
{

using argon::test::checks;
using argon::test::failures;

HudCamera cameraAt(float z)
{
    HudCamera camera;
    camera.eye = {0.f, 0.f, z};
    camera.forward = {0.f, 0.f, 1.f};
    return camera;
}

void theFirstFrameHasNoSpeed()
{
    CameraSpeed speed;

    const float first = speed.measured(cameraAt(0.f), 1.0);

    CHECK(first == 0.f);
}

void forwardMovementIsTheDistanceOverTheTime()
{
    CameraSpeed speed;
    speed.measured(cameraAt(0.f), 1.0);

    const float moving = speed.measured(cameraAt(5.f), 1.5);

    CHECK(std::abs(moving - 10.f) < 1e-4f);
}

void movingBackwardsIsNegative()
{
    CameraSpeed speed;
    speed.measured(cameraAt(5.f), 1.0);

    const float backing = speed.measured(cameraAt(0.f), 1.5);

    CHECK(backing < 0.f);
}

void theSmoothedSpeedEasesIn()
{
    CameraSpeed speed;
    speed.smoothed(cameraAt(0.f), 0.5f, 1.0);

    const float eased = speed.smoothed(cameraAt(5.f), 0.5f, 1.5);

    CHECK(eased > 0.f && eased < 10.f);
}

}  // namespace

int main()
{
    theFirstFrameHasNoSpeed();
    forwardMovementIsTheDistanceOverTheTime();
    movingBackwardsIsNegative();
    theSmoothedSpeedEasesIn();
    std::printf("camera_speed_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
