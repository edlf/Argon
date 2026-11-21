// Argon: where a test run starts in the front end (launch\test_start.hpp).
#include "launch/test_start.hpp"

#include <algorithm>
#include <iterator>

#include "launch/test_runs.hpp"
#include "logic/game_logic.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/race.hpp"
#include "logic/race_start.hpp"
#include "logic/tourney_rules.hpp"
#include "logic/tourney_win.hpp"
#include "saves/viewer_settings.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/page_id.hpp"
#include "ui/pause_rules.hpp"
#include "ui/race_end_page.hpp"
#include "ui/race_menus.hpp"

void TestStart::open()
{
    host_.openMenu();
    const int page = menu_.page;
    if (page)
    {
        openMenuPage(page);
    }
    else if (!menu_.skipTitle && race_.quickRaceTrack < 0 && race_.replay < 0)
    {
        openFirstPage();
    }
    if (menu_.routeChoice)
    {
        sources_.fe.courseSelect().openRouteChoice();
    }
    openMessageBox();
    startTestRace();
}

// FE INTROSCREENS' videos, then FE TITLE; --no-intro, settings.json's skipIntroVideos and screenshot runs start on the
// title
void TestStart::openFirstPage()
{
    if (menu_.skipIntro || sources_.settings.skipIntroVideos() || host_.screenshotRun())
    {
        sources_.fe.title().open();
    }
    else
    {
        sources_.fe.introScreens().open();
    }
}

// --menu-page: the page as reached through the menus (the pages it is opened from first)
// --trophy (FE CHARSELECT's Trophy Level, FE TOURNEYTYPE) and --tourney-done (FE TOURNEYTYPE, at --speed's level)
void TestStart::giveTestTourneyProgress()
{
    const RaceTestStart &raceTest = race_;
    const int level = std::clamp(sources_.speedClass, 0, 4);
    sources_.progress.tourneyLevel = level;
    if (raceTest.tourneyRacesWon >= 0)
    {
        sources_.progress.tourney[sources_.character][level] = raceTest.tourneyRacesWon;
    }
    for (int l = 0; l < 5; ++l)
    {
        sources_.progress.trophies[sources_.character][l] = sources_.progress.trophies[sources_.character][l] || ((raceTest.trophies >> l) & 1u);
    }
}

void TestStart::openMenuPage(int page)
{
    FeShell &fe = sources_.fe;
    if (page == menu::PageChooseSave)
    {
        fe.chooseSave().open();
        return;
    }
    if (page == menu::PageNameEntry)
    {
        fe.nameEntry().openForName();
        return;
    }
    giveTestTourneyProgress();
    if (page == menu::PageTourneyType)
    {   // --speed: the cursor's level; --tourney-done / --trophy: --char's progress there and trophies
        sources_.modes.select(sources_.modes.tournament());
        fe.goTo(menu::PageCharSelect);
        fe.tourneyType().open();
        return;
    }
    if (page == menu::PageAwards)
    {   // as after winning the --speed level's tournament with --char (--new-profile: unlocks)
        sources_.tourneyRun.awards = argon::tourney::winTournament(sources_.progress, std::clamp(sources_.speedClass, 0, 4), sources_.character);
        sources_.tourneyRun.awardsPending = true;
        fe.goTo(menu::PageMain);
        return;
    }
    if (page == menu::PageSpeedSelect)
    {
        fe.goTo(menu::PageCourseSelect);
        fe.speedSelect().open(sources_.character, sources_.course);
        return;
    }
    if (page == menu::PageTrackInfo)
    {
        fe.goTo(menu::PageCourseSelect);
        fe.trackInfo().open(sources_.course);
        return;
    }
    if (page == menu::PageGhostSelect)
    {
        sources_.modes.select(sources_.modes.timeAttack());
        fe.goTo(menu::PageCourseSelect);
        fe.ghostSelect().open(sources_.character, sources_.course);
        return;
    }
    if (!openOptionsSubpage(page))
    {
        fe.goTo(page);
    }
}

// Argon Options (FE MAIN's row) and its pages
void TestStart::openArgonOptionsPage(int page)
{
    FeShell &fe = sources_.fe;
    fe.mainMenu().selectKind(MainMenuPage::kArgonSettings);
    host_.openMenu();
    fe.argonOptions().open();
    if (page == menu::PageKeyBindings)
    {
        fe.keyBindings().open();
    }
    else if (page != menu::PageArgonSettings)
    {
        fe.settings(page).open();
    }
}

// the pages under FE OPTIONS (and FE OPTIONS SOUND)
bool TestStart::openOptionsSubpage(int page)
{
    FeShell &fe = sources_.fe;
    if (page == menu::PageOptions)
    {
        fe.options().open();
    }
    else if (page == menu::PageGameSettings || page == menu::PageSoundSettings)
    {
        fe.options().open();
        fe.settings(page).open();
    }
    else if (page >= menu::PageArgonSettings && page <= menu::PageKeyBindings)
    {
        openArgonOptionsPage(page);
    }
    else if (page == menu::PageCredits)
    {
        fe.options().open();
        fe.credits().open();
    }
    else if (page == menu::PageMusicSelect)
    {
        fe.options().open();
        fe.soundSettings().open();
        fe.musicSelect().openAt(0);
    }
    else if (page == menu::PageLevelMusic)
    {
        fe.options().open();
        fe.soundSettings().open();
        fe.levelMusic().open();
    }
    else if (page == menu::PageTrackRecords)
    {
        fe.options().open();
        fe.trackRecords().open();
    }
    else if (page == menu::PageHighScores)
    {
        fe.options().open();
        fe.highScores().open();
    }
    else if (page == menu::PageGhosts)
    {
        fe.options().open();
        fe.ghostData().open();
    }
    else if (page == menu::PageEggs)
    {
        fe.options().open();
        fe.easterEggs().open();
    }
    else if (page == menu::PageCheats)
    {
        fe.options().open();
        fe.cheatMenu().open();
    }
    else
    {
        return false;
    }
    return true;
}

// --message n: 1 info, 2 yes / no, 3 the controller message, 4 Saving, 5 timed (3 s), 6 / 7 / 8 the no-free-blocks
// boxes (kinds 8, 9, 7)
void TestStart::openMessageBox()
{
    struct TestBox
    {
        int feString;
        const char *fallback;
        MessageBoxPage::Kind kind;
    };
    static const TestBox boxes[] = {
        {0x2da, "You must enter a name.", MessageBoxPage::Kind::Info},
        {0x341, "Are you sure you want to reset all options to their default setting?", MessageBoxPage::Kind::YesNo},
        {7, "Please reconnect the controller and press START to continue.", MessageBoxPage::Kind::Controller},
        {-1, "", MessageBoxPage::Kind::Saving},
        {0x2da, "You must enter a name.", MessageBoxPage::Kind::Timed},
        {0x2d9, "There is not enough space to save.", MessageBoxPage::Kind::NoFreeBlocksProfile},
        {0x2d9, "There is not enough space to save.", MessageBoxPage::Kind::NoFreeBlocksGhost},
        {0x2d9, "There is not enough space to save.", MessageBoxPage::Kind::NoFreeBlocks},
    };
    const int boxNumber = menu_.messageBox;
    if (boxNumber < 1 || boxNumber > int(std::size(boxes)))
    {
        return;
    }
    const TestBox &box = boxes[boxNumber - 1];
    const std::u16string text = box.feString < 0 ? std::u16string() : sources_.strings.frontEnd(box.feString, box.fallback);
    if (box.kind == MessageBoxPage::Kind::Timed)
    {
        sources_.fe.messageBox().openTimed(text, 3.f);
        return;
    }
    sources_.fe.messageBox().open(text, box.kind);
}

// --tourney-race: the tournament's next race for --char at that level (--tourney-done: after that many won);
// --quick-race: that track; --loader-at: the loading screen's clock from there
void TestStart::startTestRace()
{
    RaceTestStart &raceTest = race_;
    if (raceTest.tourneyLevel >= 0)
    {
        if (raceTest.tourneyRacesWon >= 0)
        {
            sources_.progress.tourney[sources_.character][raceTest.tourneyLevel] = raceTest.tourneyRacesWon;
        }
        sources_.modes.select(sources_.modes.tournament());
        sources_.progress.tourneyLevel = raceTest.tourneyLevel;
        sources_.tourneyRun.character = sources_.character;
        sources_.speedClass = raceTest.tourneyLevel;
        raceTest.quickRaceTrack = sources_.progressKeeper.nextTourneyTrack(raceTest.tourneyLevel, sources_.character);
    }
    if (raceTest.replay > 0)
    {
        if (!host_.startAttractDemo(raceTest.replay))
        {
            return;
        }
    }
    else if (raceTest.quickRaceTrack >= 0)
    {
        host_.startQuickRace(sources_.character, raceTest.quickRaceTrack);
    }
    else
    {
        return;
    }
    if (raceTest.loaderAt >= 0)
    {
        sources_.fe.loader().showClockAt(raceTest.loaderAt);
    }
    if (raceTest.storyAt >= 0)
    {
        sources_.tests.storySceneAt = raceTest.storyAt;
    }
    if (!raceTest.continuePastLoader)
    {
        return;
    }
    host_.loadRaceNow();
    host_.finishQuickRace();
    setUpTestRace();
    if (raceTest.raceEndScene)
    {
        sources_.progress.tourney[sources_.tourneyRun.character][sources_.progress.tourneyLevel] = -1;
        host_.quitRaceTo(menu::PageMain);
    }
}

void TestStart::setUpTestRace()
{
    const RaceTestStart &raceTest = race_;
    if (raceTest.aiDrivesPlayer)
    {
        sources_.playerDrive = false;
    }
    sources_.race.routesOn = !raceTest.firstLinksOnly;
    if (raceTest.introAt >= 0)
    {
        sources_.raceStart.showIntroAt(raceTest.introAt);
    }
    if (raceTest.countdownAt >= 0)
    {
        sources_.raceStart.showCountdownAt(raceTest.countdownAt);
    }
    sources_.testRuns.applyTurboTest();
    if (raceTest.finishPage >= 0)
    {
        sources_.raceEnd = raceTest.finishPage == 0 ? race_end::kPostTable : raceTest.finishPage;
        if (raceTest.finishPage == race_end::kScoreMenu || raceTest.finishPage == race_end::kPointsMenu)
        {
            host_.computeScore();
        }
        sources_.playerDrive = false;
        if (raceTest.cash >= 0)
        {
            sources_.progress.cash[host_.upgradeChar()] = raceTest.cash;
        }
        if (raceTest.weaponLevel >= 0)
        {
            sources_.progress.upgrades[host_.upgradeChar()][0] = uint8_t(raceTest.weaponLevel);
        }
        if (raceTest.upgradeMenu)
        {
            sources_.raceMenus.openUpgrade();
        }
    }
    if (raceTest.pausePage >= 0)
    {
        sources_.simStarted = true;
        host_.pauseRace();
        if (raceTest.pausePage == 1)
        {
            sources_.raceMenus.pause().showOptions();
        }
        if (raceTest.pausePage == 2)
        {
            sources_.raceMenus.pause().askToConfirm(PauseMenu::kRestart);
        }
        showPauseTest(raceTest.pausePage);
    }
}

// --pause 3: "Saving Race" (as after a volume change, then Continue); 4: player 1's controller pulled out (the
// message in the HUD, the race held until the pad is back and START pressed)
void TestStart::showPauseTest(int page)
{
    if (page == 3)
    {
        sources_.raceMenus.pause().startSaving();
    }
    if (page == 4)
    {
        menu::PadWatch::PerPlayer connected{};
        connected.fill(true);
        sources_.padWatch.update(connected, {});
        sources_.padWatch.update({}, {});
    }
}
