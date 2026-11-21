// Argon: the front end's shell (CFEShell, vtable 0x1a2890, object 0x597e90): one page object per menu::Page,
// found by id (fe_shell_goto_page 0x31390 switches them by name), the shell's models and cameras (ui\fe_models.hpp,
// ui\fe_scene.hpp), its 60 Hz clock, the title and ticker; its update in ui\fe_shell.cpp, its draw in ui\menu.cpp.
// Built from FeShellSources and FePageSources (ui\fe_shell_sources.hpp) with an FeShellHost for the drawing App does.
#pragma once
#include "ui/fe_canvas.hpp"
#include <cstdint>
#include <vector>

#include "ui/argon_options.hpp"
#include "ui/awards.hpp"
#include "ui/charsel.hpp"
#include "ui/credits.hpp"
#include "ui/eggs.hpp"
#include "ui/fe_models.hpp"
#include "ui/fe_page.hpp"
#include "ui/fe_shell_host.hpp"
#include "ui/fe_shell_sources.hpp"
#include "logic/mode_pages.hpp"
#include "ui/page_id.hpp"
#include "ui/ghostsave.hpp"
#include "ui/ghostselect.hpp"
#include "ui/intro_screens.hpp"
#include "ui/loader.hpp"
#include "ui/mainmenu.hpp"
#include "ui/menu_kit.hpp"
#include "ui/messagebox.hpp"
#include "ui/options.hpp"
#include "ui/playlist.hpp"
#include "ui/profiles.hpp"
#include "ui/records.hpp"
#include "ui/shell_slide.hpp"
#include "ui/speed.hpp"
#include "ui/title.hpp"
#include "ui/tourney.hpp"

class FeShell : public ModePages
{
  public:
    FeShell(const FeShellSources &sources, const FePageSources &pages, FeShellHost &host);
    ~FeShell();
    FeShell(const FeShell &) = delete;
    FeShell &operator=(const FeShell &) = delete;

    // the shell's life: open on FE MAIN (main_menu 0x30af0), each frame's update and draw, closed for the race / editor
    MenuKit &menus()
    {
        return menus_;
    }
    void open();
    void close()
    {
        shown = false;
    }
    bool isOpen() const
    {
        return shown;
    }
    void update(float dt);
    // the page's models (its scene) drawn at this point of the draw list, and its boxed models over its frames
    void drawPageModels(ImDrawList *drawList)
    {
        host.drawPageModels(drawList);
    }
    void drawBoxModels(ImDrawList *drawList)
    {
        host.drawBoxModels(drawList);
    }
    void draw();
    FeScene scene()
    {
        return page(currentPage).scene();
    }   // the current page's models (the message box keeps them)
    std::vector<FeScene> boxScenes()
    {
        return page(currentPage).boxScenes();
    }

    // the pages: fe_shell_goto_page (the message box closed, the ticker from the right, the title and hints sliding in,
    // the page's enter). After slideOut the next goTo waits until the slide-out's 0.1 s are over; then the leaving
    // page plays its sound for atSwitch (UiEvent::LeftBack / LeftForward)
    void goTo(int id);
    void slideOut(menu::UiEvent atSwitch);
    bool isSlidingOut() const
    {
        return slideOutTicks >= 0;
    }
    float titleY(float titleWidth) const
    {
        return slide.titleY(titleWidth);
    }
    float hintOutset(float hintWidth) const
    {
        return slide.hintOutset(hintWidth);
    }
    int current() const
    {
        return currentPage;
    }
    FePage &page(int id) const;
    void restartTicker()
    {
        tickerX = menu::kRowFrom;
    }
    int frameCount() const
    {
        return steps;
    }   // the shell's 60 Hz clock
    FeModels &models()
    {
        return feModels;
    }

    static int connectedGamepads();
    MessageBoxPage &messageBox()
    {
        return messageBoxPage;
    }
    MainMenuPage &mainMenu()
    {
        return mainMenuPage;
    }
    GhostSelectPage &ghostSelect()
    {
        return ghostSelectPage;
    }
    GhostSavePage &ghostSave()
    {
        return ghostSavePage;
    }
    SpeedSelectPage &speedSelect()
    {
        return speedSelectPage;
    }
    TrackInfoPage &trackInfo()
    {
        return trackInfoPage;
    }
    TourneyTypePage &tourneyType()
    {
        return tourneyTypePage;
    }
    // the pages a game mode leads to (logic\mode_pages.hpp)
    void openCourseSelect() override
    {
        goTo(menu::PageCourseSelect);
    }
    void openTourneyType() override
    {
        tourneyTypePage.open();
    }
    void openGhostSelect(int pilot, int trackId) override
    {
        ghostSelectPage.open(pilot, trackId);
    }
    EasterEggsPage &easterEggs()
    {
        return easterEggsPage;
    }
    CheatMenuPage &cheatMenu()
    {
        return cheatMenuPage;
    }
    CreditsPage &credits()
    {
        return creditsPage;
    }
    TitlePage &title()
    {
        return titlePage;
    }
    IntroScreensPage &introScreens()
    {
        return introScreensPage;
    }
    OptionsPage &options()
    {
        return optionsPage;
    }
    SettingsPage &gameSettings()
    {
        return gameSettingsPage;
    }
    SettingsPage &soundSettings()
    {
        return soundSettingsPage;
    }
    ArgonOptionsPage &argonOptions()
    {
        return argonOptionsPage;
    }
    KeyBindingsPage &keyBindings()
    {
        return keyBindingsPage;
    }
    SettingsPage &settings(int page);   // menu::PageGameSettings / PageSoundSettings / PageArgonGeneral .. PageArgonFixes
    MusicSelectPage &musicSelect()
    {
        return musicSelectPage;
    }
    LevelMusicPage &levelMusic()
    {
        return levelMusicPage;
    }
    TrackRecordsPage &trackRecords()
    {
        return trackRecordsPage;
    }
    HighScoresPage &highScores()
    {
        return highScoresPage;
    }
    GhostDataPage &ghostData()
    {
        return ghostDataPage;
    }
    ChooseSavePage &chooseSave()
    {
        return chooseSavePage;
    }
    NameEntryPage &nameEntry()
    {
        return nameEntryPage;
    }
    LoaderPage &loader()
    {
        return loaderPage;
    }
    CourseSelectPage &courseSelect()
    {
        return courseSelectPage;
    }

  private:
    FePage &active();   // the message box while it is open, else the page
    void step(float dt);
    void stepSlideOut();
    void checkControllers();
    void checkPlayersControllers();
    bool padInUse(int pad) const;
    void show(int id);
    std::optional<std::u16string> tickerText() const
    {
        return page(currentPage).ticker();
    }
    void drawBackground(const FeCanvas &canvas);
    void drawModelFade(ImDrawList *drawList);
    void drawTitleAndTicker(const FeCanvas &canvas);
    void drawTicker(const FeCanvas &canvas, const std::u16string &text);
    PageKit pageKit();

    FeShellSources sources;
    FeShellHost &host;
    MenuKit menus_;
    FeModels feModels;
    bool shown = false;
    int currentPage = 0;
    float tickerX = menu::kRowFrom;
    int steps = 0, backgroundStep = -1;   // the 60 Hz clock, the background's last feedback copy
    float stepTime = 0;
    menu::ShellSlide slide;
    int slideOutTicks = -1, pageAfterSlideOut = -1;   // -1: not sliding out / no page asked for yet
    menu::UiEvent switchEvent = menu::UiEvent::LeftForward;   // the slide-out's sound at the switch
    int padsBefore = 0;                               // gamepads connected last frame
    bool padWasConnected[4]{};                        // each port, last frame (0x30a70)
    MainMenuPage mainMenuPage;
    GhostSelectPage ghostSelectPage;
    GhostSavePage ghostSavePage;
    SpeedSelectPage speedSelectPage;
    TrackInfoPage trackInfoPage;
    TourneyTypePage tourneyTypePage;
    EasterEggsPage easterEggsPage;
    CheatMenuPage cheatMenuPage;
    CreditsPage creditsPage;
    TitlePage titlePage;
    OptionsPage optionsPage;
    SettingsPage gameSettingsPage, soundSettingsPage;
    ArgonOptionsPage argonOptionsPage;
    SettingsPage argonGeneralPage, argonGraphicsPage, argonAudioPage, argonFixesPage;
    KeyBindingsPage keyBindingsPage;
    MusicSelectPage musicSelectPage;
    LevelMusicPage levelMusicPage;
    TrackRecordsPage trackRecordsPage;
    HighScoresPage highScoresPage;
    GhostDataPage ghostDataPage;
    ChooseSavePage chooseSavePage;
    NameEntryPage nameEntryPage;
    LoaderPage loaderPage;
    CharSelectPage charSelectPage;
    CharInfoPage charInfoPage;
    ShipInfoPage shipInfoPage;
    CourseSelectPage courseSelectPage;
    MessageBoxPage messageBoxPage;
    IntroScreensPage introScreensPage;
    AwardsPage awardsPage;
    std::vector<FePage *> pagesById;
};
