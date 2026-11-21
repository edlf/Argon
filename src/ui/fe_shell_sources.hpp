// Argon: what the front end's shell (ui\fe_shell.hpp) is built from: the state its own update and draw read
// (FeShellSources) and what each of its pages is bound to (FePageSources). App builds both (app\app_menu.cpp).
#pragma once
#include <memory>
#include <string>

#include "ui/fe_models.hpp"
#include "ui/ghostselect.hpp"
#include "ui/intro_screens.hpp"
#include "ui/key_bindings_page.hpp"
#include "ui/loader.hpp"
#include "ui/mainmenu.hpp"
#include "ui/multiplayer_select.hpp"
#include "ui/options.hpp"
#include "ui/profiles.hpp"
#include "ui/records.hpp"
#include "ui/speed.hpp"
#include "ui/tourney.hpp"

class FrameCopies;
class GameModes;
class GameStrings;
class HudPainter;
class LoadingFlyby;
class MultiplayerSession;
class PlayerInput;
class SettingsMenu;
class StoryScene;
namespace argon
{
class Audio;
}

struct FeShellSources
{
    const GameStrings &strings;
    HudPainter &painter;
    argon::Audio &audio;                        // the pages' sounds, the UI bank, the front end's music
    PlayerInput &players;                       // the menu presses, the controller ports
    const SettingsMenu &settingsMenu;           // Music Volume: no front-end music when off
    const GameModes &modes;                     // multiplayer: the players' ports
    const MultiplayerSession &multiplayer;
    FrameCopies &frameCopies;                   // the background's feedback, the scene transition
    const std::unique_ptr<StoryScene> &story;   // made after the shell; the loading screen over it has no background
    const LoadingFlyby &flyby;                  // the loading screen's <t>_glory.nms flyby, likewise
    const std::string &gameFolder;
    FeModels::Sources models;
};

// each page's own sources, in menu::Page order where a page has some
struct FePageSources
{
    MainMenuPage::Services mainMenu;
    PilotPicks pilotPicks;                      // FE CHARSELECT, Character Data, Ship Data, FE COURSE SELECT
    LoadingSources loading;
    MenuDoor &door;                             // FE TITLE
    ChooseSavePage::Services chooseSave;
    NameEntryPage::Services nameEntry;
    OptionSources options;                      // FE OPTIONS, the settings pages, Music Select
    KeyBindingsPage::Services keyBindings;      // Argon Options' Key Bindings
    RecordSources records;                      // Track Records, High Scores, Ghost Data
    argon::GameProgress &progress;              // FE EASTEREGGS, FE CHEATMENU
    ProgressStore &progressStore;
    CustomMusic &music;                         // FE LEVELMUSIC
    MusicStore &musicStore;
    SpeedSelectPage::Services speedSelect;
    TrackInfoPage::Services trackInfo;
    TourneyTypePage::Services tourneyType;
    GhostSelectPage::Services ghostSelect;
    GhostLibrary &ghosts;                       // FE GHOSTSAVE
    GhostKeeper &ghostKeeper;
    IntroScreensPage::Services introScreens;
    TourneyRun &tourney;                        // FE AWARDS
};
