// Argon: what the race's menus (ui\race_menus.hpp) ask of the game around them, each a small interface App
// implements; the state they read and change comes in their own Sources (RaceMenusSources).
#pragma once

class FeCanvas;
namespace argon
{
class MusicFade;
}
namespace ghost
{
struct Lap;
}

// every race menu: the race music faded as a page leaves, the race again or left, the race end reset, the menu bars
class RaceMenuHost
{
  public:
    virtual void fadeRaceMusic(const argon::MusicFade &fade, double now) = 0;
    virtual void restartRace() = 0;           // the race again from the grid (Restart, Race Again)
    virtual void quitRaceTo(int page) = 0;    // out of the race to FE MAIN or FE COURSE SELECT
    virtual void resetRaceEnd() = 0;
    virtual void drawMenuBars(const FeCanvas &canvas, float vw) = 0;

  protected:
    ~RaceMenuHost() = default;
};

// PAUSEMENU (ui\pause.cpp)
class PauseHost
{
  public:
    virtual void resumeRace() = 0;
    virtual void startGhostReplay(int ghost) = 0;   // 0 the loaded ghost, 1 the session ghost
    virtual void openGhostSave(int nextPage, ghost::Lap lap) = 0;   // the front end on FE GHOSTSAVE, then nextPage

  protected:
    ~PauseHost() = default;
};

// POSTABLE and WINMENU / LOSEMENU (ui\results.cpp)
class RaceEndHost
{
  public:
    virtual void computeScore() = 0;
    virtual bool startViewReplay() = 0;
    virtual void openMenu() = 0;
    virtual void startQuickRace(int pilotId, int trackId) = 0;

  protected:
    ~RaceEndHost() = default;
};

// UPGRADEMENU (ui\upgrade.cpp)
class UpgradeHost
{
  public:
    virtual int upgradeChar() const = 0;   // the character whose upgrades are shown and bought
    virtual void applyUpgradeCaps() = 0;

  protected:
    ~UpgradeHost() = default;
};

// all of them, as RaceMenus hands them out
class RaceMenusHost : public RaceMenuHost, public PauseHost, public RaceEndHost, public UpgradeHost
{
  protected:
    ~RaceMenusHost() = default;
};
