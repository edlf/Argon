// Argon: tests for a craft's weapon object (logic/craft_weapon.hpp: the shield up 0x6aa00 / 0x6aba0, losing and
// gaining shield 0x6acf0 / 0x6b7e0, the effects 0x6b1b8 / 0x6d000, the knock-out 0x6a8a0, the shield's sphere 0x69970).
// No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/craft_weapon.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::weapons;

const uint8_t kAbsorbs = 4;

CraftWeapon fullShieldOf(float size)
{
    CraftWeapon weapon;
    weapon.equipShield(size);
    return weapon;
}

// a shield of `size` at `value`
CraftWeapon shieldAt(float size, float value)
{
    CraftWeapon weapon = fullShieldOf(size);
    weapon.spare(value / size * 100.f);
    return weapon;
}

// a weapon whose running effects are `effects` (afflicting a fresh weapon takes them as they are)
CraftWeapon afflicted(CraftWeapon weapon, const CraftEffects &effects)
{
    weapon.afflict(effects);
    return weapon;
}

CraftEffects engineOffFor(float seconds)
{
    CraftEffects effects;
    effects.engineOffTime = seconds;
    return effects;
}

CraftEffects corrodingAt(float level)
{
    CraftEffects effects;
    effects.corrodeLevel = level;
    effects.corrodeTime = 1.f;
    return effects;
}

WeaponRec shieldRecord(float life, uint8_t flags)
{
    WeaponRec weaponType;
    weaponType.cls = 2;
    weaponType.life = life;
    weaponType.shieldFlags = flags;
    weaponType.shieldBonus.shieldDamage = 50.f;
    return weaponType;
}

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

// crafts in slots 0..n - 1 at the given places
class PlacedCrafts final : public RaceCrafts
{
  public:
    explicit PlacedCrafts(std::vector<glm::vec3> places) : places_(std::move(places))
    {
    }
    int racerCount() const override
    {
        return int(places_.size());
    }
    int slotOf(int racer) const override
    {
        return racer;
    }
    bool flies(int slot) const override
    {
        return slot >= 0 && slot < racerCount();
    }
    glm::vec3 position(int slot) const override
    {
        return places_[size_t(slot)];
    }
    glm::mat4 frame(int) const override
    {
        return glm::mat4(1.f);
    }
    bool targetable(int) const override
    {
        return true;
    }

  private:
    std::vector<glm::vec3> places_;
};

void testTheShieldPercentIsTheValueOverTheSize()
{
    const CraftWeapon weapon = shieldAt(200.f, 50.f);
    CHECK(near(weapon.shieldPercent(), 25.f));
}

void testAShieldIsUpForItsRecordsLife()
{
    const WeaponRec weaponType = shieldRecord(2.f, 0);
    CraftWeapon weapon = fullShieldOf(100.f);
    weapon.raiseShield(weaponType, 10.f, -1);
    CHECK(weapon.shieldUp(12.f) && !weapon.shieldUp(12.1f));
}

void testARaisedShieldIsDrawnAtItsBone()
{
    const WeaponRec weaponType = shieldRecord(2.f, 0);
    CraftWeapon weapon = fullShieldOf(100.f);
    weapon.raiseShield(weaponType, 0.f, 7);
    CHECK(weapon.shield().bone == 7 && weapon.shield().weaponType == &weaponType);
}

void testAnAbsorbingShieldGuardsAgainstHits()
{
    const WeaponRec weaponType = shieldRecord(2.f, kAbsorbs);
    CraftWeapon weapon = fullShieldOf(100.f);
    weapon.raiseShield(weaponType, 0.f, -1);
    const ShieldGuard guard = weapon.guard(1.f);
    CHECK(guard.blocked && guard.absorbs && weapon.shieldBonuses(1.f).size() == 1);
}

void testNoShieldUpGivesNoGuard()
{
    const ShieldGuard guard = fullShieldOf(100.f).guard(0.f);
    CHECK(!guard.blocked && !guard.absorbs);
}

void testLosingShieldTakesAPercentOfItsSize()
{
    CraftWeapon weapon = fullShieldOf(200.f);
    weapon.loseShield(25.f);
    CHECK(near(weapon.energy(), 150.f));
}

void testANegativeLossRechargesAtMostToFull()
{
    CraftWeapon weapon = fullShieldOf(200.f);
    weapon.loseShield(-25.f);
    CHECK(near(weapon.energy(), 200.f));
}

void testGainingStopsAtFull()
{
    CraftWeapon weapon = shieldAt(100.f, 90.f);
    weapon.gainShield(30.f);
    CHECK(near(weapon.energy(), 100.f));
}

void testAKnockedOutCraftGainsNothing()
{
    CraftWeapon weapon = fullShieldOf(100.f);
    weapon.knockOut(3, 5.f);
    weapon.gainShield(30.f);
    CHECK(near(weapon.energy(), 0.f) && weapon.knockedOutBy() == 3 && near(weapon.secondsKnockedOut(6.f), 1.f));
}

void testAShieldsRechargeClearsTheEffects()
{
    CraftEffects frozen;
    frozen.freezeTime = 2.f;
    CraftWeapon weapon = afflicted(shieldAt(100.f, 50.f), frozen);
    weapon.rechargeFromShield(20.f);
    CHECK(near(weapon.energy(), 70.f) && weapon.effects().freezeTime == 0.f);
}

void testALongerEngineOffStopsTheEngine()
{
    CraftWeapon weapon = afflicted(fullShieldOf(100.f), engineOffFor(1.f));
    CHECK(weapon.afflict(engineOffFor(2.f)) && near(weapon.effects().engineOffTime, 2.f));
}

void testAShorterEngineOffKeepsTheRunningOne()
{
    CraftWeapon weapon = afflicted(fullShieldOf(100.f), engineOffFor(3.f));
    CHECK(!weapon.afflict(engineOffFor(2.f)) && near(weapon.effects().engineOffTime, 3.f));
}

void testTheEngineRestartsWhenItsOffTimeRunsOut()
{
    CraftWeapon weapon = afflicted(fullShieldOf(100.f), engineOffFor(0.01f));
    CHECK(weapon.countDownEffects(1.f / 60.f) && !weapon.countDownEffects(1.f / 60.f));
}

void testTheCorrodeEatsTheShieldButNeverKnocksOut()
{
    CraftWeapon weapon = afflicted(shieldAt(100.f, 1.f), corrodingAt(600.f));
    weapon.corrode(1.f / 60.f, 1.f);
    CHECK(near(weapon.energy(), 0.f) && !weapon.knockedOut() && weapon.shieldFlashLevel() > 0.f);
}

void testTheNemesisTakesHalfTheCorrode()
{
    CraftWeapon weapon = afflicted(fullShieldOf(100.f), corrodingAt(60.f));
    weapon.corrode(1.f / 60.f, 0.5f);
    CHECK(near(weapon.energy(), 99.5f));
}

void testTheResetKeepsTheShieldValue()
{
    CraftWeapon weapon = fullShieldOf(100.f);
    weapon.knockOut(1, 2.f);
    weapon.replayShield(40.f);
    weapon.resetForRace();
    CHECK(!weapon.knockedOut() && near(weapon.energy(), 40.f));
}

void testAHitFlashesThePlayersRowRed()
{
    CraftWeapon weapon = fullShieldOf(100.f);
    weapon.flashPlayerHit();
    CHECK(weapon.hitFlashLevel() > 0.f && weapon.shieldFlashLevel() == 0.f);
}

void testAnAiPickupRaisesItsLoadoutWithinTheCaps()
{
    CraftWeapon weapon = fullShieldOf(100.f);
    const int bluePickup = 0;
    weapon.raiseAiLoadout(bluePickup, PowerLevels{1, 1, 1, 1});
    weapon.raiseAiLoadout(bluePickup, PowerLevels{1, 1, 1, 1});
    CHECK(weapon.aiLoadout().blue == 1 && weapon.aiLoadout().red == 0);
}

void testTheSphereReachesEachOtherCraftOnce()
{
    CraftWeapon weapon = fullShieldOf(100.f);
    const PlacedCrafts crafts({glm::vec3(0.f), glm::vec3(5.f, 0.f, 0.f), glm::vec3(50.f, 0.f, 0.f)});
    const ShieldSphereAt sphere{0, glm::vec3(0.f), 10.f};
    const std::vector<int> first = weapon.reachWithSphere(sphere, crafts);
    CHECK(first == std::vector<int>({1}) && weapon.reachWithSphere(sphere, crafts).empty());
}
}  // namespace

int main()
{
    testTheShieldPercentIsTheValueOverTheSize();
    testAShieldIsUpForItsRecordsLife();
    testARaisedShieldIsDrawnAtItsBone();
    testAnAbsorbingShieldGuardsAgainstHits();
    testNoShieldUpGivesNoGuard();
    testLosingShieldTakesAPercentOfItsSize();
    testANegativeLossRechargesAtMostToFull();
    testGainingStopsAtFull();
    testAKnockedOutCraftGainsNothing();
    testAShieldsRechargeClearsTheEffects();
    testALongerEngineOffStopsTheEngine();
    testAShorterEngineOffKeepsTheRunningOne();
    testTheEngineRestartsWhenItsOffTimeRunsOut();
    testTheCorrodeEatsTheShieldButNeverKnocksOut();
    testTheNemesisTakesHalfTheCorrode();
    testTheResetKeepsTheShieldValue();
    testAHitFlashesThePlayersRowRed();
    testAnAiPickupRaisesItsLoadoutWithinTheCaps();
    testTheSphereReachesEachOtherCraftOnce();
    std::printf("craft_weapon_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
