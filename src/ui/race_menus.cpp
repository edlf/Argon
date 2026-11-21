// Argon: the race's menus (ui\race_menus.hpp)
#include "ui/race_menus.hpp"

#include <stdexcept>

#include <GLFW/glfw3.h>

#include "logic/game_logic.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/race.hpp"
#include "render/pause_background.hpp"
#include "ui/game_strings.hpp"

RaceMenus::RaceMenus(RaceMenusHost &host, const RaceMenusSources &sources)
    : host(host), sources(sources), pauseMenu(kit(), host, {sources.modes, sources.settingsMenu, sources.settingsStore, sources.pauseBackground}),
      resultsTable(kit(), host, {sources.race, sources.aiGraph, sources.hudLaps, sources.raceEnd}), scoreMenu(kit(), scoreSources()),
      pointsMenu(kit(), scoreSources()),
      raceEndMenu(kit(), host, {*this, sources.modes, sources.raceEnd, sources.tourneyRun, sources.profiles, sources.progressKeeper, sources.progress}),
      upgradeMenu(kit(), host, {*this, sources.progress, sources.progressKeeper, sources.viewerSettings, sources.weapons, sources.pauseBackground}),
      multiplayerEndMenu(kit(), {sources.race, sources.multiplayer, sources.hudLaps})
{
}

RaceMenuKit RaceMenus::kit() const
{
    return RaceMenuKit{host, sources.strings, sources.menus, sources.painter, sources.audio, sources.pads};
}

ScoreSources RaceMenus::scoreSources()
{
    return ScoreSources{*this, sources.scoring, sources.modes, sources.speedClass, sources.raceEnd};
}

void RaceMenus::openUpgrade()
{
    upgrading = true;
    upgradeMenu.open();
}

void RaceMenus::closeUpgrade()
{
    upgrading = false;
    raceEndMenu.open();
}

// a second after A on SCOREMENU or POINTSMENU: WINMENU / LOSEMENU (0x95650), after the tournament progress (0x95d80)
void RaceMenus::showWinOrLose()
{
    const bool won = sources.race.playerWon();
    sources.raceEnd = won ? race_end::kWinMenu : race_end::kLoseMenu;
    sources.modes.current().raceFinished(won);
    raceEndMenu.open();
}

void RaceMenus::reset()
{
    upgrading = false;
    scoreMenu.stopLeaving();
    pointsMenu.stopLeaving();
    raceEndMenu.stopLeaving();
    upgradeMenu.stopLeaving();
}

RaceMenu &RaceMenus::current()
{
    switch (sources.raceEnd)
    {
    case race_end::kPostTable:
        return resultsTable;
    case race_end::kScoreMenu:
        return scoreMenu;
    case race_end::kPointsMenu:
        return pointsMenu;
    case race_end::kWinMenu:
    case race_end::kLoseMenu:
        return upgrading ? static_cast<RaceMenu &>(upgradeMenu) : raceEndMenu;
    case race_end::kMultiplayerEnd:
        return multiplayerEndMenu;
    default:
        throw std::logic_error("RaceMenus::current: no race-end menu while racing (raceEnd " + std::to_string(sources.raceEnd) + ")");
    }
}

void RaceMenus::draw(const FeCanvas &canvas)
{
    RaceMenu &menu = current();
    if (menu.background() == RaceMenu::Background::RaceBlur)
    {
        drawRaceBlur(canvas);
    }
    menu.draw(canvas);
}

// POSTABLE, SCOREMENU, POINTSMENU and WINMENU / LOSEMENU: pause_background (0x8e808, 0x84b24, 0xa0744, ...) every
// frame, over the echo the race scene copies each frame: the race runs on behind them, blurred, with the menu bars and
// logo; UPGRADEMENU makes its own from the current frame
void RaceMenus::drawRaceBlur(const FeCanvas &canvas)
{
    ImDrawList *drawList = canvas.drawList();
    const float originX = canvas.originX();
    sources.pauseBackground.blurAgain();
    sources.pauseBackground.draw(drawList);
    host.drawMenuBars(canvas, 640.f + 2.f * originX);
}

bool RaceMenus::holdsRaceFrame() const
{
    return sources.paused || sources.raceEnd != race_end::kRacing;
}

// ---- RaceMenu: the race-end pages' second of leaving (0x95408, 0x8d484, 0x9edb5, 0xa0c01, 0x85034)

void RaceMenu::startLeaving(argon::MusicFadeShape shape)
{
    leavingFade.start(shape, glfwGetTime());
}

void RaceMenu::stopLeaving()
{
    leavingFade.stop();
}

bool RaceMenu::stepLeaving()
{
    if (!leavingFade.active())
    {
        return false;
    }
    const double now = glfwGetTime();
    host.fadeRaceMusic(leavingFade, now);
    if (leavingFade.over(now))
    {
        leavingFade.stop();
        leave();
    }
    return true;
}

std::u16string RaceMenu::savingText() const
{
    return leaving() ? strings.race(0x520, "Saving Race") : std::u16string();
}
