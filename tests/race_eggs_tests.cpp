// Argon: tests for the easter eggs a race runs with (logic/race_eggs.hpp) and the Fruity Fruit pickup
// models (pickup_folders.hpp). Run with test.bat / test.sh.
#include "logic/race_eggs.hpp"
#include "check.hpp"
#include "track/pickup_folders.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

void switchedOnEggsHoldInTheRace()
{
    const bool eggsOn[RaceEggs::kCount] = {false, false, false, true, false, false, false, true};
    RaceEggs eggs;
    eggs.switchOn(eggsOn);
    CHECK(eggs.on(Egg::Slide) && eggs.on(Egg::MirrorTracks) && !eggs.on(Egg::UltraFocalLength) && !eggs.on(Egg::WeaponsOff));
}

void backToTheFrontEndClearsEveryEgg()
{
    const bool eggsOn[RaceEggs::kCount] = {true, true, true, true, true, true, true, true};
    RaceEggs eggs;
    eggs.switchOn(eggsOn);
    eggs.clear();
    CHECK(!eggs.on(Egg::UltraFocalLength) && !eggs.on(Egg::AtlantisMode) && !eggs.on(Egg::MirrorTracks));
}

void fruityFruitPowerUpsUseTheEeModels()
{
    CHECK(fruityPickupFolder("POWERUPRED") == "powerup_red_ee");
}

void fruityFruitTakenModelsKeepTheNsSuffixLast()
{
    CHECK(fruityPickupFolder("POWERUPBLUE_NS") == "powerup_blue_ee_ns");
}

void fruityFruitChipAndMegaUpUseTheEeModels()
{
    CHECK(fruityPickupFolder("POWERCHIP") == "powerchip_ee" && fruityPickupFolder("MEGAUP_NS") == "megaup_ee_ns");
}
}  // namespace

int main()
{
    switchedOnEggsHoldInTheRace();
    backToTheFrontEndClearsEveryEgg();
    fruityFruitPowerUpsUseTheEeModels();
    fruityFruitTakenModelsKeepTheNsSuffixLast();
    fruityFruitChipAndMegaUpUseTheEeModels();
    std::printf("race eggs tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
