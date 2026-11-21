// Argon: tests for the shield bar's fill colour (hud/shield_bar_rules.hpp). Run with test.bat / test.sh.
#include "hud/shield_bar_rules.hpp"
#include "check.hpp"

#include <cstdio>

using namespace argon::hud;

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr uint32_t kYellow = 0xffffd000u, kRed = 0xffff0000u, kWhite = 0xffffffffu;

void aHealthyShieldWithoutFlashIsYellow()
{
    const float shield = 80.f, flash = 0.f;

    const uint32_t colour = shieldFillColour(shield, flash, 3);

    CHECK(colour == kYellow);
}

void aFlashTurnsTheFillWhiteOnTheFirstHalfOfTheBlink()
{
    const float shield = 80.f, flash = 0.5f;

    const uint32_t colour = shieldFillColour(shield, flash, 7);

    CHECK(colour == kWhite);
}

void aFlashLeavesTheFillYellowOnTheSecondHalfOfTheBlink()
{
    const float shield = 80.f, flash = 0.5f;

    const uint32_t colour = shieldFillColour(shield, flash, 8);

    CHECK(colour == kYellow);
}

void theBlinkRepeatsEverySixteenBlanks()
{
    const float shield = 80.f, flash = 0.5f;

    const uint32_t colour = shieldFillColour(shield, flash, 16);

    CHECK(colour == kWhite);
}

void aLowShieldTurnsTheFillRedOnTheSecondHalfOfTheBlink()
{
    const float shield = 0.2f, flash = 0.f;

    const uint32_t colour = shieldFillColour(shield, flash, 15);

    CHECK(colour == kRed);
}

void aLowShieldStaysYellowOnTheFirstHalfOfTheBlink()
{
    const float shield = 0.2f, flash = 0.f;

    const uint32_t colour = shieldFillColour(shield, flash, 0);

    CHECK(colour == kYellow);
}

void aShieldAtTheThresholdIsNotLow()
{
    const float shield = 0.33f, flash = 0.f;

    const uint32_t colour = shieldFillColour(shield, flash, 8);

    CHECK(colour == kYellow);
}

void aTwentyPercentShieldIsNotLowAsTheGameComparesThePercentWithAFraction()
{
    const float shield = 20.f, flash = 0.f;

    const uint32_t colour = shieldFillColour(shield, flash, 8);

    CHECK(colour == kYellow);
}

void aFlashingLowShieldAlternatesWhiteAndRed()
{
    const float shield = 0.1f, flash = 1.f;

    const bool alternates = shieldFillColour(shield, flash, 4) == kWhite && shieldFillColour(shield, flash, 12) == kRed;

    CHECK(alternates);
}
}  // namespace

int main()
{
    aHealthyShieldWithoutFlashIsYellow();
    aFlashTurnsTheFillWhiteOnTheFirstHalfOfTheBlink();
    aFlashLeavesTheFillYellowOnTheSecondHalfOfTheBlink();
    theBlinkRepeatsEverySixteenBlanks();
    aLowShieldTurnsTheFillRedOnTheSecondHalfOfTheBlink();
    aLowShieldStaysYellowOnTheFirstHalfOfTheBlink();
    aShieldAtTheThresholdIsNotLow();
    aTwentyPercentShieldIsNotLowAsTheGameComparesThePercentWithAFraction();
    aFlashingLowShieldAlternatesWhiteAndRed();
    std::printf("shield_bar_rules_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
