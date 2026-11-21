// Argon: tests for the weapons' explosions (logic/explosion.hpp: the impact bursts 0x65f80, the explosion 0x7e0f0 /
// 0x7de10 / 0x7de80, the knock-out 0x6a8a0, the impact sparks 0x67231 / 0x67e81). No window or GPU: run with test.bat
// / test.sh (or ctest in build).
#include "logic/explosion.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::weapons;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

// weapon.nwp's type 7 (Tara's NUKE): clock 1, duration x 6, size 150, flash 800
ExplosionLook nukeLook()
{
    ExplosionLook look;
    look.clockFactor = 1.f;
    look.durationFactor = 6.f;
    look.sizeFactor = 150.f;
    look.flashFactor = 800.f;
    return look;
}

TripleRoll rollOf(unsigned first, unsigned second, unsigned third)
{
    TripleRoll roll;
    roll.first = first;
    roll.second = second;
    roll.third = third;
    return roll;
}

SparkRoll sparkRollOf(unsigned x, unsigned y, unsigned z, unsigned spread, unsigned life)
{
    SparkRoll roll;
    roll.x = x;
    roll.y = y;
    roll.z = z;
    roll.spread = spread;
    roll.life = life;
    return roll;
}

// ---- types and sizes (0x7e0f0)
void testATypePast15ShowsModel15()
{
    CHECK(explosionModelOf(20) == 15);
}

void testATypeBelow16KeepsItsModel()
{
    CHECK(explosionModelOf(7) == 7);
}

void testABurstAsksForItsSizePercentAsDurationAndSize()
{
    const ExplosionOrder order = burstExplosionOrder(200);
    CHECK(near(order.duration, 2.f) && near(order.size, 2.f));
}

void testTheDurationIsScaledByTheType()
{
    const ExplosionSize size = explosionSize(nukeLook(), burstExplosionOrder(100));
    CHECK(near(size.duration, 6.f));
}

void testTheScaleIsTheSizeTimesTheTypesSizePercent()
{
    const ExplosionSize size = explosionSize(nukeLook(), burstExplosionOrder(100));
    CHECK(near(size.scale, 1.5f));
}

// ---- the bursts' scatter (0x65f80)
void testAZeroScatterLandsOnTheImpact()
{
    CHECK(near(burstScatter(rollOf(0, 0x3ff, 7), 0.f, false), glm::vec3(0.f)));
}

void testTheScatterSpansMinusToPlusItsReach()
{
    const glm::vec3 offset = burstScatter(rollOf(0, 0x200, 0x3ff), 50.f, false);
    CHECK(near(offset, glm::vec3(-50.f, 0.f, 50.f * 511.f / 512.f)));
}

void testAFlatScatterKeepsTheImpactsHeight()
{
    CHECK(near(burstScatter(rollOf(0, 0, 0), 20.f, true).y, 0.f));
}

// ---- the turn (0x7e1e1..0x7e296)
void testAPlainTypeIsNotTurned()
{
    CHECK(near(explosionTurnDegrees(nukeLook(), rollOf(0x3ff, 0x3ff, 0x3ff)), glm::vec3(0.f)));
}

void testATurningTypeTakesTheSecondRollAsItsYawOnly()
{
    ExplosionLook look;
    look.turns = true;
    CHECK(near(explosionTurnDegrees(look, rollOf(0x3ff, 0x3ff, 0x3ff)), glm::vec3(360.f, 0.f, 0.f)));
}

void testATumblingTypeTakesYawPitchAndRoll()
{
    ExplosionLook look;
    look.tumbles = true;
    CHECK(near(explosionTurnDegrees(look, rollOf(0x3ff, 0, 0x3ff)), glm::vec3(0.f, 360.f, 360.f)));
}

void testTheMatrixScalesAndPlacesTheModel()
{
    const glm::mat4 m = explosionMatrix(glm::vec3(1.f, 2.f, 3.f), glm::vec3(0.f), 2.f);
    CHECK(near(glm::vec3(m * glm::vec4(1.f, 0.f, 0.f, 1.f)), glm::vec3(3.f, 2.f, 3.f)));
}

void testAQuarterYawTurnsZTowardsX()
{
    const glm::mat4 m = explosionMatrix(glm::vec3(0.f), glm::vec3(90.f, 0.f, 0.f), 1.f);
    CHECK(near(glm::vec3(m * glm::vec4(0.f, 0.f, 1.f, 0.f)), glm::vec3(1.f, 0.f, 0.f)));
}

// ---- its life (0x7de10 / 0x7de80)
void testAnExplosionShowsFromItsStart()
{
    CHECK(explosionShows(1.f, 2.f, 1.f));
}

void testAnExplosionShowsUntilItsEndIncluded()
{
    CHECK(explosionShows(1.f, 2.f, 3.f));
}

void testAnExplosionDoesNotShowBeforeItsStart()
{
    CHECK(!explosionShows(1.f, 2.f, 0.99f));
}

void testAnExplosionIsSpentOnlyPastItsEnd()
{
    CHECK(!explosionSpent(1.f, 2.f, 3.f) && explosionSpent(1.f, 2.f, 3.01f));
}

void testTheProgressRunsZeroToOne()
{
    CHECK(near(explosionProgress(1.f, 2.f, 2.f), 0.5f));
}

void testTheGreyStartsAt254()
{
    CHECK(explosionGrey(0.f) == 0xfefefefeu);
}

void testTheGreyEndsBlack()
{
    CHECK(explosionGrey(1.f) == 0u);
}

void testTheGreyHalfwayRoundsToEvenThenScales()
{
    CHECK(explosionGrey(0.5f) == 0x7e7e7e7eu);
}

// ---- the flashes (0x7df35, 0x6659d)
void testTheFlashIsCell15()
{
    CHECK(explosionFlash(glm::vec3(0.f), 10.f, 0.f).cell == 15);
}

void testTheFlashIsAdditiveWhiteFadingToBlackInHalfASecond()
{
    const TrailSprite flash = explosionFlash(glm::vec3(0.f), 10.f, 0.f);
    CHECK(flash.additive && flash.from == 0xffffffffu && flash.to == 0u && near(flash.life, 0.5f));
}

void testTheFlashShrinks()
{
    TrailSprite flash = explosionFlash(glm::vec3(0.f), 10.f, 0.f);
    flash.advance(0.1f, 0.1f);
    CHECK(flash.halfSize < 10.f);
}

void testTheFlashDoesNotSpin()
{
    CHECK(near(explosionFlash(glm::vec3(0.f), 10.f, 0.f).angleAt(0.4f), 0.f));
}

void testTheImpactFlashIsAtLeast10()
{
    CHECK(near(impactFlashHalfSize(4.f), 10.f));
}

void testTheImpactFlashTakesALargerInnerRadius()
{
    CHECK(near(impactFlashHalfSize(25.f), 25.f));
}

// ---- the pool (0x7e0f0)
void testAModelHas16Copies()
{
    ExplosionSlots slots;
    for (int k = 0; k < kExplosionCopies; ++k)
    {
        slots.take(3);
    }
    CHECK(!slots.take(3));
}

void testAFullModelLeavesTheOthersRoom()
{
    ExplosionSlots slots;
    for (int k = 0; k < kExplosionCopies; ++k)
    {
        slots.take(3);
    }
    CHECK(slots.take(6));
}

void testThePoolHolds64()
{
    ExplosionSlots slots;
    for (int k = 0; k < kExplosionSlots; ++k)
    {
        slots.take(k % kExplosionModels);
    }
    CHECK(!slots.take(0));
}

void testAReleasedCopyIsFreeAgain()
{
    ExplosionSlots slots;
    for (int k = 0; k < kExplosionCopies; ++k)
    {
        slots.take(3);
    }
    slots.release(3);
    CHECK(slots.take(3));
}

// ---- the knock-out (0x6a8a0)
void testTheKnockOutBlastLasts5WithSize3()
{
    const ExplosionOrder order = knockOutBlastOrder();
    CHECK(near(order.duration, 5.f) && near(order.size, 3.f));
}

void testTheKnockOutEchoesLast2WithSize1()
{
    const ExplosionOrder order = knockOutEchoOrder();
    CHECK(near(order.duration, 2.f) && near(order.size, 1.f));
}

void testTheThirdEchoComesAfter0Point4()
{
    CHECK(near(knockOutEchoDelay(2), 0.4f));
}

void testTheEchoesLandWithin20Metres()
{
    CHECK(near(knockOutEchoOffset(rollOf(0, 0x400, 0x7ff)), glm::vec3(-20.f, 0.f, 20.f * 1023.f / 1024.f)));
}

// ---- the sparks (0x67231 / 0x67e81)
void testACraftHitSparkFliesAt3()
{
    const SparkParticle spark = craftHitSpark(glm::vec3(0.f), sparkRollOf(255, 128, 128, 0, 0));
    CHECK(near(glm::length(spark.vel), 3.f));
}

void testACraftHitSparkStartsATenthOfAMetreOut()
{
    const SparkParticle spark = craftHitSpark(glm::vec3(0.f), sparkRollOf(255, 128, 128, 0, 0));
    CHECK(near(glm::length(spark.pos), 0.1f));
}

void testACraftHitSparkIsStarburstForUpTo3Point5Seconds()
{
    const SparkParticle spark = craftHitSpark(glm::vec3(0.f), sparkRollOf(255, 128, 128, 0, 255));
    CHECK(spark.system == SparkSystem::Starburst && near(spark.life, 3.5f));
}

void testAnImpactSparkFliesAt10To30()
{
    const SparkParticle spark = impactSpark(glm::vec3(0.f), sparkRollOf(255, 128, 128, 255, 0));
    CHECK(near(glm::length(spark.vel), 30.f));
}

void testAnImpactSparkGoesUp()
{
    const SparkParticle spark = impactSpark(glm::vec3(0.f), sparkRollOf(128, 0, 128, 0, 0));
    CHECK(spark.vel.y > 0.f);
}

void testAnImpactSparkIsStarburstRedForUpTo1Point25Seconds()
{
    const SparkParticle spark = impactSpark(glm::vec3(0.f), sparkRollOf(255, 128, 128, 0, 255));
    CHECK(spark.system == SparkSystem::StarburstRed && near(spark.life, 1.25f));
}

void testASparkHoldsItsColourForFourFifthsOfItsLife()
{
    const SparkParticle spark = impactSpark(glm::vec3(0.f), sparkRollOf(255, 128, 128, 0, 0));
    CHECK(near(spark.hold, 0.8f));
}
}  // namespace

int main()
{
    testATypePast15ShowsModel15();
    testATypeBelow16KeepsItsModel();
    testABurstAsksForItsSizePercentAsDurationAndSize();
    testTheDurationIsScaledByTheType();
    testTheScaleIsTheSizeTimesTheTypesSizePercent();
    testAZeroScatterLandsOnTheImpact();
    testTheScatterSpansMinusToPlusItsReach();
    testAFlatScatterKeepsTheImpactsHeight();
    testAPlainTypeIsNotTurned();
    testATurningTypeTakesTheSecondRollAsItsYawOnly();
    testATumblingTypeTakesYawPitchAndRoll();
    testTheMatrixScalesAndPlacesTheModel();
    testAQuarterYawTurnsZTowardsX();
    testAnExplosionShowsFromItsStart();
    testAnExplosionShowsUntilItsEndIncluded();
    testAnExplosionDoesNotShowBeforeItsStart();
    testAnExplosionIsSpentOnlyPastItsEnd();
    testTheProgressRunsZeroToOne();
    testTheGreyStartsAt254();
    testTheGreyEndsBlack();
    testTheGreyHalfwayRoundsToEvenThenScales();
    testTheFlashIsCell15();
    testTheFlashIsAdditiveWhiteFadingToBlackInHalfASecond();
    testTheFlashShrinks();
    testTheFlashDoesNotSpin();
    testTheImpactFlashIsAtLeast10();
    testTheImpactFlashTakesALargerInnerRadius();
    testAModelHas16Copies();
    testAFullModelLeavesTheOthersRoom();
    testThePoolHolds64();
    testAReleasedCopyIsFreeAgain();
    testTheKnockOutBlastLasts5WithSize3();
    testTheKnockOutEchoesLast2WithSize1();
    testTheThirdEchoComesAfter0Point4();
    testTheEchoesLandWithin20Metres();
    testACraftHitSparkFliesAt3();
    testACraftHitSparkStartsATenthOfAMetreOut();
    testACraftHitSparkIsStarburstForUpTo3Point5Seconds();
    testAnImpactSparkFliesAt10To30();
    testAnImpactSparkGoesUp();
    testAnImpactSparkIsStarburstRedForUpTo1Point25Seconds();
    testASparkHoldsItsColourForFourFifthsOfItsLife();
    std::printf("explosion_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
