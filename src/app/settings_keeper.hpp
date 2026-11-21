// Argon: what the Options pages' choices do (ui\page_hosts.hpp SettingsStore): the Game / Sound Settings (lists
// 0x81fd0 / 0x824c0, ui\settings_menu.hpp) kept in the profile's "options" string and applied to the volumes, the
// brightness, the speed units and the HUD; the viewer's own Argon Options kept in settings.json and applied to the
// layout, the window, vsync, HRTF, the language and the keyboard's keys. The pages: ui\options.cpp,
// ui\key_bindings_page.cpp.
#pragma once
#include <string>
#include <vector>

#include "saves/viewer_settings.hpp"
#include "ui/page_hosts.hpp"
#include "window/resolution.hpp"

class GameStrings;
class ProgressKeeper;
class Renderer;
class SettingsMenu;
struct GLFWwindow;
namespace argon
{
class Audio;
class ProfileStore;
class WindowMode;
struct GameProgress;
}

// what the Game / Sound Settings set for the race and the HUD (Laps and Headphones are kept only)
struct AppliedSettings
{
    bool speedMph = false;   // Speed Units: mph (race_strings 624)
    bool showHud = true;     // HUD Off hides it (and --no-hud)
    bool showMap = true;     // HUD Map Off hides the minimap
    int brightness = 4;      // 0..7 (the gamma ramp 0x2f100)
};

struct SettingsKeeperSources
{
    SettingsMenu &menu;                                   // the rows
    GameStrings &strings;                                 // their labels; read again in a new language
    ViewerSettings &viewerSettings;                       // settings.json's values
    const std::vector<Language> &languages;               // the Language row's (App::findTranslatedLanguages)
    argon::Audio &audio;                                  // the volumes, HRTF
    Renderer &renderer;                                   // vsync, the backend in use
    argon::WindowMode &windowMode;                        // Resolution, Fullscreen
    GLFWwindow *const &window;                            // made at start-up
    const std::string &gameFolder;                        // the language's string tables
    const std::string &savesFolder;                       // settings.json
    argon::ProfileStore &profiles;                        // the profile's "options"
    const argon::GameProgress &progress;                  // the cheat menu shown
    const ProgressKeeper &progressKeeper;                 // any egg won
    AppliedSettings &applied;
};

class SettingsKeeper final : public SettingsStore, public KeyBindingStore
{
  public:
    explicit SettingsKeeper(const SettingsKeeperSources &sources) : sources_(sources)
    {
    }
    void loadSettingsString(const std::string &options) override;
    void buildSettings() override;
    void buildArgonSettings() override;
    void changeSetting(int type, int option) override;
    void storeSettings() override;
    void storeArgonSettings() override;
    bool chosenRendererNeedsRestart() const override;
    bool easterEggsShown() const override;
    bool cheatsShown() const override;
    const KeyBindings &keyBindings() const override;
    void storeKeyBindings(const KeyBindings &bindings) override;

  private:
    std::vector<argon::Resolution> offeredResolutions() const;
    void applySettings();
    void relabelSettings();
    ViewerSettings chosenArgonSettings() const;
    void applyArgonSettings(const ViewerSettings &chosen);

    SettingsKeeperSources sources_;
};
