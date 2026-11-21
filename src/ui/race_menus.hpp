// Argon: the race's menus (App::raceMenus): the pause menu, and the race end's pages picked by
// RaceSession::raceEnd (0x52afa8), UPGRADEMENU over WINMENU / LOSEMENU while it is open (ui\race_menus.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "ui/multiplayer_end.hpp"
#include "ui/pause.hpp"
#include "ui/race_menu.hpp"
#include "ui/results.hpp"
#include "ui/score.hpp"
#include "ui/upgrade.hpp"

class MultiplayerSession;
class ProgressKeeper;
class SettingsStore;
class ViewerSettings;
class Weapons;
struct RaceScoring;

// what the race's menus draw with, read and change (each menu gets its part)
struct RaceMenusSources
{
    const GameStrings &strings;
    MenuKit &menus;
    HudPainter &painter;
    argon::Audio &audio;
    int &raceEnd;                            // race_end::Page: the race end's page shown
    const bool &paused;
    PauseBackground &pauseBackground;        // the race frame blurred under them
    const Race &race;
    const argon::AiGraph &aiGraph;
    GameModes &modes;
    const SettingsMenu &settingsMenu;
    SettingsStore &settingsStore;            // the pause menu's rows changed
    RaceScoring &scoring;
    const int &hudLaps, &speedClass;
    argon::GameProgress &progress;
    ProgressKeeper &progressKeeper;
    const TourneyRun &tourneyRun;
    const argon::ProfileStore &profiles;
    const ViewerSettings &viewerSettings;
    const Weapons &weapons;
    const MultiplayerSession &multiplayer;
    const PlayerPads &pads;
};

class RaceMenus final : public RaceEndPages
{
  public:
    RaceMenus(RaceMenusHost &host, const RaceMenusSources &sources);
    RaceMenus(const RaceMenus &) = delete;
    RaceMenus &operator=(const RaceMenus &) = delete;

    PauseMenu &pause()
    {
        return pauseMenu;
    }
    RaceEndMenu &raceEnd()
    {
        return raceEndMenu;
    }
    UpgradeMenu &upgrade()
    {
        return upgradeMenu;
    }
    MultiplayerEndMenu &multiplayerEnd()
    {
        return multiplayerEndMenu;
    }
    void openUpgrade() override;
    void closeUpgrade() override;
    void showWinOrLose() override;
    void reset();          // a new race: no menu open

    RaceMenu &current();   // the race end's page shown (RaceSession::raceEnd not kRacing)
    void draw(const FeCanvas &canvas);
    bool holdsRaceFrame() const;   // a menu is drawn over the race frame (app_effects.cpp leaves the speed blur out)

  private:
    RaceMenuKit kit() const;
    ScoreSources scoreSources();
    void drawRaceBlur(const FeCanvas &canvas);

    RaceMenusHost &host;
    RaceMenusSources sources;
    PauseMenu pauseMenu;
    ResultsTable resultsTable;
    ScoreMenu scoreMenu;
    PointsMenu pointsMenu;
    RaceEndMenu raceEndMenu;
    UpgradeMenu upgradeMenu;
    MultiplayerEndMenu multiplayerEndMenu;
    bool upgrading = false;
};
