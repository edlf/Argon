// Argon: tests for the translucent pass's depth lift (render\depth_lift.hpp). Run with test.bat / test.sh.
#include "render/depth_lift.hpp"
#include "check.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

const float kTolerance = 1e-6f;

argon::MeshState translucent()
{
    argon::MeshState state = argon::MeshState::alphaBlended();
    state.translucentPass = true;
    return state;
}

// the scene's reversed projection (render\projection.hpp: zNear maps to 1) looking down -z
glm::mat4 reversedProjection()
{
    return glm::perspective(1.2f, 16.f / 9.f, 30000.f, 0.3f);
}

glm::vec3 projected(const glm::mat4 &mvp, const glm::vec3 &point)
{
    const glm::vec4 clip = mvp * glm::vec4(point, 1.f);
    return glm::vec3(clip) / clip.w;
}

void opaqueMeshIsNotLifted()
{
    CHECK(argon::depthLiftOf(argon::MeshState{}) == 0.f);
}

void translucentMeshIsLifted()
{
    CHECK(argon::depthLiftOf(translucent()) == argon::kTranslucentDepthLift);
}

void laidOverMeshIsLiftedTwiceAsFar()
{
    argon::MeshState state = translucent();
    state.laidOver = true;
    CHECK(argon::depthLiftOf(state) == 2.f * argon::kTranslucentDepthLift);
}

void liftScalesTheDepth()
{
    const glm::vec3 point(3.f, -2.f, -50.f);
    const float lift = argon::kTranslucentDepthLift;
    const float depth = projected(reversedProjection(), point).z;
    const float lifted = projected(argon::liftedDepth(reversedProjection(), lift), point).z;
    CHECK(std::fabs(lifted - depth * (1.f + lift)) < kTolerance);
}

void liftMovesTowardsTheEye()
{
    const glm::vec3 point(0.f, 0.f, -50.f);
    const glm::mat4 lifted = argon::liftedDepth(reversedProjection(), argon::kTranslucentDepthLift);
    CHECK(projected(lifted, point).z > projected(reversedProjection(), point).z);   // reversed: larger is nearer
}

void liftKeepsTheScreenPosition()
{
    const glm::vec3 point(3.f, -2.f, -50.f);
    const glm::vec3 before = projected(reversedProjection(), point);
    const glm::vec3 after = projected(argon::liftedDepth(reversedProjection(), argon::kTranslucentDepthLift), point);
    CHECK(std::fabs(after.x - before.x) < kTolerance && std::fabs(after.y - before.y) < kTolerance);
}
}  // namespace

int main()
{
    opaqueMeshIsNotLifted();
    translucentMeshIsLifted();
    laidOverMeshIsLiftedTwiceAsFar();
    liftScalesTheDepth();
    liftMovesTowardsTheEye();
    liftKeepsTheScreenPosition();
    std::printf("depth_lift_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
