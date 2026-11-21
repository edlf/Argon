// Argon: tests for the damage arithmetic of a weapon hit (logic/weapon_damage.hpp, the game's 0x6acf0 /
// 0x6aa00). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/weapon_damage.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::original::NwpEffects;
using namespace argon::weapons;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

// Tara's level 1 CLUSTER: 30 % shield damage, area 15..30 m at 100..60 %
NwpEffects cluster()
{
    NwpEffects e{};
    e.shieldDamage = 30.f;
    e.flags = 0x400;
    e.areaInnerRadius = 15.f;
    e.areaOuterRadius = 30.f;
    e.areaInnerPercent = 100.f;
    e.areaOuterPercent = 60.f;
    return e;
}

NwpEffects rocket()
{
    NwpEffects e{};
    e.shieldDamage = 20.f;
    e.areaInnerRadius = 5.f;
    e.areaOuterRadius = 15.f;
    e.areaInnerPercent = 100.f;
    e.areaOuterPercent = 100.f;
    return e;
}

NwpEffects tshield()
{
    NwpEffects e{};
    e.shieldDamage = 100.f;
    e.shieldLeech = 100.f;
    return e;
}

void testDirectWeaponIgnoresDistance()
{
    CHECK(areaFactor(rocket(), 500.f).value_or(-1.f) == 1.f);
}

void testAreaIsFullInsideInnerRadius()
{
    CHECK(near(areaFactor(cluster(), 10.f).value_or(-1.f), 1.f));
}

void testAreaFallsLinearlyToOuterPercent()
{
    CHECK(near(areaFactor(cluster(), 22.5f).value_or(-1.f), 0.8f));
}

void testAreaMissesAtOuterRadius()
{
    CHECK(!areaFactor(cluster(), 30.f).has_value());
}

void testBlastPushesFullyInsideInnerRadius()
{
    CHECK(near(blastPush(cluster(), 10.f, 100.f), 100.f));
}

void testBlastPushFallsToOuterPercent()
{
    CHECK(near(blastPush(cluster(), 22.5f, 100.f), 80.f));
}

void testBlastPushStopsAtOuterRadius()
{
    CHECK(blastPush(cluster(), 30.f, 100.f) == 0.f);
}

void testBlastPushesWithoutAreaFlag()
{
    CHECK(near(blastPush(rocket(), 10.f, 100.f), 100.f));
}

void testBlastPushScalesByExplosionType()
{
    CHECK(blastPush(cluster(), 10.f, 0.f) == 0.f);
}

void testShieldSizeCancelsResistance()
{
    // a shield of 125 points resists 20 %: a 20 % hit takes 16 % of it = 20 points
    CHECK(near(shieldResistance(125.f, {}), 20.f));
}

void testActiveShieldCapsResistanceAt100()
{
    const NwpEffects bonus = tshield();
    CHECK(near(shieldResistance(125.f, {&bonus}), 100.f));
}

void testHitScalesByResistanceAndArea()
{
    const ScaledHit hit = scaleHit(rocket(), HitResistance{20.f, 0.f}, 0.5f, ShieldGuard{});
    CHECK(near(hit.shieldPercent, 8.f));
}

void testLeechScalesByItsOwnResistance()
{
    NwpEffects stream = rocket();
    stream.shieldLeech = 50.f;
    const ScaledHit hit = scaleHit(stream, HitResistance{0.f, 50.f}, 1.f, ShieldGuard{});
    CHECK(near(hit.leechPercent, 25.f));
}

void testBlockedHitDoesNothing()
{
    NwpEffects stream = rocket();
    stream.shieldLeech = 50.f;
    const ScaledHit hit = scaleHit(stream, HitResistance{0.f, 0.f}, 1.f, ShieldGuard{true, false});
    CHECK(hit.shieldPercent == 0.f && hit.leechPercent == 0.f);
}

void testAbsorbTurnsDamageIntoRecharge()
{
    const ScaledHit hit = scaleHit(rocket(), HitResistance{0.f, 0.f}, 1.f, ShieldGuard{true, true});
    CHECK(near(hit.shieldPercent, -20.f));
}

void testMercyFromAThirdOfTheShield()
{
    CHECK(sparesThePlayer(33.f) && !sparesThePlayer(32.9f));
}

void testMercyLeavesOneToSixPercent()
{
    CHECK(near(mercyPercent(0), 1.f) && near(mercyPercent(0x3ff), 6.f));
}
}  // namespace

int main()
{
    testDirectWeaponIgnoresDistance();
    testAreaIsFullInsideInnerRadius();
    testAreaFallsLinearlyToOuterPercent();
    testAreaMissesAtOuterRadius();
    testBlastPushesFullyInsideInnerRadius();
    testBlastPushFallsToOuterPercent();
    testBlastPushStopsAtOuterRadius();
    testBlastPushesWithoutAreaFlag();
    testBlastPushScalesByExplosionType();
    testShieldSizeCancelsResistance();
    testActiveShieldCapsResistanceAt100();
    testHitScalesByResistanceAndArea();
    testLeechScalesByItsOwnResistance();
    testBlockedHitDoesNothing();
    testAbsorbTurnsDamageIntoRecharge();
    testMercyFromAThirdOfTheShield();
    testMercyLeavesOneToSixPercent();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
