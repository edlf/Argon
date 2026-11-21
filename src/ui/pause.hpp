// Argon: the in-race pause menu PAUSEMENU (vtable 0x19c5e0) (ui\pause.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <string>
#include <vector>

#include "ui/menu_list.hpp"
#include "ui/pause_rules.hpp"
#include "ui/race_menu.hpp"

class GameModes;
class PauseBackground;
class SettingsMenu;
class SettingsStore;

// what the pause menu reads and changes
struct PauseSources
{
    GameModes &modes;                     // Time Attack's ghosts, the tournament's items
    const SettingsMenu &settingsMenu;     // the volume, HUD and vibration rows
    SettingsStore &settingsStore;         // a row changed: applied and written at once
    PauseBackground &background;          // the race frame blurred under it
};

class PauseMenu : public RaceMenu
{
  public:
    // the items (race_strings): the main page, the Options page, the confirm page
    enum Item
    {
        kContinue = 0,
        kReplayLoadedGhost = 1,
        kReplaySessionGhost = 2,
        kTrackSelect = 3,
        kMainMenu = 4,
        kOptions = 5,
        kExit = 6,
        kSfxVolume = 7,
        kMusicVolume = 8,
        kEngineVolume = 9,
        kRestart = 10,
        kYes = 11,
        kNo = 12,
        kHud = 13,         // the viewer's: the Game Settings' HUD entry (Off / Map Off / Full)
        kVibration = 14,   // the viewer's: player 1's Controller n Vibration, single player only
        kHudSize = 15      // the viewer's: the race HUD's size, 25 / 50 / 75 / 100 % (settings.json)
    };

    PauseMenu(const RaceMenuKit &kit, PauseHost &pauseHost, const PauseSources &sources) : RaceMenu(kit), pauseHost(pauseHost), sources(sources)
    {
    }
    void reset();                       // "PAUSE ON": the main page, the cursor on Continue
    void showOptions();                 // 0x8b6c0: the cursor on Exit
    void askToConfirm(Item action);     // Restart / Track Select / Main Menu: "Are you sure?", the cursor on No
    void startSaving();                 // "Saving Settings" as after a volume change, then Continue (tests: --pause 3)
    bool isSaving() const
    {
        return saving.active();
    }
    void stepSaving(double now);        // the second over: the item that started it runs

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::Own;
    }

  private:
    enum class Page
    {
        Main,
        Options,
        Confirm
    };
    std::vector<Item> items() const;
    std::vector<Item> mainItems() const;
    std::u16string label(Item item) const;
    std::vector<Item> optionItems() const;
    std::u16string settingLabel(Item item) const;
    std::u16string settingName(Item item) const;
    int settingType(Item item) const;
    static bool isSettingRow(Item item);
    void play(menu::PauseSound sound) const;
    void takePresses(const menu::MenuInput &in);
    menu::ListLayout listLayout() const;
    void followCursor();   // the window over the page's items keeps the selected one shown
    void back();
    void moveCursor(const menu::MenuInput &in);
    void changeSetting(const menu::MenuInput &in);
    void runItem();
    void leaveBy(Item item);
    void finishLeaving(Item item);
    void confirmed();
    void replayGhost();
    void leaveRace();
    std::u16string hint() const;

    PauseHost &pauseHost;
    PauseSources sources;
    Page page = Page::Main;
    Item selected = kContinue;
    Item confirmAction = kRestart;
    bool settingChanged = false;   // 0x584339: a volume (here also the HUD, vibration) stepped since the profile was last written
    menu::SavingRace saving;      // +0x1d4 / +0x618
    Item leavingBy = kContinue;   // what runs once "Saving Settings" is over
    menu::ScrollWindow window;
};
