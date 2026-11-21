// Argon: tests for the meshes placed by the camera (render\camera_facing.hpp: the render list 0xb7e72, cmm_draw_mesh
// 0xb7765, the look-at frame 0xb4e70). Run with test.bat / test.sh.
#include "render/camera_facing.hpp"
#include "check.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

const float kTolerance = 1e-4f;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < kTolerance;
}

// an eye at (0, 0, 10) looking down -z
argon::ViewPoint eyeOnZ()
{
    return argon::ViewPoint::of(glm::lookAt(glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));
}

glm::mat4 placedAt(const glm::vec3 &position)
{
    return glm::translate(glm::mat4(1.f), position);
}

void viewPointHasTheEye()
{
    CHECK(near(eyeOnZ().eye, glm::vec3(0, 0, 10)));
}

void viewPointHasTheViewAxis()
{
    CHECK(std::abs(std::abs(eyeOnZ().axis.z) - 1.f) < kTolerance);
}

void factorOneKeepsThePosition()
{
    CHECK(near(argon::billboardPosition(glm::vec3(3, 4, -20), eyeOnZ(), 1.f), glm::vec3(3, 4, -20)));
}

void factorZeroPutsItOnTheViewAxis()
{
    CHECK(near(argon::billboardPosition(glm::vec3(3, 4, -20), eyeOnZ(), 0.f), glm::vec3(0, 0, -20)));
}

void negativeFactorCrossesTheViewAxis()
{
    CHECK(near(argon::billboardPosition(glm::vec3(4, 2, -20), eyeOnZ(), -0.5f), glm::vec3(-2, -1, -20)));
}

void facingPointsZAwayFromTheEye()
{
    const glm::mat4 faced = argon::facingEye(placedAt(glm::vec3(10, 0, 10)), glm::vec3(0, 0, 10));
    CHECK(near(glm::vec3(faced[2]), glm::vec3(1, 0, 0)));
}

void facingKeepsXLevel()
{
    const glm::mat4 faced = argon::facingEye(placedAt(glm::vec3(10, 5, 3)), glm::vec3(0, 0, 0));
    CHECK(std::abs(faced[0].y) < kTolerance);
}

void facingIsAUnitRotation()
{
    const glm::mat4 scaled = glm::scale(placedAt(glm::vec3(10, 5, 3)), glm::vec3(3.f));
    const glm::mat3 axes(argon::facingEye(scaled, glm::vec3(0, 0, 0)));
    CHECK(std::abs(glm::determinant(axes) - 1.f) < kTolerance);
}

void facingKeepsThePosition()
{
    CHECK(near(glm::vec3(argon::facingEye(placedAt(glm::vec3(10, 5, 3)), glm::vec3(0))[3]), glm::vec3(10, 5, 3)));
}

void facingStraightUpIsLeftAsItIs()
{
    const glm::mat4 world = glm::scale(placedAt(glm::vec3(0, 50, 0)), glm::vec3(2.f));
    CHECK(argon::facingEye(world, glm::vec3(0)) == world);
}

void followingCameraAddsTheEye()
{
    argon::CameraPlacement placement;
    placement.followsCamera = true;
    CHECK(near(glm::vec3(placement.listed(placedAt(glm::vec3(1, 2, 3)), eyeOnZ())[3]), glm::vec3(1, 2, 13)));
}

void billboardIsNotFacedInTheList()
{
    argon::CameraPlacement placement;
    placement.facesCamera = placement.billboard = true;
    const glm::mat4 world = placedAt(glm::vec3(5, 0, -20));
    CHECK(placement.listed(world, eyeOnZ()) == world);
}

void billboardIsFacedAfterItMoves()
{
    argon::CameraPlacement placement;
    placement.facesCamera = placement.billboard = true;
    placement.billboardFactor = 0.f;
    const glm::mat4 drawn = placement.drawn(placedAt(glm::vec3(5, 0, -20)), eyeOnZ());
    CHECK(near(glm::vec3(drawn[2]), glm::vec3(0, 0, -1)));
}

void withoutBillboardTheDrawLeavesTheMatrix()
{
    argon::CameraPlacement placement;
    placement.facesCamera = true;
    const glm::mat4 world = placedAt(glm::vec3(5, 0, -20));
    CHECK(placement.drawn(world, eyeOnZ()) == world);
}
} // namespace

int main()
{
    viewPointHasTheEye();
    viewPointHasTheViewAxis();
    factorOneKeepsThePosition();
    factorZeroPutsItOnTheViewAxis();
    negativeFactorCrossesTheViewAxis();
    facingPointsZAwayFromTheEye();
    facingKeepsXLevel();
    facingIsAUnitRotation();
    facingKeepsThePosition();
    facingStraightUpIsLeftAsItIs();
    followingCameraAddsTheEye();
    billboardIsNotFacedInTheList();
    billboardIsFacedAfterItMoves();
    withoutBillboardTheDrawLeavesTheMatrix();
    std::printf("camera_facing_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
