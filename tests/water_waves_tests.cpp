// Argon: tests for the water's wave heightfield and depth query (logic/water_waves.hpp: 0xcd4f0, 0xce6b0)
// its bump basis (render/water_basis.hpp: 0xcd170) and the wake's walk (logic/water_wake.hpp: 0x50960).
// Run with test.bat / test.sh.
#include "logic/water_wake.hpp"
#include "check.hpp"
#include "logic/water_waves.hpp"
#include "render/water_basis.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

constexpr float kSilverScale = 4.f;

float metres(float stored, float scale)
{
    return stored / 32768.f * scale;
}

float secondsAtGameFrame(int frame)
{
    return float(frame) / 60.f;
}

WaterBox box(float bottom, float top, uint32_t flags = 0)
{
    WaterBox b;
    b.min = {0.f, bottom, 0.f};
    b.max = {256.f, top, 256.f};
    b.flags = flags;
    return b;
}

void firstFrameStartsAtTheSumOfTheCosines()
{   // 5000 cos 0 + 2000 cos 0
    CHECK(WaterWaves(1.f).height(0, 0, 0) == 7000);
}

void wavesRepeatAcrossTheTile()
{
    const WaterWaves waves(1.f);
    CHECK(waves.height(7, 16, 3) == waves.height(7, 0, 3) && waves.height(7, 3, 16) == waves.height(7, 3, 0));
}

void flatWaterHasNoWave()
{
    CHECK(near(WaterWaves(0.f).offsetAt(13.f, 57.f, 2.f), 0.f));
}

void vertexOffsetIsItsShortTimesTheScale()
{
    const WaterWaves waves(kSilverScale);
    CHECK(near(waves.offsetAt(0.f, 0.f, 0.f), metres(7000.f, kSilverScale)));
}

void eightGameFramesMoveOneWaveFrame()
{
    const WaterWaves waves(kSilverScale);
    CHECK(near(waves.offsetAt(0.f, 0.f, secondsAtGameFrame(8)), metres(waves.height(1, 0, 0), kSilverScale)));
}

void framesBlendInEighths()
{
    const WaterWaves waves(kSilverScale);
    const float halfway = (waves.height(0, 2, 5) + waves.height(1, 2, 5)) * 0.5f;
    CHECK(near(waves.offsetAt(16.f, 40.f, secondsAtGameFrame(4)), metres(halfway, kSilverScale)));
}

void cellBlendsAlongX()
{
    const WaterWaves waves(kSilverScale);
    const float halfway = (waves.height(0, 3, 2) + waves.height(0, 4, 2)) * 0.5f;
    CHECK(near(waves.offsetAt(28.f, 16.f, 0.f), metres(halfway, kSilverScale)));
}

void cellBlendsAlongZ()
{
    const WaterWaves waves(kSilverScale);
    const float quarter = waves.height(0, 3, 2) * 0.75f + waves.height(0, 3, 3) * 0.25f;
    CHECK(near(waves.offsetAt(24.f, 18.f, 0.f), metres(quarter, kSilverScale)));
}

void negativeCoordinatesWrapToTheTile()
{
    const WaterWaves waves(kSilverScale);
    CHECK(near(waves.offsetAt(-120.f, -100.f, 1.f), waves.offsetAt(8.f, 28.f, 1.f)));
}

void depthIsMeasuredFromTheWavyTop()
{
    const WaterWaves waves(kSilverScale);
    float depth = 0.f;
    CHECK(waterDepthAt({box(-10.f, 0.f)}, waves, {0.f, -1.f, 0.f}, 0.f, depth) && near(depth, -1.f - metres(7000.f, kSilverScale)));
}

void physicsOnlyBoxesAreFlat()
{
    float depth = 0.f;
    CHECK(waterDepthAt({box(-10.f, 0.f, 1)}, WaterWaves(kSilverScale), {0.f, -1.f, 0.f}, 0.f, depth) && near(depth, -1.f));
}

void pointsBelowTheBottomAreNotInWater()
{
    float depth = 0.f;
    CHECK(!waterDepthAt({box(-10.f, 0.f)}, WaterWaves(kSilverScale), {5.f, -10.5f, 5.f}, 0.f, depth));
}

void pointsHighAboveStillGetADepth()
{   // no upper bound: the callers keep only depth < 0
    float depth = 0.f;
    CHECK(waterDepthAt({box(-10.f, 0.f, 1)}, WaterWaves(kSilverScale), {5.f, 100.f, 5.f}, 0.f, depth) && near(depth, 100.f));
}

void maxEdgeIsOutside()
{
    float depth = 0.f;
    CHECK(!waterDepthAt({box(-10.f, 0.f)}, WaterWaves(kSilverScale), {256.f, -1.f, 5.f}, 0.f, depth));
}

void firstHoldingBoxWins()
{
    float depth = 0.f;
    CHECK(waterDepthAt({box(-10.f, 0.f, 1), box(-10.f, 5.f, 1)}, WaterWaves(), {5.f, -1.f, 5.f}, 0.f, depth) && near(depth, -1.f));
}

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

void flatWaterBasisIsTheGridAxes()
{
    const std::vector<BumpBasis> basis = buildWaterBasis(WaterWaves(0.f), 0);
    CHECK(near(basis[40].tangent, {1, 0, 0}) && near(basis[40].binormal, {0, 0, 1}));
}

void basisNormalIsTangentCrossBinormalUnflipped()
{   // points down on flat water: the reflection does not care about its sign
    const std::vector<BumpBasis> basis = buildWaterBasis(WaterWaves(0.f), 0);
    CHECK(near(basis[40].normal, {0, -1, 0}));
}

void wavyTangentFollowsTheSlope()
{
    const std::vector<BumpBasis> basis = buildWaterBasis(WaterWaves(kSilverScale), 3);
    float sloped = 0.f;
    for (const BumpBasis &b : basis)
    {
        sloped = std::fmax(sloped, std::abs(b.tangent.y));
    }
    CHECK(sloped > 0.01f);
}

void tileEdgesShareTheirBasis()
{   // row ends (0, 5) and (16, 5)
    const std::vector<BumpBasis> basis = buildWaterBasis(WaterWaves(kSilverScale), 3);
    CHECK(near(basis[5 * 17].tangent, basis[5 * 17 + 16].tangent) && near(basis[5 * 17].binormal, basis[5 * 17 + 16].binormal));
}

void slowWakeEmitsNothing()
{   // 0.3 m in a frame (18 m/s): squared 0.09 is not above 0.1
    CHECK(wakeStepDistances({0.3f, 0.f, 0.f}).empty());
}

void wakeWalkStartsAtLastFramesPosition()
{   // 100 m/s at 60 Hz: 1.67 m, one step at the start
    const std::vector<float> steps = wakeStepDistances({0.f, 0.f, 100.f / 60.f});
    CHECK(steps.size() == 1 && steps[0] == 0.f);
}

void wakeWalkStepsEveryTwoMetres()
{
    const std::vector<float> steps = wakeStepDistances({3.f, 0.f, 4.f});
    CHECK(steps == std::vector<float>({0.f, 2.f, 4.f}));
}

void wakeHeightAddsTheDepthAtTheQuery()
{   // base 10.9, the query 0.4 above the surface: 10.9 + 0.4 - 1.5
    CHECK(near(wakeSpriteHeight(10.9f, 10.4f, 0.4f), 9.8f));
}

void wakeHeightOutsideTheBoxesAddsTheQueryHeight()
{   // 0xce6b0 leaves the out value (the query's y) untouched
    CHECK(near(wakeSpriteHeight(10.f, 12.f, std::nullopt), 20.5f));
}

} // namespace

int main()
{
    firstFrameStartsAtTheSumOfTheCosines();
    wavesRepeatAcrossTheTile();
    flatWaterHasNoWave();
    vertexOffsetIsItsShortTimesTheScale();
    eightGameFramesMoveOneWaveFrame();
    framesBlendInEighths();
    cellBlendsAlongX();
    cellBlendsAlongZ();
    negativeCoordinatesWrapToTheTile();
    depthIsMeasuredFromTheWavyTop();
    physicsOnlyBoxesAreFlat();
    pointsBelowTheBottomAreNotInWater();
    pointsHighAboveStillGetADepth();
    maxEdgeIsOutside();
    firstHoldingBoxWins();
    flatWaterBasisIsTheGridAxes();
    basisNormalIsTangentCrossBinormalUnflipped();
    wavyTangentFollowsTheSlope();
    tileEdgesShareTheirBasis();
    slowWakeEmitsNothing();
    wakeWalkStartsAtLastFramesPosition();
    wakeWalkStepsEveryTwoMetres();
    wakeHeightAddsTheDepthAtTheQuery();
    wakeHeightOutsideTheBoxesAddsTheQueryHeight();
    std::printf("water_waves_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
