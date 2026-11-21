// Argon: tests for the flares' occlusion rule (render\flare_occlusion.hpp: cmm_draw_mesh 0xb6e60, 0xa64f0 / 0xa6750).
// Run with test.bat / test.sh.
#include "render/flare_occlusion.hpp"
#include "check.hpp"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

void untestedFlareIsNotDrawn()
{
    static_assert(argon::flare::kUntestedIntensity <= 0.f);
}

void hiddenPointsGiveZero()
{
    CHECK(argon::flare::intensity(argon::flare::visiblePoints(0, 4, 4), 4) == 0.f);
}

void halfThePointsVisibleIsFull()
{
    CHECK(argon::flare::intensity(argon::flare::visiblePoints(8, 4, 4), 4) == 1.f);
}

void onePointOfFourIsHalf()
{
    CHECK(argon::flare::intensity(argon::flare::visiblePoints(4, 4, 4), 4) == 0.5f);
}

void allPointsVisibleStaysAtOne()
{
    CHECK(argon::flare::intensity(argon::flare::visiblePoints(32 * 4, 4, 32), 32) == 1.f);
}

void samplesCountAsPartPoints()
{
    CHECK(argon::flare::visiblePoints(1, 4, 4) == 0.25f);
}

void anySampleQueryCountsAllPoints()
{
    CHECK(argon::flare::visiblePoints(1, 0, 32) == 32.f);
}

void anySampleQueryWithoutSamplesIsZero()
{
    CHECK(argon::flare::visiblePoints(0, 0, 32) == 0.f);
}

void zeroPointsDoNotDivideByZero()
{
    CHECK(argon::flare::intensity(0.f, 0) == 0.f);
}

void fullyVisibleWhiteTintIsOne()
{
    const float channel = argon::flare::tintedChannel(255, 1.f);

    CHECK(channel == 1.f);
}

void hiddenTintIsBlack()
{
    const float channel = argon::flare::tintedChannel(255, 0.f);

    CHECK(channel == 0.f);
}

void tintedChannelRoundsHalfToEven()
{
    const float channel = argon::flare::tintedChannel(255, 0.5f);

    CHECK(channel == 128.f / 255.f);
}

void tintedChannelScalesTheTintByte()
{
    const float channel = argon::flare::tintedChannel(128, 0.25f);

    CHECK(channel == 32.f / 255.f);
}
void testPointsAreTheGamesCount()
{
    const std::vector<glm::vec3> points = argon::flare::testPoints();

    CHECK(points.size() == size_t(argon::flare::kTestPoints));
}

void testPointsLieInTheUnitSphere()
{
    const std::vector<glm::vec3> points = argon::flare::testPoints();

    bool inside = true;
    for (const glm::vec3 &point : points)
    {
        inside &= glm::dot(point, point) <= 1.f;
    }
    CHECK(inside);
}

void testPointsReachEveryOctant()
{
    const std::vector<glm::vec3> points = argon::flare::testPoints();

    bool octants[8]{};
    for (const glm::vec3 &point : points)
    {
        octants[(point.x < 0.f ? 1 : 0) | (point.y < 0.f ? 2 : 0) | (point.z < 0.f ? 4 : 0)] = true;
    }
    CHECK(std::count(std::begin(octants), std::end(octants), true) == 8);
}

void testPointsAreTheSameEachTime()
{
    const std::vector<glm::vec3> first = argon::flare::testPoints();

    const std::vector<glm::vec3> second = argon::flare::testPoints();

    CHECK(first == second);
}

}  // namespace

int main()
{
    untestedFlareIsNotDrawn();
    hiddenPointsGiveZero();
    halfThePointsVisibleIsFull();
    onePointOfFourIsHalf();
    allPointsVisibleStaysAtOne();
    samplesCountAsPartPoints();
    anySampleQueryCountsAllPoints();
    anySampleQueryWithoutSamplesIsZero();
    zeroPointsDoNotDivideByZero();
    fullyVisibleWhiteTintIsOne();
    hiddenTintIsBlack();
    tintedChannelRoundsHalfToEven();
    tintedChannelScalesTheTintByte();
    testPointsAreTheGamesCount();
    testPointsLieInTheUnitSphere();
    testPointsReachEveryOctant();
    testPointsAreTheSameEachTime();
    std::printf("flare_occlusion_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
