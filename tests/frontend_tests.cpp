// Argon: tests for the front-end leftovers' logic, no window or GPU: the rotated quad (0xcbe50), the
// Nemesis Challenge intro (0x808e0), FE MESSAGEBOX's buttons (0x2b000 / 0x2b4b0), FE TOURNEYTYPE's icons (0x35e50),
// UPGRADEMENU's weapon icons (0x9f040), the nemesis race and the speedometer's needle, "Percentage Complete" (0xc03a0), the Trophy Level (0x82910)
// and the UI sounds per page (sound_play_ui 0xc3ca0's callers)
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>

#include "logic/lap_record_rules.hpp"
#include "check.hpp"
#include "logic/profile_progress.hpp"
#include "logic/tourney_awards.hpp"
#include "logic/tourney_rules.hpp"
#include "logic/tourney_win.hpp"
#include "ui/fe_sounds.hpp"
#include "ui/keyboard_moves.hpp"
#include "ui/menu_common.hpp"
#include "ui/message_buttons.hpp"
#include "ui/nemesis_intro.hpp"
#include "ui/pause_rules.hpp"
#include "ui/rotated_quad.hpp"
#include "ui/title_texts.hpp"
#include "ui/tourney_icons.hpp"
#include "ui/upgrade_icons.hpp"
#include "logic/upgrade_rules.hpp"
#include "ui/viewer_text.hpp"
#include "original/text_records.hpp"

namespace
{
using argon::test::checks;
using argon::test::failures;
#define CHECK_NEAR(a, b, eps) CHECK(std::fabs(double(a) - double(b)) <= (eps))

bool near(glm::vec2 a, glm::vec2 b)
{
    return std::fabs(a.x - b.x) < 1e-3f && std::fabs(a.y - b.y) < 1e-3f;
}

// ---- hud_draw_rotated_quad
void testTrackDataQuadIsUprightAcrossTheScreen()
{
    // FE TRACK INFO: (320, 150), angle 0 (+ 180), from -128 to 128, 640 wide
    const menu::RotatedQuad q = menu::rotatedQuad({{320.f, 150.f}, 0.f, -128.f, 128.f, 640.f});
    CHECK(near(q.corners[0], {640.f, 22.f}));
    CHECK(near(q.corners[1], {0.f, 22.f}));
    CHECK(near(q.corners[2], {640.f, 278.f}));
    CHECK(near(q.corners[3], {0.f, 278.f}));
}

void testRotatedQuadTexturesTheTopRightCornerWithURight()
{
    const menu::RotatedQuad q = menu::rotatedQuad({{320.f, 150.f}, 0.f, -128.f, 128.f, 640.f});
    CHECK(near(q.uvs[0], {1.f, 0.f}));   // (640, 22): the picture is not mirrored
    CHECK(near(q.uvs[3], {0.f, 1.f}));
}

void testIdCardQuadTurnsAboutItsPivot()
{
    // FE CHOOSESAVE: (320, 260), -20 deg, from -160 to 160: the four corners stay 160 / 320 from the pivot
    const menu::RotatedQuad q = menu::rotatedQuad({{320.f, 260.f}, -20.f, -160.f, 160.f, 640.f});
    const glm::vec2 centre = (q.corners[0] + q.corners[1] + q.corners[2] + q.corners[3]) * 0.25f;
    CHECK(near(centre, {320.f, 260.f}));
    CHECK_NEAR(glm::length(q.corners[0] - q.corners[1]), 640.f, 1e-2);
    CHECK_NEAR(glm::length(q.corners[0] - q.corners[2]), 320.f, 1e-2);
}

// ---- the Nemesis Challenge intro
void testIntroShowsForSixSeconds()
{
    CHECK(menu::NemesisIntro::showing(0.f));
    CHECK(menu::NemesisIntro::showing(5.99f));
    CHECK(!menu::NemesisIntro::showing(6.f));
}

void testIntroStartsWithTwoSweepsAndSixteenCopies()
{
    CHECK(menu::NemesisIntro::texts(0.f).size() == 18u);
    CHECK(menu::NemesisIntro::texts(2.f).size() == 20u);   // + the yellow text and its echo
}

void testIntroSweepsCrossTheScreen()
{
    const auto texts = menu::NemesisIntro::texts(1.f);
    CHECK_NEAR(texts[0].x, 100.f, 1e-3);   // 800 - 700 t
    CHECK_NEAR(texts[1].x, 1000.f, 1e-3);
    CHECK(texts[0].colour == 0x60ff4040u);
    CHECK(texts[1].colour == 0x80ff4040u);
}

void testIntroCopiesStartTurnedAndLarge()
{
    const auto texts = menu::NemesisIntro::texts(0.f);
    const menu::IntroText &first = texts[2], &last = texts[17];
    CHECK_NEAR(first.angleDegrees, 1280.f, 1e-3);           // 320 x 4
    CHECK_NEAR(last.angleDegrees, 1280.f - 32.f * 15.f, 1e-3);
    CHECK_NEAR(first.scale, 2.5f * 4.f + 1.4f, 1e-4);
    CHECK(first.colour == 0x00ff4040u);
    CHECK(last.colour == 0xf0ff4040u);
}

void testIntroCopiesSettleByFourSeconds()
{
    for (const menu::IntroText &t : menu::NemesisIntro::texts(4.f))
    {
        CHECK(t.angleDegrees == 0.f);
    }
    CHECK_NEAR(menu::NemesisIntro::texts(4.f)[2].scale, 1.2f, 1e-6);
}

void testIntroEchoFadesOutAfterThreeSeconds()
{
    const auto texts = menu::NemesisIntro::texts(3.f);
    CHECK(texts[18].colour == 0xffffb040u);
    CHECK((texts.back().colour >> 24) == 0u);
    CHECK_NEAR(texts.back().scale, 21.2f, 1e-4);
}

// ---- FE MESSAGEBOX
void testYesNoOpensOnNo()
{
    CHECK(menu::MessageButtons(menu::MessageKind::YesNo).first() == 1);
    CHECK(menu::MessageButtons(menu::MessageKind::ReplaceGhost).first() == 1);
    CHECK(menu::MessageButtons(menu::MessageKind::Info).first() == 0);
}

void testNoFreeBlocksHasThreeButtons()
{
    const auto profile = menu::MessageButtons(menu::MessageKind::NoFreeBlocksProfile).labels();
    const auto ghost = menu::MessageButtons(menu::MessageKind::NoFreeBlocksGhost).labels();
    CHECK(profile.size() == 3u && ghost.size() == 3u);
    CHECK(profile[1].feString == 0x246);
    CHECK(ghost[1].feString == 0x247);
    CHECK(ghost[2].feString == 0x245);
}

void testThreeButtonsGoRound()
{
    const menu::MessageButtons b(menu::MessageKind::NoFreeBlocksGhost);
    CHECK(b.moved(0, false) == 2);
    CHECK(b.moved(2, true) == 0);
    CHECK(b.moved(1, true) == 2);
}

void testKindSevenTakesNoInput()
{
    CHECK(!menu::MessageButtons(menu::MessageKind::NoFreeBlocks).takesCursorInput());
    CHECK(menu::MessageButtons(menu::MessageKind::NoFreeBlocks).count() == 3);
}

void testTimedBoxHasNoButton()
{
    CHECK(menu::MessageButtons(menu::MessageKind::Timed).count() == 0);
}

// ---- FE TOURNEYTYPE
void testTrophyComesFirstThenTheRaces()
{
    const auto icons = menu::tourneyRowIcons({2, 200.f, true, 1, 4});
    CHECK(icons.size() == 5u);
    CHECK(icons[0].icon == menu::TourneyIcon::Trophy && icons[0].x == 240.f && icons[0].y == 170.f);
    CHECK(icons[1].icon == menu::TourneyIcon::RaceWon && icons[1].x == 280.f && icons[1].y == 175.f);
    CHECK(icons[2].icon == menu::TourneyIcon::RaceToGo && icons[4].x == 370.f);
}

void testFinishedTournamentShowsOnlyItsTrophy()
{
    CHECK(menu::tourneyRowIcons({0, 150.f, true, -1, 2}).size() == 1u);
    CHECK(menu::tourneyRowIcons({0, 150.f, false, 0, 2}).empty());
}

void testNemesisRaces()
{
    CHECK(!argon::tourney::isNemesisRace(0, 1));
    CHECK(argon::tourney::isNemesisRace(1, 3) && argon::tourney::isNemesisRace(2, 3));
    CHECK(argon::tourney::isNemesisRace(3, 4) && !argon::tourney::isNemesisRace(3, 3));
    CHECK(argon::tourney::isNemesisRace(4, 0) && !argon::tourney::isNemesisRace(4, 8));
}

void testNemesisRaceSeatsOnlyTheNemesis()
{
    CHECK(argon::tourney::droneSeats(1, 3, 5) == 1);
    CHECK(argon::tourney::droneSeats(4, 0, 5) == 1);
}

void testOtherTournamentRacesKeepTheChosenDrones()
{
    CHECK(argon::tourney::droneSeats(1, 2, 5) == 5);
    CHECK(argon::tourney::droneSeats(0, 3, 4) == 4);
}

void testSpeedoNeedle()
{
    CHECK(argon::tourney::speedoNeedleDegrees(0) == 0.f);
    CHECK(argon::tourney::speedoNeedleDegrees(3) == 105.f);
    CHECK(argon::tourney::speedoNeedleDegrees(4) == 140.f);
}

// ---- Percentage Complete, Trophy Level
void testNewProfileIsAtZeroPercent()
{
    argon::ProfileProgress p;
    p.characters = 0x007f;
    CHECK(p.percentComplete() == 0);
}

void testTrophyLevelIsTheBestLevelPlusOne()
{
    argon::ProfileProgress p;
    p.trophies[3][0] = p.trophies[3][2] = true;
    CHECK(p.trophyLevel(3) == 3);
    CHECK(p.trophyLevel(4) == 0);
}

void testEverythingIsAHundredPercent()
{
    argon::ProfileProgress p;
    for (auto &levels : p.trophies)
    {
        levels.fill(true);
    }
    p.characters = 0xffff;
    p.eggs = 0xff;
    p.redshiftWins = 7;
    CHECK(p.percentComplete() == 100);
}

void testRedshiftWinWithTheEggCountsTwice()
{
    argon::ProfileProgress p;
    p.redshiftWins = uint32_t(argon::RedshiftWin::WithFirstEgg);
    CHECK(p.percentComplete() == 2);
}

void testKaHekiliDoesNotCount()
{
    argon::ProfileProgress p;
    p.characters = 0x00ff;
    CHECK(p.percentComplete() == 0);
}

// ---- FE NAME ENTRY's keyboard (the letters page: 0 [Delete] 1..0, 1 [CAPS] A..J, 2 [Accents] K..T, 3 U..Z Space,
// 4 Done)
class LettersPage : public menu::KeyLayout
{
  public:
    int code(int row, int col) const override
    {
        if (col < 0 || col >= rowLength(row))
        {
            return 0;
        }
        if (col == 0)
        {
            const int specials[5] = {0x10000, 0x10002, kAccentsKey, 0, 0};
            return specials[row];
        }
        if (row == 4)
        {
            return 0x10005;
        }
        return row == 3 && col == 7 ? ' ' : 'a' + row * 10 + col;
    }
    int rowLength(int row) const override
    {
        const int lengths[5] = {11, 11, 11, 8, 2};
        return lengths[row];
    }
};

menu::KeyCursor at(int row, int col, int remembered = 0)
{
    return {row, col, remembered};
}

bool same(const menu::KeyCursor &a, const menu::KeyCursor &b)
{
    return a.row == b.row && a.col == b.col && a.remembered == b.remembered;
}

void testDownFromTheRightOfTheThirdRowLandsOnSpace()
{
    const LettersPage keys;
    CHECK(same(menu::KeyboardMoves(keys, false).down(at(2, 9)), at(3, 7, 9)));
}

void testDownFromALetterLandsOnDoneAndBackUp()
{
    const LettersPage keys;
    const menu::KeyboardMoves moves(keys, false);
    const menu::KeyCursor done = moves.down(at(3, 4));
    CHECK(same(done, at(4, 1, 4)));
    CHECK(same(moves.up(done), at(3, 4, 4)));
}

void testDownFromDoneGoesRoundToTheRememberedColumn()
{
    const LettersPage keys;
    CHECK(same(menu::KeyboardMoves(keys, false).down(at(4, 1, 9)), at(0, 9, 9)));
}

void testUpFromTheTopRowGoesRoundOntoDone()
{
    const LettersPage keys;
    CHECK(same(menu::KeyboardMoves(keys, false).up(at(0, 5)), at(4, 1, 5)));
}

void testUpFromSpaceReturnsToTheRememberedColumn()
{
    const LettersPage keys;
    CHECK(same(menu::KeyboardMoves(keys, false).up(at(3, 7, 9)), at(2, 9, 9)));
}

void testUpFromDeleteSkipsTheEmptyPlaces()
{
    const LettersPage keys;
    CHECK(same(menu::KeyboardMoves(keys, false).up(at(0, 0)), at(2, 0)));
}

void testGermanSkipsTheAccentsKey()
{
    const LettersPage keys;
    CHECK(same(menu::KeyboardMoves(keys, true).up(at(0, 0)), at(1, 0)));
    CHECK(same(menu::KeyboardMoves(keys, true).left(at(2, 1)), at(2, 10)));
}

void testLeftAndRightGoRound()
{
    const LettersPage keys;
    const menu::KeyboardMoves moves(keys, false);
    CHECK(same(moves.right(at(3, 7)), at(3, 1)));
    CHECK(same(moves.left(at(0, 0)), at(0, 10)));
}

// ---- the lap records
void testAnyModeSetsTheRecordUnderTheTable()
{
    const argon::laps::LapVerdict v = argon::laps::judgeLap(50.f, {60.f, 40.f, 55.f}, false);
    CHECK(v.raceBest && v.record && !v.profileBest);
}

void testTimeAttackNeedsTheProfileBestFirst()
{
    const argon::laps::LapVerdict v = argon::laps::judgeLap(50.f, {60.f, 45.f, 55.f}, true);
    CHECK(v.raceBest && !v.profileBest && !v.record);
}

void testTimeAttackProfileBestWithoutRecord()
{
    const argon::laps::LapVerdict v = argon::laps::judgeLap(50.f, {60.f, 52.f, 48.f}, true);
    CHECK(v.profileBest && !v.record);
}

void testASlowerLapBeatsNothing()
{
    const argon::laps::LapVerdict v = argon::laps::judgeLap(61.f, {60.f, 999999.f, 999999.f}, true);
    CHECK(!v.raceBest && !v.profileBest && !v.record);
}

void testTrackRecordsOpenOnTheSecondRoutes()
{
    std::array<bool, argon::laps::kTracks> fell{};
    fell[9] = true;
    CHECK(argon::laps::opensOnSecondRoutes(fell));
    fell[0x10] = true;
    CHECK(!argon::laps::opensOnSecondRoutes(fell));
}

// ---- FE AWARDS' ticker (0x141cf)
void testAwardsHideTheTickerWithoutAnEgg()
{
    TourneyAwards awards;
    awards.trophy = 0;
    CHECK(!awardsShowTicker(awards));
}

void testAwardsShowTheTickerWithAnEgg()
{
    TourneyAwards awards;
    awards.trophy = 4;
    awards.easterEgg = 0;
    CHECK(awardsShowTicker(awards));
}

void testFlashFallsAndWraps()
{
    CHECK(menu::stepFlash(255) == 239);
    CHECK(menu::stepFlash(80) == 64);
    CHECK(menu::stepFlash(79) == 255);
}

void testPausePickIsSilentOnAVolumeRow()
{
    CHECK(menu::pickSound(true) == menu::PauseSound::None);
    CHECK(menu::pickSound(false) == menu::PauseSound::Select);
}

void testPauseVolumeSoundsOnlyWhenItMoves()
{
    CHECK(menu::steppedOption(7, true, 8) == 7);
    CHECK(menu::steppedOption(0, false, 8) == 0);
    CHECK(menu::optionSound(7, menu::steppedOption(7, true, 8)) == menu::PauseSound::None);
    CHECK(menu::optionSound(4, menu::steppedOption(4, false, 8)) == menu::PauseSound::Navigate);
}

void testPauseHudRowStopsAtItsLastOption()
{
    CHECK(menu::steppedOption(2, true, 3) == 2);
}

void testSavingRaceLastsOneSecond()
{
    menu::SavingRace saving;
    saving.start(10.0);
    CHECK(saving.active());
    CHECK(!saving.over(10.9));
    CHECK(saving.over(11.01));
}

menu::PadWatch::PerPlayer onlyPlayer(int p)
{
    menu::PadWatch::PerPlayer v{};
    v[size_t(p)] = true;
    return v;
}

void testPullingAPadOutPausesBehindItsMessage()
{
    menu::PadWatch watch;
    watch.update(onlyPlayer(0), {});
    CHECK(watch.update({}, {}) == menu::PadWatch::Event::Pause);
    CHECK(watch.messageShown(0));
    CHECK(watch.holdsMenu());
}

void testAKeyboardOnlyPlayerIsNeverAsked()
{
    menu::PadWatch watch;
    CHECK(watch.update({}, {}) == menu::PadWatch::Event::None);
    CHECK(!watch.messageShown(0));
}

void testReconnectingWaitsForStart()
{
    menu::PadWatch watch;
    watch.update(onlyPlayer(0), {});
    watch.update({}, {});
    CHECK(watch.update(onlyPlayer(0), {}) == menu::PadWatch::Event::None);
    CHECK(watch.messageShown(0));
}

void testStartOnTheReconnectedPadOpensThePauseMenu()
{
    menu::PadWatch watch;
    watch.update(onlyPlayer(0), {});
    watch.update({}, {});
    CHECK(watch.update(onlyPlayer(0), onlyPlayer(0)) == menu::PadWatch::Event::OpenMenu);
    CHECK(!watch.messageShown(0));
    CHECK(!watch.holdsMenu());
}

void testStartOnAnotherPadDoesNotClearTheWait()
{
    menu::PadWatch watch;
    const menu::PadWatch::PerPlayer both = {true, true, false, false};
    watch.update(both, {});
    watch.update(onlyPlayer(1), {});
    CHECK(watch.update(both, onlyPlayer(1)) == menu::PadWatch::Event::None);
    CHECK(watch.messageShown(0));
}
}  // namespace

// ---- UPGRADEMENU's weapon icons
void testUpgradeIconsStartWithTheShadowedBase()
{
    const auto layers = menu::upgradeWeaponIcons(3, 128);
    CHECK(layers[0].texture == "weaponbase" && layers[2].argb == 0xff000000u && layers[3].argb == 0xffffffffu);
}

void testLevelZeroShowsTheFirstCapsAndPulsesBlueTwo()
{
    const auto layers = menu::upgradeWeaponIcons(0, 200);
    CHECK(layers.size() == 8u);
    CHECK(layers[4].texture == "weaponred1" && layers[5].texture == "weaponblue1" && layers[6].texture == "weaponyellow1");
    CHECK(layers[7].texture == "weaponblue2" && layers[7].argb == 0xffc8c8c8u && layers[7].blend == menu::IconBlend::Additive);
}

void testLevelThreePulsesBlueThree()
{
    const auto layers = menu::upgradeWeaponIcons(3, 128);
    CHECK(layers[4].texture == "weaponred2" && layers[5].texture == "weaponblue2" && layers[6].texture == "weaponyellow2");
    CHECK(layers[7].texture == "weaponblue3");
}

void testLevelFivePulsesRedThree()
{
    CHECK(menu::upgradeWeaponIcons(5, 128)[7].texture == "weaponred3");
}

void testLevelSixFadesTheSpecialInOverAllCaps()
{
    const auto layers = menu::upgradeWeaponIcons(6, 0x40);
    CHECK(layers[4].texture == "weaponred3" && layers[6].texture == "weaponyellow3");
    CHECK(layers[7].texture == "weaponspecial" && layers[7].argb == 0x40ffffffu && layers[7].blend == menu::IconBlend::Alpha);
}

void testLevelSevenShowsOnlyTheBaseAndTheSpecial()
{
    const auto layers = menu::upgradeWeaponIcons(7, 128);
    CHECK(layers.size() == 5u && layers[4].texture == "weaponspecial" && layers[4].argb == 0xffffffffu);
}

void testTournamentsRunTwoToNineRaces()
{
    CHECK(argon::tourney::racesIn(0) == 2 && argon::tourney::racesIn(3) == 5 && argon::tourney::racesIn(4) == 9);
}

void testTournamentStartsOnTheHomeTrack()
{
    CHECK(argon::tourney::trackOf(1, 3, 0) == 3 && argon::tourney::trackOf(2, 8, 0) == 8);
}

void testNemesisRaceIsOnTheHomeTracksOtherRoute()
{
    CHECK(argon::tourney::trackOf(1, 0, 3) == 8 && argon::tourney::trackOf(1, 8, 3) == 0);
}

void testMasterFourthRaceIsDracoUnlessDracoIsHome()
{
    CHECK(argon::tourney::trackOf(3, 0, 3) == 7 && argon::tourney::trackOf(3, 7, 3) == 9);
}

void testRedshiftStartsAndEndsOnTheHomeTrack()
{
    CHECK(argon::tourney::trackOf(4, 0, 0) == 8 && argon::tourney::trackOf(4, 0, 8) == 0 && argon::tourney::trackOf(4, 8, 8) == 8);
}

void testExpertTakesTheTableFromItsThirdTrack()
{
    CHECK(argon::tourney::trackOf(2, 0, 1) == 5 && argon::tourney::trackOf(2, 7, 2) == 14);
}

// a profile as --new-profile starts it: the seven originals, their home tracks, Novice and Amateur
argon::GameProgress newProfileProgress()
{
    argon::GameProgress progress;
    progress.characters = 0x007f;
    progress.tracks = 0x7f;
    progress.mars = false;
    progress.speeds = 1;
    return progress;
}

void testTournamentWinGivesItsTrophyOnce()
{
    argon::GameProgress progress = newProfileProgress();
    const TourneyAwards first = argon::tourney::winTournament(progress, 0, 2);
    const TourneyAwards again = argon::tourney::winTournament(progress, 0, 2);
    CHECK(first.trophy == 0 && progress.trophies[2][0] && again.trophy == -1);
}

void testAmateurWinUnlocksTheNemesisAndTheOtherRouteAndOpensExpert()
{
    argon::GameProgress progress = newProfileProgress();
    const TourneyAwards awards = argon::tourney::winTournament(progress, 1, 3);
    CHECK(awards.character == 11 && awards.track == 11 && awards.level == 2 && ((progress.characters >> 11) & 1) && progress.speeds == 2);
}

void testNemesisWinningExpertUnlocksTheNextEasterEgg()
{
    argon::GameProgress progress = newProfileProgress();
    progress.eggs[0] = true;
    const TourneyAwards awards = argon::tourney::winTournament(progress, 2, 9);
    CHECK(awards.easterEgg == 1 && progress.eggs[1] && !progress.eggsOn[1]);
}

void testMasterWinGivesDracoAndItsOwnerOnceTheOriginalsHoldMaster()
{
    argon::GameProgress progress = newProfileProgress();
    for (int character = 0; character < 6; ++character)
    {
        progress.trophies[character][3] = true;
    }
    const TourneyAwards awards = argon::tourney::winTournament(progress, 3, 6);
    CHECK(awards.draco && awards.kaHekili && ((progress.characters >> 7) & 1));
}

void testRedshiftOpensOnceEveryOpenCharacterHoldsMaster()
{
    argon::GameProgress progress = newProfileProgress();
    progress.characters = 0x0101;   // Tara and her nemesis, who already holds Master
    progress.trophies[8][3] = true;
    const TourneyAwards awards = argon::tourney::winTournament(progress, 3, 0);
    CHECK(awards.level == 4 && progress.speeds == 4);
}

void testRedshiftWinGivesMarsOnceEveryOpenCharacterHoldsIt()
{
    argon::GameProgress progress = newProfileProgress();
    progress.characters = 0x0101;   // Tara and her nemesis, who already holds Redshift
    progress.trophies[8][4] = true;
    const TourneyAwards awards = argon::tourney::winTournament(progress, 4, 0);
    CHECK(awards.mars && progress.mars);
}

void testUpgradePriceRisesBySeventyFiveThousand()
{
    CHECK(argon::upgrades::priceOfLevel(0) == 50000 && argon::upgrades::priceOfLevel(6) == 500000);
}

void testLapTurboGrowsHalfASecondALevel()
{
    CHECK(argon::upgrades::lapTurboSeconds(0) == 1.5f && argon::upgrades::lapTurboSeconds(7) == 5.f);
}

void testWeaponStrengthRaisesBlueFirst()
{
    const argon::weapons::PowerLevels caps = argon::upgrades::pickupCaps(1);
    CHECK(caps.red == 1 && caps.yellow == 1 && caps.blue == 2 && caps.special == 0);
}

void testOnlyTheLastWeaponStrengthAllowsASpecial()
{
    CHECK(argon::upgrades::pickupCaps(6).special == 0 && argon::upgrades::pickupCaps(7).special == 1);
}

void testWeaponStrengthOutsideTheLevelsIsKeptToThem()
{
    CHECK(argon::upgrades::pickupCaps(-2).red == 1 && argon::upgrades::pickupCaps(12).special == 1);
}

void testUpgradeIconPulseSwingsNinetySixAroundHalf()
{
    CHECK(menu::upgradeIconPulse(0) == 128 && menu::upgradeIconPulse(9) == 224 && menu::upgradeIconPulse(27) == 32);
}

// ---- the front end's UI sounds per page (ui\fe_sounds.hpp)
namespace
{
using menu::UiEvent;
using menu::UiSound;
namespace sounds = menu::fe_sounds;

bool plays(const menu::PageSounds &page, UiEvent event, UiSound sound)
{
    return page.soundFor(event) == sound;
}

bool isSilent(const menu::PageSounds &page, UiEvent event)
{
    return !page.soundFor(event).has_value();
}

void testUiSoundsAreFrontendTxtIds()
{
    static_assert(int(UiSound::Select) == 0 && int(UiSound::BadSelect) == 1 && int(UiSound::Rollover) == 2);
    static_assert(int(UiSound::ScreenForward) == 3 && int(UiSound::ScreenBack) == 4);
}

void testSoundForReadsEachEventsOwnSound()
{
    const menu::PageSounds page{.cursor = UiSound::Rollover,
                                .pick = UiSound::Select,
                                .refused = UiSound::BadSelect,
                                .back = UiSound::ScreenBack,
                                .leftForward = UiSound::ScreenForward};
    CHECK(plays(page, UiEvent::Cursor, UiSound::Rollover));
    CHECK(plays(page, UiEvent::Pick, UiSound::Select));
    CHECK(plays(page, UiEvent::Refused, UiSound::BadSelect));
    CHECK(plays(page, UiEvent::Back, UiSound::ScreenBack));
    CHECK(plays(page, UiEvent::LeftForward, UiSound::ScreenForward));
    CHECK(isSilent(page, UiEvent::Cancel) && isSilent(page, UiEvent::Forward) && isSilent(page, UiEvent::LeftBack));
}

void testSilentPagesPlayNothing()
{
    for (UiEvent e :
         {UiEvent::Cursor, UiEvent::Pick, UiEvent::Refused, UiEvent::Cancel, UiEvent::Back, UiEvent::Forward, UiEvent::LeftBack, UiEvent::LeftForward})
    {
        CHECK(isSilent(sounds::kSilent, e));
    }
}

void testMainMenuSoundsItsLeavesAtTheSwitch()
{
    CHECK(isSilent(sounds::kMain, UiEvent::Forward) && isSilent(sounds::kMain, UiEvent::Back));
    CHECK(plays(sounds::kMain, UiEvent::LeftForward, UiSound::ScreenForward));
    CHECK(plays(sounds::kMain, UiEvent::LeftBack, UiSound::ScreenBack));
}

void testCharacterSelectSoundsItsLeavesAtThePress()
{
    CHECK(plays(sounds::kCharSelect, UiEvent::Forward, UiSound::ScreenForward));
    CHECK(plays(sounds::kCharSelect, UiEvent::Back, UiSound::ScreenBack));
    CHECK(isSilent(sounds::kCharSelect, UiEvent::LeftForward) && isSilent(sounds::kCharSelect, UiEvent::LeftBack));
}

void testUndoingAPickIsABadSelect()
{
    CHECK(plays(sounds::kCharSelect, UiEvent::Cancel, UiSound::BadSelect));
    CHECK(plays(sounds::kCourseSelect, UiEvent::Cancel, UiSound::BadSelect));
}

void testRecordTablesGoBackWithSelectThenScreenBack()
{
    CHECK(plays(sounds::kRecordTable, UiEvent::Back, UiSound::Select));
    CHECK(plays(sounds::kRecordTable, UiEvent::LeftBack, UiSound::ScreenBack));
}

void testSpeedSelectLeavesSilentlyAfterItsPick()
{
    CHECK(plays(sounds::kSpeedSelect, UiEvent::Pick, UiSound::Select));
    CHECK(isSilent(sounds::kSpeedSelect, UiEvent::LeftForward));
}

void testInfoPagesScrollSilently()
{
    CHECK(isSilent(sounds::kInfo, UiEvent::Cursor));
    CHECK(plays(sounds::kInfo, UiEvent::Back, UiSound::ScreenBack));
}

void testLevelMusicSoundsOnlyItsLeave()
{
    CHECK(isSilent(sounds::kLevelMusic, UiEvent::Cursor) && isSilent(sounds::kLevelMusic, UiEvent::Pick));
    CHECK(plays(sounds::kLevelMusic, UiEvent::LeftBack, UiSound::ScreenBack));
}

void testTourneyTypeStartsSilently()
{
    CHECK(isSilent(sounds::kTourneyType, UiEvent::Pick) && isSilent(sounds::kTourneyType, UiEvent::LeftForward));
    CHECK(plays(sounds::kTourneyType, UiEvent::Refused, UiSound::BadSelect));
}

void testTitleAndAwardsLeaveForward()
{
    CHECK(plays(sounds::kTitle, UiEvent::LeftForward, UiSound::ScreenForward));
    CHECK(plays(sounds::kAwards, UiEvent::LeftForward, UiSound::ScreenForward));
}

void testTitlePressStartIsOrange()
{
    const menu::fe::TitleText &text = menu::fe::kPressStart;

    CHECK(text.colour == 0xffff8000u);
}

void testTitleCreditsKeepTheDefaultColour()
{
    const uint32_t colours[2] = {menu::fe::kDevelopedBy.colour, menu::fe::kCopyright.colour};

    CHECK(colours[0] == menu::kText && colours[1] == menu::kText);
}

void testMessageBoxRollsOverAndSelects()
{
    CHECK(plays(sounds::kMessageBox, UiEvent::Cursor, UiSound::Rollover));
    CHECK(plays(sounds::kMessageBox, UiEvent::Pick, UiSound::Select));
    CHECK(isSilent(sounds::kMessageBox, UiEvent::Back));
}
// the Square721 fonts draw U+0020..U+0177 (original::FontMap: first char 32, 344 chars)
bool drawnByTheLatinFonts(const char16_t *text)
{
    for (const char16_t *c = text; *c; ++c)
    {
        if (*c < u' ' || *c > u'ŷ')
        {
            return false;
        }
    }
    return true;
}

// game_data\fonts\MSPGoth.abc's characters; none without the game's files
std::set<char16_t> japaneseFontCharacters()
{
    std::ifstream in(QR_SOURCE_DIR "/game_data/fonts/MSPGoth.abc", std::ios::binary);
    const std::vector<char> file{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    std::set<char16_t> characters;
    if (file.size() < sizeof(argon::original::AbcFontHeader))
    {
        return characters;
    }
    argon::original::AbcFontHeader header{};
    std::memcpy(&header, file.data(), sizeof(header));
    for (uint32_t i = 0; i < header.glyphCount && sizeof(header) + (i + 1) * sizeof(argon::original::AbcGlyph) <= file.size(); ++i)
    {
        argon::original::AbcGlyph glyph{};
        std::memcpy(&glyph, file.data() + sizeof(header) + i * sizeof(glyph), sizeof(glyph));
        characters.insert(char16_t(glyph.code));
    }
    return characters;
}

void testViewerTextsHaveEveryLanguage()
{
    bool complete = true;
    for (const ViewerText *text : viewer_text::all())
    {
        complete = complete && text->english && text->french && text->german && text->italian && text->japanese;
    }
    CHECK(complete);
}

void testViewerTextsInEuropeanLanguagesUseOnlyTheLatinFontsCharacters()
{
    bool drawn = true;
    for (const ViewerText *text : viewer_text::all())
    {
        for (const char16_t *translation : {text->english, text->french, text->german, text->italian})
        {
            drawn = drawn && drawnByTheLatinFonts(translation);
        }
    }
    CHECK(drawn);
}

void testViewerTextsInJapaneseUseOnlyMspGothsCharacters()
{
    const std::set<char16_t> characters = japaneseFontCharacters();
    if (characters.empty())
    {
        return;
    }
    bool drawn = true;
    for (const ViewerText *text : viewer_text::all())
    {
        for (const char16_t *c = text->japanese; *c; ++c)
        {
            drawn = drawn && characters.count(*c) == 1;
        }
    }
    CHECK(drawn);
}

void testViewerTextPicksTheLanguage()
{
    CHECK(viewer_text::kArgonOptions.in(Language::German) == u"Argon-Optionen");
}

void testViewerTextPicksJapanese()
{
    CHECK(viewer_text::kArgonOptions.in(Language::Japanese) == u"Argon オプション");
}

void testLanguageNameIsInTheMenusLanguage()
{
    CHECK(viewer_text::kLanguageNames[int(Language::French)].in(Language::Japanese) == u"フランス語");
}

void testViewerTextWithoutTheLanguageShowsEnglish()
{
    const ViewerText englishOnly{u"Exit"};
    CHECK(englishOnly.in(Language::French) == u"Exit");
}

}  // namespace

int main()
{
    testTrackDataQuadIsUprightAcrossTheScreen();
    testRotatedQuadTexturesTheTopRightCornerWithURight();
    testIdCardQuadTurnsAboutItsPivot();
    testIntroShowsForSixSeconds();
    testIntroStartsWithTwoSweepsAndSixteenCopies();
    testIntroSweepsCrossTheScreen();
    testIntroCopiesStartTurnedAndLarge();
    testIntroCopiesSettleByFourSeconds();
    testIntroEchoFadesOutAfterThreeSeconds();
    testYesNoOpensOnNo();
    testNoFreeBlocksHasThreeButtons();
    testThreeButtonsGoRound();
    testKindSevenTakesNoInput();
    testTimedBoxHasNoButton();
    testTrophyComesFirstThenTheRaces();
    testFinishedTournamentShowsOnlyItsTrophy();
    testNemesisRaces();
    testNemesisRaceSeatsOnlyTheNemesis();
    testOtherTournamentRacesKeepTheChosenDrones();
    testSpeedoNeedle();
    testNewProfileIsAtZeroPercent();
    testTrophyLevelIsTheBestLevelPlusOne();
    testEverythingIsAHundredPercent();
    testRedshiftWinWithTheEggCountsTwice();
    testKaHekiliDoesNotCount();
    testDownFromTheRightOfTheThirdRowLandsOnSpace();
    testDownFromALetterLandsOnDoneAndBackUp();
    testDownFromDoneGoesRoundToTheRememberedColumn();
    testUpFromTheTopRowGoesRoundOntoDone();
    testUpFromSpaceReturnsToTheRememberedColumn();
    testUpFromDeleteSkipsTheEmptyPlaces();
    testGermanSkipsTheAccentsKey();
    testLeftAndRightGoRound();
    testAnyModeSetsTheRecordUnderTheTable();
    testTimeAttackNeedsTheProfileBestFirst();
    testTimeAttackProfileBestWithoutRecord();
    testASlowerLapBeatsNothing();
    testTrackRecordsOpenOnTheSecondRoutes();
    testFlashFallsAndWraps();
    testAwardsHideTheTickerWithoutAnEgg();
    testAwardsShowTheTickerWithAnEgg();
    testPausePickIsSilentOnAVolumeRow();
    testPauseVolumeSoundsOnlyWhenItMoves();
    testPauseHudRowStopsAtItsLastOption();
    testSavingRaceLastsOneSecond();
    testPullingAPadOutPausesBehindItsMessage();
    testAKeyboardOnlyPlayerIsNeverAsked();
    testReconnectingWaitsForStart();
    testStartOnTheReconnectedPadOpensThePauseMenu();
    testStartOnAnotherPadDoesNotClearTheWait();
    testUpgradeIconsStartWithTheShadowedBase();
    testLevelZeroShowsTheFirstCapsAndPulsesBlueTwo();
    testLevelThreePulsesBlueThree();
    testLevelFivePulsesRedThree();
    testLevelSixFadesTheSpecialInOverAllCaps();
    testLevelSevenShowsOnlyTheBaseAndTheSpecial();
    testUpgradeIconPulseSwingsNinetySixAroundHalf();
    testUiSoundsAreFrontendTxtIds();
    testSoundForReadsEachEventsOwnSound();
    testSilentPagesPlayNothing();
    testMainMenuSoundsItsLeavesAtTheSwitch();
    testCharacterSelectSoundsItsLeavesAtThePress();
    testUndoingAPickIsABadSelect();
    testRecordTablesGoBackWithSelectThenScreenBack();
    testSpeedSelectLeavesSilentlyAfterItsPick();
    testInfoPagesScrollSilently();
    testLevelMusicSoundsOnlyItsLeave();
    testTourneyTypeStartsSilently();
    testTitleAndAwardsLeaveForward();
    testTitlePressStartIsOrange();
    testTitleCreditsKeepTheDefaultColour();
    testMessageBoxRollsOverAndSelects();
    testViewerTextsHaveEveryLanguage();
    testViewerTextsInEuropeanLanguagesUseOnlyTheLatinFontsCharacters();
    testViewerTextsInJapaneseUseOnlyMspGothsCharacters();
    testViewerTextPicksTheLanguage();
    testViewerTextPicksJapanese();
    testLanguageNameIsInTheMenusLanguage();
    testViewerTextWithoutTheLanguageShowsEnglish();
    testTournamentWinGivesItsTrophyOnce();
    testAmateurWinUnlocksTheNemesisAndTheOtherRouteAndOpensExpert();
    testNemesisWinningExpertUnlocksTheNextEasterEgg();
    testMasterWinGivesDracoAndItsOwnerOnceTheOriginalsHoldMaster();
    testRedshiftOpensOnceEveryOpenCharacterHoldsMaster();
    testRedshiftWinGivesMarsOnceEveryOpenCharacterHoldsIt();
    testTournamentsRunTwoToNineRaces();
    testTournamentStartsOnTheHomeTrack();
    testNemesisRaceIsOnTheHomeTracksOtherRoute();
    testMasterFourthRaceIsDracoUnlessDracoIsHome();
    testRedshiftStartsAndEndsOnTheHomeTrack();
    testExpertTakesTheTableFromItsThirdTrack();
    testUpgradePriceRisesBySeventyFiveThousand();
    testLapTurboGrowsHalfASecondALevel();
    testWeaponStrengthRaisesBlueFirst();
    testOnlyTheLastWeaponStrengthAllowsASpecial();
    testWeaponStrengthOutsideTheLevelsIsKeptToThem();
    std::printf("frontend_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
