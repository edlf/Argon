// Argon: UPGRADEMENU (upgrademenu.cpp), from the tournament's WINMENU / LOSEMENU (ui\upgrade.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <cstdint>

#include "ui/race_menu.hpp"
#include "ui/upgrade_icons.hpp"

class PauseBackground;
class ProgressStore;
class ViewerSettings;
class Weapons;
namespace argon
{
struct GameProgress;
}

// what UPGRADEMENU reads and changes
struct UpgradeSources
{
    RaceEndPages &pages;                     // back to WINMENU / LOSEMENU
    argon::GameProgress &progress;           // the characters' cash and upgrade levels
    ProgressStore &progressStore;            // each purchase written to the profile
    const ViewerSettings &viewerSettings;    // the language
    const Weapons &weapons;                  // the shield sizes
    PauseBackground &background;             // the race frame blurred under it
};

class UpgradeMenu : public RaceMenu
{
  public:
    UpgradeMenu(const RaceMenuKit &kit, UpgradeHost &upgradeHost, const UpgradeSources &sources) : RaceMenu(kit), upgradeHost(upgradeHost), sources(sources)
    {
    }
    void open();   // "UPGRADEMENU ON": the cursor on Continue; the levels and cash kept for Clear Changes

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::Own;
    }

  private:
    enum Row
    {
        kWeapon,
        kTurbo,
        kShield,
        kContinue,
        kClearChanges,
        kRowCount
    };
    void leave() override;
    int level(int upgrade) const;
    bool affordable(int upgrade) const;
    void buy(int upgrade);
    void clearChanges();
    void drawRows(const FeCanvas &canvas, float rowRight[3]) const;
    void drawUpgradePanel(const FeCanvas &canvas, int upgrade, float rowRight) const;
    void drawInsufficientFunds(const FeCanvas &canvas, float x, int upgrade) const;
    void drawCost(const FeCanvas &canvas, float textX, int upgrade) const;
    void drawJoinLine(const FeCanvas &canvas, int upgrade, float rowRight) const;
    void drawWeaponPanel(const FeCanvas &canvas) const;
    void drawWeaponIcons(const FeCanvas &canvas, int weaponLevel) const;
    void setIconBlend(ImDrawList *drawList, menu::IconBlend blend) const;
    void drawIconLayer(const FeCanvas &canvas, const menu::UpgradeIconLayer &layer) const;
    void drawStatPanel(const FeCanvas &canvas, int upgrade) const;

    UpgradeHost &upgradeHost;
    UpgradeSources sources;
    int selected = kContinue;
    int snapshotCash = 0;
    uint8_t snapshotLevels[3]{};
};
