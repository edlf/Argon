// Argon: tests for the crafts' engine trails (effects/engine_trails.hpp: 0x50960, 0x9db10, 0x9dca0) and water spray
// (effects/wake_spray.hpp: 0x50960's wake, 0x9c540, 0x9d2c0). No window or GPU: run with test.bat / test.sh (or ctest
// in build).
#include "effects/engine_trails.hpp"
#include "effects/wake_spray.hpp"
#include "model/cmm.hpp"
#include "check.hpp"

#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon;

const TrailDrive kCruise{100.f, false};
const float kStep = 1.f / 60.f;

// water everywhere, its surface at the query's height
class FlatWater final : public WaterQuery
{
  public:
    std::optional<float> depthAt(const glm::vec3 &) const override
    {
        return 0.f;
    }
};

glm::mat4 craftAt(float z)
{
    glm::mat4 matrix(1.f);
    matrix[3] = glm::vec4(0.f, 0.f, z, 1.f);
    return matrix;
}

uint32_t ribbonsAfter(int updates, const TrailDrive &drive)
{
    EngineTrails trails;
    for (int i = 0; i < updates; ++i)
    {
        trails.follow(0, craftAt(float(i)), drive);
    }
    std::vector<Vertex> out(EngineTrails::kMaxVertices);
    return trails.writeRibbons(out.data());
}

void testAFreshTrailDrawsNothing()
{
    CHECK(ribbonsAfter(0, kCruise) == 0);
}

void testATrailLightsOnlyPointsWrittenThreeUpdatesLate()
{
    CHECK(ribbonsAfter(3, kCruise) == 0);
}

void testATrailDrawsTwoRibbonsPerLitSegmentOnBothSides()
{
    // after 5 updates the points 4 and 3 updates back are lit; a segment draws when either end is lit, so the two
    // segments touching the newer one draw: 2 segments x 2 sides x 2 ribbons x 6 vertices
    CHECK(ribbonsAfter(5, kCruise) == 2 * 2 * 2 * 6);
}

void testAStillCraftLightsNoTrail()
{
    CHECK(ribbonsAfter(10, TrailDrive{0.f, false}) == 0);
}

void testResetClearsTheTrails()
{
    EngineTrails trails;
    for (int i = 0; i < 10; ++i)
    {
        trails.follow(0, craftAt(float(i)), kCruise);
    }
    trails.reset();
    std::vector<Vertex> out(EngineTrails::kMaxVertices);
    CHECK(trails.writeRibbons(out.data()) == 0);
}

void followInWater(WakeSpray &spray, const glm::mat4 &matrix, float now)
{
    static ViewerRand rand;
    static const FlatWater water;
    spray.follow(WakeCraft{0, matrix, true}, WakeStep{now, rand, water});
}

void testTheFirstUpdateInWaterThrowsNoSpray()
{
    WakeSpray spray;
    followInWater(spray, craftAt(0.f), 0.f);
    CHECK(spray.empty());
}

void testMovingThroughWaterThrowsAPuffEachSidePerStep()
{
    WakeSpray spray;
    followInWater(spray, craftAt(0.f), 0.f);
    followInWater(spray, craftAt(3.f), kStep);   // steps at 0 and 2 m
    CHECK(spray.puffs() == 4);
}

void testPuffsGoAtTheEndOfTheirLife()
{
    WakeSpray spray;
    followInWater(spray, craftAt(0.f), 0.f);
    followInWater(spray, craftAt(3.f), 0.f);
    spray.age(2.f);   // the longest life is 1.53 s
    CHECK(spray.empty());
}

void testClearEmptiesTheSpray()
{
    WakeSpray spray;
    followInWater(spray, craftAt(0.f), 0.f);
    followInWater(spray, craftAt(3.f), 0.f);
    spray.clear();
    CHECK(spray.empty());
}
}  // namespace

int main()
{
    testAFreshTrailDrawsNothing();
    testATrailLightsOnlyPointsWrittenThreeUpdatesLate();
    testATrailDrawsTwoRibbonsPerLitSegmentOnBothSides();
    testAStillCraftLightsNoTrail();
    testResetClearsTheTrails();
    testTheFirstUpdateInWaterThrowsNoSpray();
    testMovingThroughWaterThrowsAPuffEachSidePerStep();
    testPuffsGoAtTheEndOfTheirLife();
    testClearEmptiesTheSpray();
    std::printf("race_effects_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
