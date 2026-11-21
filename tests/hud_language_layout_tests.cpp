// Argon: tests for the race HUD's positions per language (hud/hud_language_layout.hpp). Run with test.bat / test.sh.
#include "hud/hud_language_layout.hpp"
#include "check.hpp"

#include <cstdio>

using namespace argon::hud;

namespace
{
using argon::test::checks;
using argon::test::failures;

void englishPutsTheShieldLineAtSixtyOne()
{
    const HudLanguageLayout layout = hudLanguageLayout(Language::English);

    CHECK(layout.shieldBarTop == 61.f && layout.shieldBarDrop == 0.f);
}

void japanesePutsTheShieldLineAtSeventyNineWithTheBarThreeLower()
{
    const HudLanguageLayout layout = hudLanguageLayout(Language::Japanese);

    CHECK(layout.shieldBarTop == 79.f && layout.shieldBarDrop == 3.f);
}

void japaneseDropsTheShieldValueThreeAndAHalf()
{
    const HudLanguageLayout layout = hudLanguageLayout(Language::Japanese);

    CHECK(layout.shieldValueDrop == 3.5f);
}

void japaneseSpacesTheTimeRowsNineteenApart()
{
    const HudLanguageLayout layout = hudLanguageLayout(Language::Japanese);

    CHECK(layout.timeRowStep == 19.f);
}

void japaneseMovesTheSpeedReadOut()
{
    const HudLanguageLayout layout = hudLanguageLayout(Language::Japanese);

    CHECK(layout.speedValueLeft == 58.f && layout.speedValueAbove == 36.f && layout.speedUnitAbove == 26.f);
}

void japaneseMovesTheTurboTexts()
{
    const HudLanguageLayout layout = hudLanguageLayout(Language::Japanese);

    CHECK(layout.turboTextDrop == 0.5f && layout.splitTurboTextDrop == 3.5f);
}

void everyOtherLanguageTakesTheEnglishLayout()
{
    const HudLanguageLayout english = hudLanguageLayout(Language::English);

    const HudLanguageLayout german = hudLanguageLayout(Language::German);

    CHECK(german.shieldBarTop == english.shieldBarTop && german.timeRowStep == english.timeRowStep && german.scoreValueTop == english.scoreValueTop);
}
}  // namespace

int main()
{
    englishPutsTheShieldLineAtSixtyOne();
    japanesePutsTheShieldLineAtSeventyNineWithTheBarThreeLower();
    japaneseDropsTheShieldValueThreeAndAHalf();
    japaneseSpacesTheTimeRowsNineteenApart();
    japaneseMovesTheSpeedReadOut();
    japaneseMovesTheTurboTexts();
    everyOtherLanguageTakesTheEnglishLayout();
    std::printf("hud_language_layout_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
