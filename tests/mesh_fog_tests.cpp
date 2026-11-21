// Argon: tests for a mesh's fog as the surface setup sets it (render/mesh_fog.hpp). No window or GPU: run with
// test.bat (or ctest in build\).
#include "render/mesh_fog.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::fog::Curve;
using argon::fog::Source;

constexpr uint32_t kOwnFog = argon::original::surface_flags::kOwnFog;
constexpr uint32_t kNoFog = argon::original::surface_flags::kNoFog;

void testPlainSurfaceTakesTheTracksFog()
{
    CHECK(argon::fog::meshFog(0x1b, 3, 0xff00ff00u, 0.5f, 100.f).source == Source::Track);
}

void testNoFogFlagTurnsTheFogOff()
{
    CHECK(argon::fog::meshFog(kNoFog | kOwnFog, 3, 0, 0.f, 0.f).curve == Curve::Off);
}

void testOwnFogTakesTheBlocksColour()
{
    CHECK(argon::fog::meshFog(kOwnFog, 3, 0x00102030u, 0.f, 0.f).colour == 0x00102030u);
}

void testOwnFogTakesTheBlocksDensityAndRange()
{
    const argon::fog::MeshFog fog = argon::fog::meshFog(kOwnFog, 1, 0, 0.25f, 800.f);

    CHECK(fog.density == 0.25f && fog.range == 800.f);
}

void testOwnFogModeZeroIsOff()
{
    CHECK(argon::fog::meshFog(kOwnFog, 0, 0, 0.0007f, 0.f).curve == Curve::Off);
}

void testModesOneAndTwoAreExp()
{
    CHECK(argon::fog::curveOf(1) == Curve::Exp && argon::fog::curveOf(2) == Curve::Exp);
}

void testModeThreeIsLinear()
{
    CHECK(argon::fog::curveOf(3) == Curve::Linear);
}
}  // namespace

int main()
{
    testPlainSurfaceTakesTheTracksFog();
    testNoFogFlagTurnsTheFogOff();
    testOwnFogTakesTheBlocksColour();
    testOwnFogTakesTheBlocksDensityAndRange();
    testOwnFogModeZeroIsOff();
    testModesOneAndTwoAreExp();
    testModeThreeIsLinear();
    std::printf("mesh_fog_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
