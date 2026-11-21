// Argon: tests for a weapon hit's lasting effects on a craft (logic/weapon_effects.hpp, logic/hit_flash.hpp, the game's 0x6acf0
// / 0x6d000 / 0x5e370 / 0x5f9d0). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/hit_flash.hpp"
#include "check.hpp"
#include "logic/weapon_effects.hpp"

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

NwpEffects everyEffect()
{
    NwpEffects e{};
    e.corrodeLevel = 10.f;
    e.corrodeTime = 4.f;
    e.freezeTime = 2.f;
    e.controlJitter = 30.f;
    e.controlJitterTime = 3.f;
    e.engineJitterLevel = 40.f;
    e.engineJitterTime = 5.f;
    e.engineOffTime = 1.f;
    return e;
}

NwpEffects halfResistance()
{
    NwpEffects r{};
    r.shieldDamage = 50.f;
    r.freezeTime = 50.f;
    r.controlJitter = 50.f;
    r.engineJitterLevel = 50.f;
    r.engineOffTime = 50.f;
    return r;
}

void testCorrodeTimeIgnoresResistance()
{
    const CraftEffects e = scaleEffects(everyEffect(), halfResistance(), 1.f);
    CHECK(near(e.corrodeLevel, 5.f) && near(e.corrodeTime, 4.f));
}

void testTimesScaleByTheirLevelsResistance()
{
    const CraftEffects e = scaleEffects(everyEffect(), halfResistance(), 1.f);
    CHECK(near(e.freezeTime, 1.f) && near(e.controlJitterTime, 1.5f) && near(e.engineJitterTime, 2.5f) && near(e.engineOffTime, 0.5f));
}

void testEffectsScaleByArea()
{
    const CraftEffects e = scaleEffects(everyEffect(), NwpEffects{}, 0.5f);
    CHECK(near(e.engineOffTime, 0.5f) && near(e.corrodeTime, 2.f));
}

void testActiveShieldAddsToTheResistances()
{
    NwpEffects bonus{};
    bonus.engineOffTime = 100.f;
    const NwpEffects r = targetResistances(NwpEffects{}, 20.f, {&bonus});
    CHECK(near(r.engineOffTime, 100.f) && near(r.shieldDamage, 20.f));
}

void testHitKeepsTheLongerTime()
{
    CraftEffects running;
    running.engineOffTime = 2.f;
    CraftEffects hit;
    hit.engineOffTime = 1.f;
    hit.freezeTime = 3.f;
    takeLonger(running, hit);
    CHECK(near(running.engineOffTime, 2.f) && near(running.freezeTime, 3.f));
}

void testJitterLevelEndsWithItsTime()
{
    CraftEffects e;
    e.engineJitter = 40.f;
    e.engineJitterTime = 0.01f;
    countDown(e, 1.f / 60.f);
    CHECK(e.engineJitterTime == 0.f && e.engineJitter == 0.f);
}

void testCorrodeTakesItsLevelPerSecond()
{
    CraftEffects e;
    e.corrodeLevel = 12.f;
    e.corrodeTime = 2.f;
    countDown(e, 0.5f);
    CHECK(near(corrodeLoss(e, 0.5f), 6.f));
}

void testCorrodeStopsWithItsTime()
{
    CraftEffects e;
    e.corrodeLevel = 12.f;
    e.corrodeTime = 0.25f;
    countDown(e, 0.5f);
    CHECK(corrodeLoss(e, 0.5f) == 0.f);
}

void testEngineOffAndFreezeCutTheEngine()
{
    CraftEffects off, frozen;
    off.engineOffTime = 1.f;
    frozen.freezeTime = 1.f;
    CHECK(off.cutsEngine() && frozen.cutsEngine() && !CraftEffects{}.cutsEngine());
}

void testJitterTakesUpToTheLevel()
{
    CHECK(near(jitter(100.f, 30.f, 15), 70.f) && near(jitter(100.f, 30.f, 0x10), 100.f));
}

void testJitterStopsAtZero()
{
    CHECK(jitter(10.f, 30.f, 15) == 0.f);
}

void testHitFlashStartsAtFullRed()
{
    HitFlash f;
    f.raise(HitFlash::kPerHit);
    CHECK(f.colour() == 0xffcb0000u);
}

void testHitFlashCapsAtOne()
{
    HitFlash f;
    f.raise(HitFlash::kPerHit);
    f.raise(HitFlash::kPerHit);
    CHECK(f.level() == 1.f && f.red() == 254);
}

void testHitFlashFadesByTheFrameTime()
{
    HitFlash f;
    f.raise(HitFlash::kPerHit);
    f.fade(0.3f);
    CHECK(near(f.level(), 0.5f) && f.red() == 127);
}

void testHitFlashLastsFourFifthsOfASecond()
{
    HitFlash f;
    f.raise(HitFlash::kPerHit);
    f.fade(0.79f);
    const bool showing = f.level() > 0.f;
    f.fade(0.01f);
    CHECK(showing && f.level() == 0.f);
}

void testHitFlashStopsAtZero()
{
    HitFlash f;
    f.raise(0.1f);
    f.fade(1.f);
    CHECK(f.level() == 0.f && f.colour() == 0xff000000u);
}
}  // namespace

int main()
{
    testCorrodeTimeIgnoresResistance();
    testTimesScaleByTheirLevelsResistance();
    testEffectsScaleByArea();
    testActiveShieldAddsToTheResistances();
    testHitKeepsTheLongerTime();
    testJitterLevelEndsWithItsTime();
    testCorrodeTakesItsLevelPerSecond();
    testCorrodeStopsWithItsTime();
    testEngineOffAndFreezeCutTheEngine();
    testJitterTakesUpToTheLevel();
    testJitterStopsAtZero();
    testHitFlashStartsAtFullRed();
    testHitFlashCapsAtOne();
    testHitFlashFadesByTheFrameTime();
    testHitFlashLastsFourFifthsOfASecond();
    testHitFlashStopsAtZero();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
