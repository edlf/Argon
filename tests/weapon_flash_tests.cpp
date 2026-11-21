// Argon: tests for the weapons' one-frame flashes (logic/weapon_flash.hpp: the beam update 0x68620 and the sparks
// 0x68150 -> 0xbd670). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/weapon_flash.hpp"
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

BeamFlashes castFlashes(BeamStop stop, bool firstFrame)
{
    return beamFlashesOf(BeamFrame{stop, firstFrame, false});
}

BeamFlashes replayedFlashes(BeamStop stop, bool firstFrame)
{
    return beamFlashesOf(BeamFrame{stop, firstFrame, true});
}

// ---- which flashes (0x68620)
void testACastBeamGlowsAtItsStartEveryFrame()
{
    CHECK(castFlashes(BeamStop::Open, false).startGlow);
}

void testAnOpenBeamMakesNoSparks()
{
    const BeamFlashes flashes = castFlashes(BeamStop::Open, true);
    CHECK(!flashes.spark && !flashes.hit);
}

void testABeamOnTheWorldSparksEveryFrame()
{
    CHECK(castFlashes(BeamStop::World, false).spark);
}

void testABeamOnTheWorldMakesNoHitFlash()
{
    CHECK(!castFlashes(BeamStop::World, true).hit);
}

void testACraftHitOnTheFirstFrameSparksAndFlashes()
{
    const BeamFlashes flashes = castFlashes(BeamStop::Craft, true);
    CHECK(flashes.spark && flashes.hit);
}

void testACraftHitAfterTheFirstFrameOnlyGlows()
{
    const BeamFlashes flashes = castFlashes(BeamStop::Craft, false);
    CHECK(flashes.startGlow && !flashes.spark && !flashes.hit);
}

void testAReplayedBeamHasNoStartGlow()
{
    CHECK(!replayedFlashes(BeamStop::Craft, true).startGlow);
}

void testAReplayedCraftHitSparksAndFlashesOnTheFirstFrame()
{
    const BeamFlashes flashes = replayedFlashes(BeamStop::Craft, true);
    CHECK(flashes.spark && flashes.hit);
}

void testAReplayedOpenBeamSparksOnTheFirstFrame()
{
    const BeamFlashes flashes = replayedFlashes(BeamStop::Open, true);
    CHECK(flashes.spark && !flashes.hit);
}

void testAReplayedBeamMakesNothingAfterTheFirstFrame()
{
    const BeamFlashes flashes = replayedFlashes(BeamStop::World, false);
    CHECK(!flashes.startGlow && !flashes.spark && !flashes.hit);
}

// ---- grey level (0x68b6a..0x68bf2, 0x68282..0x682b5)
void testTheFadeFallsWithTheBeamsAge()
{
    CHECK(near(beamFade(0.05f, 0.2f), 0.75f));
}

void testTheFadeStopsAtZeroPastTheLife()
{
    CHECK(beamFade(0.3f, 0.2f) == 0.f);
}

void testFullStrengthIsOpaqueWhite()
{
    CHECK(greyFlashColour(1.f) == 0xffffffffu);
}

void testHalfStrengthRoundsToEven()
{
    CHECK(greyFlashColour(0.5f) == 0xff808080u);
}

void testStrengthIsClampedAtBlack()
{
    CHECK(greyFlashColour(-1.f) == 0xff000000u);
}

// ---- the quads (0xbd670's arguments)
void testTheStartGlowIsHalfSize2Point5()
{
    CHECK(near(beamStartGlow(glm::vec3(0.f), 1.f, 0).halfSize, 2.5f));
}

void testTheStartGlowTakesTheBeamsGrey()
{
    CHECK(beamStartGlow(glm::vec3(0.f), 0.f, 0).argb == 0xff000000u);
}

void testTheSparkFlashGrowsBy0To3()
{
    CHECK(near(sparkFlash(glm::vec3(0.f), 1.f, 0, 7).halfSize, 13.f));
}

void testTheHitFlashIsWhiteHalfSize25()
{
    const WeaponFlash flash = beamHitFlash(glm::vec3(1.f, 2.f, 3.f), 0);
    CHECK(near(flash.halfSize, 25.f) && flash.argb == 0xffffffffu && near(flash.pos, glm::vec3(1.f, 2.f, 3.f)));
}

void testTheAngleIsNineBitsOfDegrees()
{
    CHECK(beamHitFlash(glm::vec3(0.f), 0x3ffu).angleDegrees == 511.f);
}

// ---- the sparks' particles (0x68150 -> 0xbcb60)
void testACraftHitThrowsFourStarbursts()
{
    const SparkBurst burst = sparkBurstOf(BeamStop::Craft);
    CHECK(burst.count == 4 && burst.system == SparkSystem::Starburst);
}

void testTheWorldThrowsOneStarburstRed()
{
    const SparkBurst burst = sparkBurstOf(BeamStop::World);
    CHECK(burst.count == 1 && burst.system == SparkSystem::StarburstRed);
}

void testAStarburstRedSparkSpreadsFiveEachWay()
{
    const SparkParticle spark = sparkParticle(glm::vec3(0.f), SparkSystem::StarburstRed, ParticleRoll{0, 255, 0x1ff});
    CHECK(near(spark.vel, glm::vec3(-5.f, 5.f, 5.f)));
}

void testAStarburstSparkFliesTenTimesFaster()
{
    const SparkParticle spark = sparkParticle(glm::vec3(0.f), SparkSystem::Starburst, ParticleRoll{255, 0, 0});
    CHECK(near(spark.vel.x, 50.f) && near(spark.vel.y, -50.f));
}

void testASparkStartsHalfItsVelocityAhead()
{
    const SparkParticle spark = sparkParticle(glm::vec3(1.f, 2.f, 3.f), SparkSystem::StarburstRed, ParticleRoll{255, 0, 255});
    CHECK(near(spark.pos, glm::vec3(3.5f, -0.5f, 5.5f)));
}

void testASparkHoldsWhiteForPoint8ThenFadesToBlackAtOneSecond()
{
    const SparkParticle spark = sparkParticle(glm::vec3(0.f), SparkSystem::Starburst, ParticleRoll{});
    CHECK(spark.from == 0xffffffffu && spark.to == 0u && near(spark.hold, 0.8f) && near(spark.life, 1.f));
}

// ---- the 0x9d000 sprites
void testTheSparksHaveTheirSpriteOnTheFirstFrame()
{
    CHECK(castFlashes(BeamStop::World, true).sparkSprite);
}

void testTheSparksAtAWallHaveNoSpriteAfterTheFirstFrame()
{
    const BeamFlashes flashes = castFlashes(BeamStop::World, false);
    CHECK(flashes.spark && !flashes.sparkSprite);
}

void testAnOpenBeamHasNoSparkSprite()
{
    CHECK(!castFlashes(BeamStop::Open, true).sparkSprite);
}

void testAReplayedBeamHasItsSparkSpriteOnTheFirstFrame()
{
    CHECK(replayedFlashes(BeamStop::Open, true).sparkSprite);
}

void testTheSparkSpriteIsAQuarterOf10To13()
{
    CHECK(near(sparkSprite(glm::vec3(0.f), glm::vec3(0.f), FlashSpriteRoll{0.f, 0, 0, 7}).halfSize, 3.25f));
}

void testTheSparkSpriteMovesWithTheLastSpark()
{
    CHECK(near(sparkSprite(glm::vec3(0.f), glm::vec3(1.f, 2.f, 3.f), FlashSpriteRoll{}).vel, glm::vec3(1.f, 2.f, 3.f)));
}

void testTheHitSpriteIsStillAndAQuarterOf20To23()
{
    const TrailSprite sprite = beamHitSprite(glm::vec3(0.f), FlashSpriteRoll{0.f, 0, 0, 1});
    CHECK(near(sprite.halfSize, 5.25f) && near(sprite.vel, glm::vec3(0.f)));
}

void testTheFlashSpritesAreAdditiveInCell10()
{
    const TrailSprite sprite = beamHitSprite(glm::vec3(0.f), FlashSpriteRoll{});
    CHECK(sprite.additive && !sprite.fadeIn && sprite.cell == 10);
}

void testTheFlashSpritesBlendWhiteToBlackOverASecond()
{
    const TrailSprite sprite = beamHitSprite(glm::vec3(0.f), FlashSpriteRoll{2.f, 0, 0, 0});
    CHECK(sprite.from == 0xffffffffu && sprite.to == 0u && sprite.fade == 0.f && near(sprite.life, 1.f) && near(sprite.born, 2.f));
}

void testTheFlashSpritesDoNotGrow()
{
    CHECK(sparkSprite(glm::vec3(0.f), glm::vec3(0.f), FlashSpriteRoll{}).grow == 1.f);
}

void testTheFlashSpritesSpinAndStartAtNineBitsOfDegrees()
{
    const TrailSprite sprite = sparkSprite(glm::vec3(0.f), glm::vec3(0.f), FlashSpriteRoll{0.f, 0x3ffu, 0x201u, 0});
    CHECK(sprite.spin == 511.f && sprite.startAngle == 1.f);
}

// ---- the beam's bone tint (0x68620 -> 0xb9930)
void testTheBeamsTintIsBlackAtTheEndOfItsLife()
{
    CHECK(greyFlashColour(beamFade(0.5f, 0.5f)) == 0xff000000u);
}

// ---- the hit point (0x683b0)
void testTheHitPointIsOnTheBeamAtTheCraftsDistance()
{
    CHECK(near(pointAlongBeam(glm::vec3(0.f), glm::vec3(0.f, 0.f, 100.f), glm::vec3(3.f, 1.f, 40.f)), glm::vec3(0.f, 0.f, 40.f)));
}

void testAZeroLengthBeamHitsAtItsStart()
{
    CHECK(near(pointAlongBeam(glm::vec3(5.f), glm::vec3(5.f), glm::vec3(0.f)), glm::vec3(5.f)));
}
}  // namespace

int main()
{
    testACastBeamGlowsAtItsStartEveryFrame();
    testAnOpenBeamMakesNoSparks();
    testABeamOnTheWorldSparksEveryFrame();
    testABeamOnTheWorldMakesNoHitFlash();
    testACraftHitOnTheFirstFrameSparksAndFlashes();
    testACraftHitAfterTheFirstFrameOnlyGlows();
    testAReplayedBeamHasNoStartGlow();
    testAReplayedCraftHitSparksAndFlashesOnTheFirstFrame();
    testAReplayedOpenBeamSparksOnTheFirstFrame();
    testAReplayedBeamMakesNothingAfterTheFirstFrame();
    testTheFadeFallsWithTheBeamsAge();
    testTheFadeStopsAtZeroPastTheLife();
    testFullStrengthIsOpaqueWhite();
    testHalfStrengthRoundsToEven();
    testStrengthIsClampedAtBlack();
    testTheStartGlowIsHalfSize2Point5();
    testTheStartGlowTakesTheBeamsGrey();
    testTheSparkFlashGrowsBy0To3();
    testTheHitFlashIsWhiteHalfSize25();
    testTheAngleIsNineBitsOfDegrees();
    testACraftHitThrowsFourStarbursts();
    testTheWorldThrowsOneStarburstRed();
    testAStarburstRedSparkSpreadsFiveEachWay();
    testAStarburstSparkFliesTenTimesFaster();
    testASparkStartsHalfItsVelocityAhead();
    testASparkHoldsWhiteForPoint8ThenFadesToBlackAtOneSecond();
    testTheSparksHaveTheirSpriteOnTheFirstFrame();
    testTheSparksAtAWallHaveNoSpriteAfterTheFirstFrame();
    testAnOpenBeamHasNoSparkSprite();
    testAReplayedBeamHasItsSparkSpriteOnTheFirstFrame();
    testTheSparkSpriteIsAQuarterOf10To13();
    testTheSparkSpriteMovesWithTheLastSpark();
    testTheHitSpriteIsStillAndAQuarterOf20To23();
    testTheFlashSpritesAreAdditiveInCell10();
    testTheFlashSpritesBlendWhiteToBlackOverASecond();
    testTheFlashSpritesDoNotGrow();
    testTheFlashSpritesSpinAndStartAtNineBitsOfDegrees();
    testTheBeamsTintIsBlackAtTheEndOfItsLife();
    testTheHitPointIsOnTheBeamAtTheCraftsDistance();
    testAZeroLengthBeamHitsAtItsStart();
    std::printf("weapon_flash_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
