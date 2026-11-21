// Argon: the front end's shell (ui\fe_shell.hpp): its pages, one object per menu::Page, and the shell's own
// update; its draw (background, title, ticker): ui\menu.cpp.
#include "ui/fe_shell.hpp"

#include <stdexcept>

#include "audio/audio.hpp"
#include "input/player_input.hpp"
#include "logic/game_logic.hpp"
#include "logic/multiplayer_session.hpp"
#include "ui/game_strings.hpp"
#include "ui/page_id.hpp"
#include "ui/settings_menu.hpp"
#include "ui/viewer_text.hpp"

namespace
{
// the pages that do not show the controller message (fe_shell_update 0x306b0: FE TITLE, FE INTROSCREENS; the
// loading screen has its own)
bool pageShowsControllerMessage(int page)
{
    return page != menu::PageTitle && page != menu::PageIntroScreens && page != menu::PageLoader;
}
}  // namespace

PageKit FeShell::pageKit()
{
    return PageKit{sources.strings, *this, menus_, sources.painter, sources.audio};
}

FeShell::FeShell(const FeShellSources &sources, const FePageSources &pages, FeShellHost &host)
    : sources(sources), host(host), menus_(sources.painter, sources.strings, *this), feModels(sources.models),
      mainMenuPage(pageKit(), pages.mainMenu), ghostSelectPage(pageKit(), pages.ghostSelect), ghostSavePage(pageKit(), pages.ghosts, pages.ghostKeeper),
      speedSelectPage(pageKit(), pages.speedSelect), trackInfoPage(pageKit(), pages.trackInfo), tourneyTypePage(pageKit(), pages.tourneyType),
      easterEggsPage(pageKit(), pages.progress, pages.progressStore), cheatMenuPage(pageKit(), pages.progress, pages.progressStore),
      creditsPage(pageKit(), sources.gameFolder), titlePage(pageKit(), pages.door), optionsPage(pageKit(), pages.options),
      gameSettingsPage(pageKit(), pages.options, {menu::PageGameSettings, SettingsList::Game, 0, 0x281, {u"Game Settings"}}),
      soundSettingsPage(pageKit(), pages.options, {menu::PageSoundSettings, SettingsList::Sound, 1, 0x282, {u"Sound Settings"}}),
      argonOptionsPage(pageKit()),
      argonGeneralPage(pageKit(),
                       pages.options,
                       {menu::PageArgonGeneral, SettingsList::ArgonGeneral, ArgonOptionsPage::kGeneral, -1, viewer_text::kGeneral.label}),
      argonGraphicsPage(pageKit(),
                        pages.options,
                        {menu::PageArgonGraphics, SettingsList::ArgonGraphics, ArgonOptionsPage::kGraphics, -1, viewer_text::kGraphics.label}),
      argonAudioPage(pageKit(), pages.options, {menu::PageArgonAudio, SettingsList::ArgonAudio, ArgonOptionsPage::kAudio, -1, viewer_text::kAudio.label}),
      argonFixesPage(pageKit(),
                     pages.options,
                     {menu::PageArgonFixes, SettingsList::ArgonFixes, ArgonOptionsPage::kGameFixes, -1, viewer_text::kGameFixes.label}),
      keyBindingsPage(pageKit(), pages.keyBindings),
      musicSelectPage(pageKit(), pages.options), levelMusicPage(pageKit(), pages.music, pages.musicStore), trackRecordsPage(pageKit(), pages.records),
      highScoresPage(pageKit(), pages.records), ghostDataPage(pageKit(), pages.records), chooseSavePage(pageKit(), pages.chooseSave),
      nameEntryPage(pageKit(), pages.nameEntry), loaderPage(pageKit(), pages.loading), charSelectPage(pageKit(), pages.pilotPicks),
      charInfoPage(pageKit(), pages.pilotPicks), shipInfoPage(pageKit(), pages.pilotPicks), courseSelectPage(pageKit(), pages.pilotPicks),
      messageBoxPage(pageKit(), sources.players), introScreensPage(pageKit(), pages.introScreens), awardsPage(pageKit(), pages.tourney),
      pagesById{// in menu::Page order
                &mainMenuPage,     &charSelectPage,  &charInfoPage,    &shipInfoPage,     &courseSelectPage,  &loaderPage,     &titlePage,
                &chooseSavePage,   &nameEntryPage,   &optionsPage,     &gameSettingsPage, &soundSettingsPage, &creditsPage,    &musicSelectPage,
                &trackRecordsPage, &highScoresPage,  &ghostDataPage,   &easterEggsPage,   &cheatMenuPage,     &levelMusicPage, &speedSelectPage,
                &trackInfoPage,    &tourneyTypePage, &ghostSelectPage, &ghostSavePage,    &introScreensPage,  &awardsPage,     &argonOptionsPage,
                &argonGeneralPage, &argonGraphicsPage, &argonAudioPage, &argonFixesPage,  &keyBindingsPage}
{
    if (pagesById.size() != size_t(menu::PageCount))
    {
        throw std::logic_error("FeShell: " + std::to_string(pagesById.size()) + " pages for " + std::to_string(int(menu::PageCount)) + " menu::Page ids");
    }
}

FeShell::~FeShell() = default;

SettingsPage &FeShell::settings(int id)
{
    if (id == menu::PageGameSettings)
    {
        return gameSettingsPage;
    }
    if (id == menu::PageSoundSettings)
    {
        return soundSettingsPage;
    }
    if (id == menu::PageArgonGeneral)
    {
        return argonGeneralPage;
    }
    if (id == menu::PageArgonGraphics)
    {
        return argonGraphicsPage;
    }
    if (id == menu::PageArgonAudio)
    {
        return argonAudioPage;
    }
    if (id == menu::PageArgonFixes)
    {
        return argonFixesPage;
    }
    throw std::invalid_argument("FeShell::settings: page " + std::to_string(id) + " is not a settings page");
}

FePage &FeShell::active()
{
    if (messageBoxPage.isOpen())
    {
        return messageBoxPage;
    }
    return page(currentPage);
}

// main_menu 0x30af0: its clock from 0, FE MAIN (the shell's models come with the preload, ui\fe_models.hpp)
void FeShell::open()
{
    if (!shown)
    {
        slideOutTicks = -1;   // already open (a page going back to FE MAIN): a slide-out still holds the page
    }
    shown = true;
    steps = 0;
    stepTime = 0;
    goTo(menu::PageMain);
}

// fe_shell_goto_page 0x31390, held back while the title and hints slide out
void FeShell::goTo(int id)
{
    if (isSlidingOut())
    {
        pageAfterSlideOut = id;
        return;
    }
    show(id);
}

// fe_shell_goto_page: it leaves the message box too; the new page's enter calls fe_shell_set_state(1)
void FeShell::show(int id)
{
    messageBoxPage.close();
    currentPage = id;
    restartTicker();
    slide.enter();
    sources.audio.enterFrontEndPage(id != menu::PageTitle && id != menu::PageIntroScreens &&
                                sources.settingsMenu.volumeMillibels(SettingsMenu::kMusicVolume) <= -10000.f);
    page(id).enter();
}

// fe_shell_set_state(4) on leaving FE MAIN, FE OPTIONS, FE CHOOSESAVE, FE CHARSELECT and FE COURSE SELECT: their
// update goes to the next page once the clock is 0.1 s past the pick (0x2a520, 0x2d830, 0x1bab0, 0x15db0, 0x1d860;
// 0x1a3508); the other pages call state 4 and switch at once. atSwitch (LeftBack / LeftForward): the page's sound
// for the switch, which those updates play (ui\fe_sounds.hpp)
void FeShell::slideOut(menu::UiEvent atSwitch)
{
    slide.leave();
    slideOutTicks = 0;
    pageAfterSlideOut = -1;
    switchEvent = atSwitch;
}

// one 60 Hz step of a slide-out: past 0.1 s the page plays its switch sound and the page asked for opens (none: the
// page is told)
void FeShell::stepSlideOut()
{
    constexpr int kSlideOutTicks = 6;   // 0.1 s at 60 Hz; the update waits for the clock to pass it
    if (!isSlidingOut() || ++slideOutTicks <= kSlideOutTicks)
    {
        return;
    }
    slideOutTicks = -1;
    page(currentPage).play(switchEvent);
    if (pageAfterSlideOut >= 0)
    {
        show(pageAfterSlideOut);
    }
    else
    {
        page(currentPage).slideOutDone();
    }
}

// one frame: the UI sounds' bank, the preloaded models read so far uploaded (not while the loading screen's frames go to the
// track and its flyby), the pad to the page (or the message box over it; each plays its own sounds), the ticker and
// the 60 Hz steps
void FeShell::update(float dt)
{
    if (!shown)
    {
        return;
    }
    sources.audio.loadUi(sources.gameFolder);
    if (currentPage != menu::PageLoader)
    {
        feModels.uploadPreloaded();
    }
    checkControllers();
    const menu::MenuInput in = sources.players.readMenu();
    if (!isSlidingOut())
    {
        active().input(in);   // the pages take no input in their sequences 4 / 7
    }
    if (shown)
    {
        step(dt);
    }
}

// fe_shell_update 0x306b0: with no pad left the shell opens FE MESSAGEBOX kind 2 (fe 7, "Please reconnect the
// controller and press START to continue."); here when the last gamepad goes (the keyboard alone never asks)
void FeShell::checkControllers()
{
    checkPlayersControllers();
    const int pads = connectedGamepads();
    const bool lost = pads == 0 && padsBefore > 0;
    padsBefore = pads;
    if (!lost || messageBoxPage.isOpen() || !pageShowsControllerMessage(currentPage))
    {
        return;
    }
    messageBoxPage.open(sources.strings.frontEnd(7, "Please reconnect the controller and press START to continue."), MessageBoxPage::Kind::Controller);
}

// 0x30a70: a player's port losing its controller opens the controller message for that port, strings 0x44c + port
// ("Please insert a controller into port n, and press START."), kind 3 in multiplayer: only that port closes it
void FeShell::checkPlayersControllers()
{
    for (int pad = 0; pad < 4; ++pad)
    {
        const bool now = sources.players.portConnected(pad);
        const bool lost = padWasConnected[pad] && !now;
        padWasConnected[pad] = now;
        if (lost && padInUse(pad) && !messageBoxPage.isOpen() && pageShowsControllerMessage(currentPage))
        {
            messageBoxPage.openForPad(sources.strings.general(0x44c + pad, "Please insert a controller, and press START."), pad);
        }
    }
}

// the ports of the multiplayer session's players (those who joined on FE CHARSELECT, or raced)
bool FeShell::padInUse(int pad) const
{
    if (!sources.modes.multiplayerSelected())
    {
        return false;
    }
    if (charSelectPage.hasJoined(pad))
    {
        return true;
    }
    return sources.multiplayer.padRaces(pad);
}

// the ticker by the frame time (smooth at the display's rate, the game's 2 px per 60 Hz frame; ui\menu_common.hpp);
// the page's elements once per 60 Hz frame, as the game steps them
void FeShell::step(float dt)
{
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const float originX = (displaySize.x * 480.f / displaySize.y - 640.f) * 0.5f;
    tickerX = menu::scrollTicker(tickerX, dt, menu::tickerWrapWidth(sources.painter.textWidth(tickerText().value_or(u""), 0, 0.75f), originX));
    stepTime += dt;
    while (stepTime >= 1.f / 60.f && shown)
    {
        stepTime -= 1.f / 60.f;
        ++steps;
        slide.step();
        page(currentPage).step();
        messageBoxPage.step();
        stepSlideOut();
    }
}

int FeShell::connectedGamepads()
{
    int pads = 0;
    for (int j = GLFW_JOYSTICK_1; j <= GLFW_JOYSTICK_LAST; ++j)
    {
        pads += glfwJoystickIsGamepad(j) ? 1 : 0;
    }
    return pads;
}

FePage &FeShell::page(int id) const
{
    if (id < 0 || id >= int(pagesById.size()))
    {
        throw std::out_of_range("FeShell::page: no front-end page " + std::to_string(id));
    }
    return *pagesById[size_t(id)];
}
