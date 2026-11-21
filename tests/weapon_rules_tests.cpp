// Argon: tests for the drones, the nemesis race, the special's fire rule, the power-up levels, the volley targets,
// the shields' colour fade and sphere (logic/weapon_rules.hpp: 0x58e40, 0x3f3d0, 0x791f0, 0x6d000, 0x6ba30, 0x6c110,
// 0x69970), the drones' catch-up rows
// (logic/catch_up_rules.hpp: 0x6d650) and the trail sprites (logic/trail_sprite.hpp: 0x66650, 0x9c540). No window or
// GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/catch_up_rules.hpp"
#include "check.hpp"
#include "logic/trail_sprite.hpp"
#include "logic/weapon_rules.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::weapons;

const int kTara = 0, kAlicia = 8, kYuri = 1;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

bool sameSetup(const AiWeaponSetup &a, const AiWeaponSetup &b)
{
    return a.levelCap == b.levelCap && a.specialCap == b.specialCap && a.shieldLevel == b.shieldLevel;
}

TrailSprite fadingSprite()
{
    TrailSprite s;
    s.born = 0.f;
    s.life = 1.f;
    s.fade = 0.5f;
    s.from = 0xff00ff00u;
    s.to = 0x00000000u;
    return s;
}

// ---- drones (0x58e40)
void testThePlayersNemesisIsANemesisDrone()
{
    CHECK(droneKind(kAlicia, kTara, 3) == DroneKind::Nemesis && droneKind(kTara, kAlicia, 3) == DroneKind::Nemesis);
}

void testTheNemesisFlagWinsOverTheHomeTrack()
{
    CHECK(droneKind(kAlicia, kTara, kAlicia) == DroneKind::Nemesis);
}

void testAPilotOnItsOwnTrackIsAtHome()
{
    CHECK(droneKind(kYuri, kTara, kYuri) == DroneKind::HomeCircuit);
}

void testOtherDronesAreGeneral()
{
    CHECK(droneKind(kYuri, kTara, kAlicia) == DroneKind::General);
}

// ---- their catch-up rows (0x6d650)
argon::catchup::ZoneGainRows brackenZoneOne()
{
    argon::catchup::ZoneGainRows rows;
    rows.general = {-2.f, -2.f, -1.f, -1.f, -1.f};
    rows.homeCircuit = {-1.f, -1.f, 0.f, 0.f, 0.f};
    rows.nemesis = {-3.f, -3.f, -2.f, -2.f, -2.f};
    return rows;
}

argon::catchup::PlayerIsRows jayaPlayerIs()
{
    argon::catchup::PlayerIsRows rows;
    rows.general = {2.f, 2.f, 2.f, 2.f, 2.f};
    rows.nemesis = {5.f, 5.f, 5.f, 5.f, 5.f};
    return rows;
}

void testTheNemesisTakesTheNemesisZoneGain()
{
    CHECK(near(argon::catchup::zoneGain(brackenZoneOne(), DroneKind::Nemesis, 2), -2.f));
}

void testAHomeCircuitDroneTakesTheHomeCircuitZoneGain()
{
    CHECK(near(argon::catchup::zoneGain(brackenZoneOne(), DroneKind::HomeCircuit, 0), -1.f));
}

void testAGeneralDroneTakesTheGeneralZoneGain()
{
    CHECK(near(argon::catchup::zoneGain(brackenZoneOne(), DroneKind::General, 4), -1.f));
}

void testTheZoneGainClampsTheSpeedClass()
{
    CHECK(near(argon::catchup::zoneGain(brackenZoneOne(), DroneKind::Nemesis, 9), -2.f));
}

void testTheNemesisTakesThePlayersNemesisModifier()
{
    CHECK(near(argon::catchup::playerIsGain(jayaPlayerIs(), DroneKind::Nemesis, 1), 5.f));
}

void testAHomeCircuitDroneTakesThePlayersGeneralModifier()
{
    CHECK(near(argon::catchup::playerIsGain(jayaPlayerIs(), DroneKind::HomeCircuit, 1), 2.f));
}

// ---- AI caps and shield level (0x3f3d0)
void testNoviceGeneralDronesHoldNoWeapons()
{
    CHECK(sameSetup(aiWeaponSetup(0, DroneKind::General), AiWeaponSetup{0, 0, 0}));
}

void testExpertDronesByKind()
{
    CHECK(sameSetup(aiWeaponSetup(2, DroneKind::Nemesis), AiWeaponSetup{3, 0, 5}) &&
          sameSetup(aiWeaponSetup(2, DroneKind::HomeCircuit), AiWeaponSetup{2, 0, 4}) &&
          sameSetup(aiWeaponSetup(2, DroneKind::General), AiWeaponSetup{1, 0, 3}));
}

void testOnlyMasterGeneralDronesMissTheSpecial()
{
    CHECK(aiWeaponSetup(3, DroneKind::Nemesis).specialCap == 1 && aiWeaponSetup(3, DroneKind::HomeCircuit).specialCap == 0 &&
          aiWeaponSetup(4, DroneKind::HomeCircuit).specialCap == 1 && aiWeaponSetup(4, DroneKind::General).specialCap == 0);
}

void testSpeedClassIsClamped()
{
    CHECK(sameSetup(aiWeaponSetup(9, DroneKind::General), AiWeaponSetup{3, 0, 6}));
}

// ---- the nemesis race (0x791f0: logic\tourney_rules.hpp, tests\frontend_tests.cpp)
void testOnlySlotOneTakesHalfInANemesisRace()
{
    CHECK(near(nemesisShare(true, 1), 0.5f) && near(nemesisShare(true, 0), 1.f) && near(nemesisShare(false, 1), 1.f));
}

// ---- attack points (0x83f90, 0x64070)
const int kWeaponHitBonus = 75;

void testAttackPointsPayPerShieldPointTaken()
{
    CHECK(attackPoints(20.f, 120.f, false, kWeaponHitBonus) == 24 * kWeaponHitBonus);
}

void testAttackPointsTruncateThePartShieldPoint()
{
    CHECK(attackPoints(10.f, 125.f, false, kWeaponHitBonus) == 12 * kWeaponHitBonus);
}

void testAttackPointsDoubleOnThePlayersNemesis()
{
    CHECK(attackPoints(10.f, 125.f, true, kWeaponHitBonus) == 25 * kWeaponHitBonus);
}

void testAHitUnderOneShieldPointScoresNothing()
{
    CHECK(attackPoints(0.5f, 120.f, false, kWeaponHitBonus) == 0);
}

// ---- the special (0x6d000)
void testAHeldSpecialTakesEveryRequest()
{
    CHECK(fireColour(0, true) == kSpecialColour && fireColour(2, true) == kSpecialColour && fireColour(2, false) == 2);
}

PowerLevels levels(int red, int blue, int yellow, int special)
{
    PowerLevels l;
    l.red = red;
    l.blue = blue;
    l.yellow = yellow;
    l.special = special;
    return l;
}

bool sameLevels(const PowerLevels &a, const PowerLevels &b)
{
    return a.red == b.red && a.blue == b.blue && a.yellow == b.yellow && a.special == b.special;
}

const PowerLevels kStrength7Caps = levels(3, 3, 3, 1);

void testTheSpecialEmptiesEveryLevel()
{
    const PowerLevels held = levels(3, 3, 3, 1);

    const PowerLevels after = levelsAfterFiring(held, kSpecialSlot);

    CHECK(sameLevels(after, levels(0, 0, 0, 0)));
}

void testAColourShotEmptiesOnlyItsColour()
{
    const PowerLevels held = levels(2, 3, 1, 0);
    const int blueLevel3 = 8;

    const PowerLevels after = levelsAfterFiring(held, blueLevel3);

    CHECK(sameLevels(after, levels(2, 0, 1, 0)));
}

void testTheSpecialsShieldIsYellowLevel3()
{
    const PowerLevels emptied = levels(0, 0, 0, 0);

    const PowerLevels shield = levelsForSpecialsShield(emptied);

    CHECK(sameLevels(shield, levels(0, 0, 3, 0)));
}

void testAMegaUpAtLevel3GrantsASpecial()
{
    const PowerLevels full = levels(3, 3, 3, 0);

    const PowerLevels after = raisePowerLevels(full, kMegaUp, kStrength7Caps);

    CHECK(sameLevels(after, levels(3, 3, 3, 1)));
}

void testTheSpecialIsCappedByWeaponStrength()
{
    const PowerLevels full = levels(3, 3, 3, 0);

    const PowerLevels after = raisePowerLevels(full, kMegaUp, levels(3, 3, 3, 0));

    CHECK(after.special == 0);
}

void testAMegaUpBelowLevel3RaisesAllThree()
{
    const PowerLevels some = levels(1, 2, 0, 0);

    const PowerLevels after = raisePowerLevels(some, kMegaUp, kStrength7Caps);

    CHECK(sameLevels(after, levels(2, 3, 1, 0)));
}

void testPickupTypesRaiseRedBlueYellow()
{
    const PowerLevels none = levels(0, 0, 0, 0);
    const int red = 1, blue = 0, yellow = 2;

    const bool raised = sameLevels(raisePowerLevels(none, red, kStrength7Caps), levels(1, 0, 0, 0)) &&
                        sameLevels(raisePowerLevels(none, blue, kStrength7Caps), levels(0, 1, 0, 0)) &&
                        sameLevels(raisePowerLevels(none, yellow, kStrength7Caps), levels(0, 0, 1, 0));

    CHECK(raised);
}

// ---- volley targets (0x6c22e)
void testEachProjectileTakesItsLock()
{
    const std::vector<int> locks = {4, 2};

    const std::vector<int> targets = volleyTargets(locks, 2);

    CHECK(targets == std::vector<int>({4, 2}));
}

void testProjectilesBeyondTheLocksTakeTheNearest()
{
    const std::vector<int> locks = {4, 2};

    const std::vector<int> targets = volleyTargets(locks, 5);

    CHECK(targets == std::vector<int>({4, 2, 4, 4, 4}));
}

void testNoLocksLeaveEveryProjectileUnaimed()
{
    const std::vector<int> targets = volleyTargets({}, 3);

    CHECK(targets == std::vector<int>({-1, -1, -1}));
}

void testMoreLocksThanProjectilesUseTheNearestOnes()
{
    const std::vector<int> locks = {5, 3, 1};

    const std::vector<int> targets = volleyTargets(locks, 1);

    CHECK(targets == std::vector<int>({5}));
}

// ---- fire buttons (0x70fd2 / 0x71918)
FireButtons held(bool b, bool x, bool y)
{
    FireButtons f;
    f.b = b;
    f.x = x;
    f.y = y;
    return f;
}

void testEachButtonFiresItsColour()
{
    CHECK(padFireColour(held(true, false, false)) == 0 && padFireColour(held(false, true, false)) == 2 &&
          padFireColour(held(false, false, true)) == kYellowColour);
}

void testBWinsOverXAndY()
{
    CHECK(padFireColour(held(true, true, true)) == 0);
}

void testXWinsOverY()
{
    CHECK(padFireColour(held(false, true, true)) == 2);
}

void testNoButtonNoFire()
{
    CHECK(padFireColour(FireButtons{}) == kNoFire);
}

// ---- shields (0x69970)
void testShieldTintHoldsBeforeTheFade()
{
    const ShieldFade fade{2.7f, 3.f, 0x007e7e7eu, 0u};
    CHECK(shieldTint(fade, 1.f) == 0xff7e7e7eu && shieldTint(fade, 2.7f) == 0xff7e7e7eu);
}

void testShieldTintBlendsToTheEndColour()
{
    const ShieldFade fade{2.f, 3.f, 0x00804020u, 0x00000000u};
    CHECK(shieldTint(fade, 2.5f) == 0xff402010u && shieldTint(fade, 3.f) == 0xff000000u);
}

void testShieldSphereGrowsBetweenItsAges()
{
    const ShieldSphere sphere{0.f, 2.f, 2.f, 100.f};
    CHECK(near(*shieldSphereRadius(sphere, 1.f), 51.f));
}

void testShieldSphereIsGoneOutsideItsAges()
{
    const ShieldSphere sphere{0.f, 2.f, 2.f, 100.f};
    CHECK(!shieldSphereRadius(sphere, 0.f) && !shieldSphereRadius(sphere, 2.f));
}

// ---- trail sprites (0x66650 / 0x9c540)
void testSpriteCellsAreFourARow()
{
    CHECK(trailSpriteCell(0) == glm::vec2(0.f, 0.f) && trailSpriteCell(5) == glm::vec2(128.f, 128.f) && trailSpriteCell(15) == glm::vec2(384.f, 384.f));
}

void testSpriteRandomsScaleSpinAndSize()
{
    TrailSpriteStyle style;
    style.size = 1.f;
    style.sizeRange = 10.f;
    style.spin = 4.f;
    style.spinRange = 360.f;
    style.flags = 1;
    const TrailSprite s = makeTrailSprite(style, glm::vec3(0.f), glm::vec3(0.f), 0.f, 0x3ff, 0x3ff, true);
    CHECK(near(s.halfSize, 11.f) && near(s.spin, -364.f));
}

void testSpriteFlag2IsAlphaBlended()
{
    TrailSpriteStyle style;
    style.flags = 2;
    CHECK(!makeTrailSprite(style, glm::vec3(0.f), glm::vec3(0.f), 0.f, 0, 0, false).additive);
}

void testSpriteDiesAtItsLife()
{
    TrailSprite s = fadingSprite();
    CHECK(s.advance(0.9f, 1.f / 60.f) && !s.advance(1.f, 1.f / 60.f));
}

void testSpriteMovesWithItsVelocity()
{
    TrailSprite s = fadingSprite();
    s.vel = glm::vec3(60.f, 0.f, 0.f);
    s.advance(0.1f, 1.f / 60.f);
    CHECK(near(s.pos.x, 1.f));
}

void testSpriteGrowsWithDtSquared()
{
    TrailSprite s = fadingSprite();
    s.halfSize = 1.f;
    s.grow = 2.f;
    s.advance(0.1f, 0.1f);
    CHECK(near(s.halfSize, 1.2f));
}

void testSpriteHoldsItsColourUntilTheFade()
{
    CHECK(fadingSprite().colourAt(0.4f) == 0xff00ff00u);
}

void testSpriteBlendsAfterTheFade()
{
    CHECK(fadingSprite().colourAt(0.75f) == 0x7e007e00u);
}

void testAdditiveSpriteFadesItsColourIn()
{
    TrailSprite s = fadingSprite();
    s.fadeIn = true;
    CHECK(s.colourAt(0.25f) == 0xff007f00u);
}

void testBlendedSpriteFadesItsAlphaIn()
{
    TrailSprite s = fadingSprite();
    s.fadeIn = true;
    s.additive = false;
    CHECK(s.colourAt(0.25f) == 0x7f00ff00u);
}

void testSpriteTurnsBySpinDegrees()
{
    TrailSprite s = fadingSprite();
    s.spin = 180.f;
    CHECK(near(s.angleAt(0.5f), 3.14159265f / 2.f));
}
}  // namespace

int main()
{
    testThePlayersNemesisIsANemesisDrone();
    testTheNemesisFlagWinsOverTheHomeTrack();
    testAPilotOnItsOwnTrackIsAtHome();
    testOtherDronesAreGeneral();
    testTheNemesisTakesTheNemesisZoneGain();
    testAHomeCircuitDroneTakesTheHomeCircuitZoneGain();
    testAGeneralDroneTakesTheGeneralZoneGain();
    testTheZoneGainClampsTheSpeedClass();
    testTheNemesisTakesThePlayersNemesisModifier();
    testAHomeCircuitDroneTakesThePlayersGeneralModifier();
    testNoviceGeneralDronesHoldNoWeapons();
    testExpertDronesByKind();
    testOnlyMasterGeneralDronesMissTheSpecial();
    testSpeedClassIsClamped();
    testOnlySlotOneTakesHalfInANemesisRace();
    testAttackPointsPayPerShieldPointTaken();
    testAttackPointsTruncateThePartShieldPoint();
    testAttackPointsDoubleOnThePlayersNemesis();
    testAHitUnderOneShieldPointScoresNothing();
    testAHeldSpecialTakesEveryRequest();
    testTheSpecialEmptiesEveryLevel();
    testAColourShotEmptiesOnlyItsColour();
    testTheSpecialsShieldIsYellowLevel3();
    testAMegaUpAtLevel3GrantsASpecial();
    testTheSpecialIsCappedByWeaponStrength();
    testAMegaUpBelowLevel3RaisesAllThree();
    testPickupTypesRaiseRedBlueYellow();
    testEachProjectileTakesItsLock();
    testProjectilesBeyondTheLocksTakeTheNearest();
    testNoLocksLeaveEveryProjectileUnaimed();
    testMoreLocksThanProjectilesUseTheNearestOnes();
    testEachButtonFiresItsColour();
    testBWinsOverXAndY();
    testXWinsOverY();
    testNoButtonNoFire();
    testShieldTintHoldsBeforeTheFade();
    testShieldTintBlendsToTheEndColour();
    testShieldSphereGrowsBetweenItsAges();
    testShieldSphereIsGoneOutsideItsAges();
    testSpriteCellsAreFourARow();
    testSpriteRandomsScaleSpinAndSize();
    testSpriteFlag2IsAlphaBlended();
    testSpriteDiesAtItsLife();
    testSpriteMovesWithItsVelocity();
    testSpriteGrowsWithDtSquared();
    testSpriteHoldsItsColourUntilTheFade();
    testSpriteBlendsAfterTheFade();
    testAdditiveSpriteFadesItsColourIn();
    testBlendedSpriteFadesItsAlphaIn();
    testSpriteTurnsBySpinDegrees();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
