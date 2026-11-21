// Argon: tests for weapon.nwp's book (logic/weapon_book.hpp: 0x640c0, 0x64030), the explosions in flight
// (logic/explosions.hpp: 0x65f80, 0x7e0f0, 0x7de10, 0x6a8a0) and the spray pool's weapon sprites
// (effects/weapon_sprites.hpp: 0x9d000, 0xbd670, 0xbd5f0). No window or GPU: run with test.bat / test.sh (or ctest in
// build).
#include "effects/weapon_sprites.hpp"
#include "logic/explosions.hpp"
#include "logic/weapon_book.hpp"
#include "model/cmm.hpp"
#include "check.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon;
using namespace argon::weapons;

const size_t kPilotBytes = original::kNwpPilotCount * sizeof(original::NwpPilot);
const float kPushOfLastType = 7.f;

// a weapon.nwp of zeros but for every explosion type lasting its order's time at full size, the last one pushing
std::vector<uint8_t> nwpBytes()
{
    std::vector<uint8_t> bytes(kPilotBytes + original::kNwpExplosionTypes * sizeof(original::NwpExplosionType), 0);
    for (int t = 0; t < original::kNwpExplosionTypes; ++t)
    {
        original::NwpExplosionType type{};
        type.animationScale = 1.f;
        type.durationScale = 1.f;
        type.sizeScale = 100.f;
        type.pushScale = t == original::kNwpExplosionTypes - 1 ? kPushOfLastType : 0.f;
        std::memcpy(bytes.data() + kPilotBytes + size_t(t) * sizeof type, &type, sizeof type);
    }
    return bytes;
}

WeaponBook loadedBook()
{
    WeaponBook book;
    book.read(nwpBytes());
    return book;
}

// a weapon whose burst A starts `count` explosions on a craft or world hit
WeaponRec burstingWeapon(uint8_t count)
{
    WeaponRec weaponType;
    weaponType.impact.flagsA = 1;
    weaponType.impact.countA = count;
    weaponType.impact.sizePercentA = 100;
    return weaponType;
}

TrailSprite spriteIn(bool additive)
{
    TrailSprite sprite;
    sprite.life = 1.f;
    sprite.halfSize = 1.f;
    sprite.additive = additive;
    return sprite;
}

void testAShortFileGivesNoRecords()
{
    WeaponBook book;
    book.read(std::vector<uint8_t>(10, 0));
    CHECK(!book.hasRecords());
}

void testAFullFileGivesEveryPilotsRecordsInTheirSlots()
{
    CHECK(loadedBook().hasRecords() && loadedBook().record(15, 9).slot == 9);
}

void testTheShieldSizeIs20PerLevelPlus100WithoutResistance()
{
    CHECK(std::abs(loadedBook().shieldSize(0, 2) - 140.f) < 1e-4f);
}

void testAnExplosionTypeOutOfRangeIsTheLastOne()
{
    CHECK(loadedBook().explosionType(99).push == kPushOfLastType);
}

void testAnImpactStartsTheExplosionsItsBurstAsksFor()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    const WeaponRec weaponType = burstingWeapon(3);
    const int asked = explosions.startImpact(Impact{glm::vec3(0.f), weaponType, 0, 1}, ExplosionClock{0.f, rand});
    CHECK(asked == 3 && explosions.inFlight().size() == 3);
}

void testAnImpactOfAnotherKindStartsNothing()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    const WeaponRec weaponType = burstingWeapon(3);
    CHECK(explosions.startImpact(Impact{glm::vec3(0.f), weaponType, 0, 3}, ExplosionClock{0.f, rand}) == 0);
}

void testAKnockOutStartsFourExplosions()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    explosions.startKnockOut(glm::vec3(0.f), ExplosionClock{0.f, rand});
    CHECK(explosions.inFlight().size() == 4 && explosions.takeStartedModels().size() == 4);
}

void testTheStartedModelsAreHandedOutOnce()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    explosions.startKnockOut(glm::vec3(0.f), ExplosionClock{0.f, rand});
    explosions.takeStartedModels();
    CHECK(explosions.takeStartedModels().empty());
}

void testTheKnockOutsBlastAndFirstEchoGoOffAtOnce()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    explosions.startKnockOut(glm::vec3(0.f), ExplosionClock{0.f, rand});
    CHECK(explosions.update(0.f).size() == 2);   // echo k is k x 0.2 s late
}

void testAnExplosionGoesOffOnce()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    explosions.startKnockOut(glm::vec3(0.f), ExplosionClock{0.f, rand});
    explosions.update(0.f);
    CHECK(explosions.update(0.f).empty());
}

void testSpentExplosionsLeave()
{
    const WeaponBook book = loadedBook();
    Explosions explosions(book);
    ViewerRand rand;
    explosions.startKnockOut(glm::vec3(0.f), ExplosionClock{0.f, rand});
    explosions.update(100.f);
    CHECK(explosions.inFlight().empty());
}

void testThePoolTakesNoMoreSpritesWhenFull()
{
    WeaponSprites sprites;
    for (int i = 0; i < WeaponSprites::kMaxSprites + 5; ++i)
    {
        sprites.add(spriteIn(true));
    }
    CHECK(sprites.sprites() == size_t(WeaponSprites::kMaxSprites) && !sprites.hasRoom());
}

void testDroppingTheFlashesKeepsTheSprites()
{
    WeaponSprites sprites;
    sprites.add(spriteIn(true));
    sprites.addFlash(WeaponFlash{});
    sprites.dropFlashes();
    CHECK(sprites.sprites() == 1);
}

void testTheQuadsAreTheAlphaListThenTheAdditiveListAndTheFlashes()
{
    WeaponSprites sprites;
    sprites.add(spriteIn(false));
    sprites.add(spriteIn(true));
    sprites.addFlash(WeaponFlash{});
    std::vector<Vertex> out(WeaponSprites::kMaxVertices);
    const WeaponSprites::Written written = sprites.writeQuads(glm::mat4(1.f), 0.f, out.data());
    CHECK(written.alpha == 6 && written.additive == 12);
}

void testSpritesGoAtTheEndOfTheirLife()
{
    WeaponSprites sprites;
    sprites.add(spriteIn(true));
    sprites.age(2.f, 1.f / 60.f);
    CHECK(sprites.empty());
}
}  // namespace

int main()
{
    testAShortFileGivesNoRecords();
    testAFullFileGivesEveryPilotsRecordsInTheirSlots();
    testTheShieldSizeIs20PerLevelPlus100WithoutResistance();
    testAnExplosionTypeOutOfRangeIsTheLastOne();
    testAnImpactStartsTheExplosionsItsBurstAsksFor();
    testAnImpactOfAnotherKindStartsNothing();
    testAKnockOutStartsFourExplosions();
    testTheStartedModelsAreHandedOutOnce();
    testTheKnockOutsBlastAndFirstEchoGoOffAtOnce();
    testAnExplosionGoesOffOnce();
    testSpentExplosionsLeave();
    testThePoolTakesNoMoreSpritesWhenFull();
    testDroppingTheFlashesKeepsTheSprites();
    testTheQuadsAreTheAlphaListThenTheAdditiveListAndTheFlashes();
    testSpritesGoAtTheEndOfTheirLife();
    std::printf("weapons_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
