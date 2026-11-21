// Argon: tests for the hover's dust (logic/hover_dust.hpp: craft_hover 0x5f377..0x5f667 -> 0x9d000) and the trail
// sprite's start angle (0x9c540). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/hover_dust.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon;

const int kMount = 6, kAltMount = 14, kBracken = 5;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

DustRoll largestRoll()
{
    DustRoll roll;
    roll.driftX = roll.driftZ = 0xff;
    roll.alphaA = 0x3f;
    roll.alphaB = 0x1f;
    roll.spin = 59;
    roll.angle = 359;
    roll.size = 19;
    roll.now = 3.f;
    return roll;
}

DustPuff puffAhead()
{
    DustPuff puff;
    puff.at = glm::vec3(1.f, 2.f, 3.f);
    puff.forward = glm::vec3(0.f, 0.f, 1.f);
    puff.groundColour = 0xffffffffu;
    return puff;
}

void testOnlyRoughAndLooseRaiseDust()
{
    CHECK(raisesDust(1) && raisesDust(2));
}

void testTrackWaterSlipperyAndNoHitRaiseNone()
{
    CHECK(!raisesDust(0) && !raisesDust(3) && !raisesDust(4) && !raisesDust(-1));
}

void testTheRollWinsOnlyWithTheLowFourBitsClear()
{
    CHECK(dustRollWins(0x30) && !dustRollWins(0x31) && !dustRollWins(0x0f));
}

void testMountDustIsGrey()
{
    CHECK(dustTintFor(kMount).base == 0xffb0b0b0u && dustTintFor(kMount).offset == 0xff303030u);
}

void testOtherTracksDustIsSandy()
{
    CHECK(dustTintFor(kAltMount).base == 0xffb0a070u && dustTintFor(kBracken).offset == 0xff302816u);
}

void testColourScalesTheBaseByTheGroundAndAddsTheOffset()
{
    // red (0xb0 x 0x80 >> 8) + 0x30 = 0x88, green (0xa0 x 0x40 >> 8) + 0x28 = 0x50, blue (0x70 x 0 >> 8) + 0x16
    CHECK((dustColour(0xff804000u, DustTint{}, DustRoll{}) & 0x00ffffffu) == 0x00885016u);
}

void testAlphaIsFortyPlusTwoRandoms()
{
    CHECK(dustColour(0u, DustTint{}, largestRoll()) >> 24 == 40u + 0x3fu + 0x1fu);
}

void testVelocityBlowsBackTenAlongTheForward()
{
    DustRoll roll;
    roll.driftX = roll.driftZ = 0x7f;
    const glm::vec3 v = dustVelocity(glm::vec3(0.6f, 0.f, 0.8f), roll);
    CHECK(near(v.x, -6.f) && near(v.y, 1.5f) && near(v.z, -8.f));
}

void testVelocityDriftsUpToOnePointSixMetresPerSecond()
{
    const glm::vec3 v = dustVelocity(glm::vec3(0.f), largestRoll());
    CHECK(near(v.x, 128.f * 0.0125f) && near(v.z, 128.f * 0.0125f));
}

void testSpriteLivesTwoAndAHalfSecondsFadingInHalfASecond()
{
    const weapons::TrailSprite s = makeDustSprite(puffAhead(), DustTint{}, largestRoll());
    CHECK(near(s.life, 2.5f) && near(s.fade, 0.5f) && s.fadeIn && near(s.born, 3.f));
}

void testSpriteIsAnAlphaBlendedCellZero()
{
    const weapons::TrailSprite s = makeDustSprite(puffAhead(), DustTint{}, largestRoll());
    CHECK(!s.additive && s.cell == 0);
}

void testSpriteFadesToItsOwnColourWithoutAlpha()
{
    const weapons::TrailSprite s = makeDustSprite(puffAhead(), DustTint{}, largestRoll());
    CHECK(s.to == (s.from & 0x00ffffffu));
}

void testSpriteSizeSpinAndAngleFromTheRoll()
{
    const weapons::TrailSprite s = makeDustSprite(puffAhead(), DustTint{}, largestRoll());
    CHECK(near(s.halfSize, 3.9f) && near(s.spin, 99.f) && near(s.startAngle, 359.f) && near(s.grow, 1.08f));
}

void testRay3SpinsTheOtherWay()
{
    DustPuff puff = puffAhead();
    puff.fromRay3 = true;
    CHECK(near(makeDustSprite(puff, DustTint{}, largestRoll()).spin, -99.f));
}

void testSpriteTurnsFromItsStartAngle()
{
    weapons::TrailSprite s;
    s.startAngle = 90.f;
    s.spin = 180.f;
    CHECK(near(s.angleAt(0.5f), 3.14159265f));
}
}  // namespace

int main()
{
    testOnlyRoughAndLooseRaiseDust();
    testTrackWaterSlipperyAndNoHitRaiseNone();
    testTheRollWinsOnlyWithTheLowFourBitsClear();
    testMountDustIsGrey();
    testOtherTracksDustIsSandy();
    testColourScalesTheBaseByTheGroundAndAddsTheOffset();
    testAlphaIsFortyPlusTwoRandoms();
    testVelocityBlowsBackTenAlongTheForward();
    testVelocityDriftsUpToOnePointSixMetresPerSecond();
    testSpriteLivesTwoAndAHalfSecondsFadingInHalfASecond();
    testSpriteIsAnAlphaBlendedCellZero();
    testSpriteFadesToItsOwnColourWithoutAlpha();
    testSpriteSizeSpinAndAngleFromTheRoll();
    testRay3SpinsTheOtherWay();
    testSpriteTurnsFromItsStartAngle();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
