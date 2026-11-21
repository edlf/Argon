// Argon: what the Options pages' choices do (app\settings_keeper.hpp).
#include "app/settings_keeper.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

#include "audio/audio.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "render/renderer.hpp"
#include "saves/profile_store.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_common.hpp"
#include "ui/settings_menu.hpp"
#include "window/window_mode.hpp"

namespace
{
// a volume step's millibels as a gain; step 0 silent
float volumeGain(float millibels)
{
    return millibels <= -10000.f ? 0.f : std::pow(10.f, millibels / 2000.f);
}
}  // namespace

void SettingsKeeper::buildSettings()
{
    if (sources_.menu.hasProfileLists())
    {
        return;
    }
    sources_.menu.buildProfileLists(sources_.strings);
    buildArgonSettings();
}

void SettingsKeeper::buildArgonSettings()
{
    const std::vector<argon::Resolution> resolutions = offeredResolutions();
    sources_.menu.buildViewerList(sources_.strings, {sources_.viewerSettings, sources_.languages, resolutions});
}

// the monitor's sizes; the one in use too when the monitor offers none (no monitor to ask)
std::vector<argon::Resolution> SettingsKeeper::offeredResolutions() const
{
    if (sources_.windowMode.offeredResolutions().empty())
    {
        return {argon::validResolution(sources_.viewerSettings.resolution(), {})};
    }
    return sources_.windowMode.offeredResolutions();
}

// the profile's entries: applied and written to the profile; Argon Options' only to settings.json
void SettingsKeeper::changeSetting(int type, int option)
{
    sources_.menu.select(type, option);
    if (sources_.menu.isViewerSetting(type))
    {
        storeArgonSettings();
        return;
    }
    storeSettings();
}

// what the viewer applies: the volumes, the brightness, the speed units (HUD), the HUD (Off hides it, Map Off the
// minimap) and vibration. Laps and headphones are kept only.
// Volumes: the game's curve (0x1a8564, through 0x2eb80 / 0xdee90) gives DirectSound millibels per step 0..7, played as
// they are; each voice path's headroom comes on top (audio\mix_paths.hpp).
void SettingsKeeper::applySettings()
{
    if (!sources_.menu.hasProfileLists())
    {
        return;
    }
    sources_.audio.engineVol = volumeGain(sources_.menu.volumeMillibels(SettingsMenu::kEngineVolume));
    sources_.audio.sfxVol = volumeGain(sources_.menu.volumeMillibels(SettingsMenu::kSfxVolume));
    sources_.audio.musicVol = volumeGain(sources_.menu.volumeMillibels(SettingsMenu::kMusicVolume));
    sources_.applied.speedMph = sources_.menu.current(SettingsMenu::kSpeedUnits) == 1;
    sources_.applied.showHud = sources_.menu.current(SettingsMenu::kHud) != 0;
    sources_.applied.showMap = sources_.menu.current(SettingsMenu::kHud) == 2;
    sources_.applied.brightness = std::clamp(sources_.menu.current(SettingsMenu::kBrightness), 0, 7);
}

void SettingsKeeper::loadSettingsString(const std::string &options)
{
    buildSettings();
    sources_.menu.readProfileString(options);
    applySettings();
}

// the settings belong to the profile (the game saves them with it): written to its JSON as "options"
void SettingsKeeper::storeSettings()
{
    applySettings();
    if (const auto current = sources_.profiles.current())
    {
        argon::Profile &profile = *current;
        profile.options = sources_.menu.profileString();
        sources_.profiles.save(profile);
    }
}

// settings.json's values with Argon Options' rows in them
ViewerSettings SettingsKeeper::chosenArgonSettings() const
{
    const std::vector<argon::Resolution> resolutions = offeredResolutions();
    return sources_.menu.chosenViewerSettings({sources_.viewerSettings, sources_.languages, resolutions});
}

// what changed from settings.json's values; the window only when its rows did (--size and the screenshot runs keep
// their window until then). Leaving fullscreen first and entering it last switches the monitor's mode once
void SettingsKeeper::applyArgonSettings(const ViewerSettings &chosen)
{
    menu::widescreenLayout = chosen.widescreen();
    sources_.renderer.presenter().setVsync(chosen.vsync());
    sources_.audio.setHrtfSet(chosen.hrtf());
    sources_.audio.setKemarTaps(chosen.kemarTaps());
    const bool switched = chosen.fullscreen() != sources_.viewerSettings.fullscreen();
    if (switched && !chosen.fullscreen())
    {
        sources_.windowMode.setFullscreen(sources_.window, false);
    }
    if (chosen.resolution() != sources_.viewerSettings.resolution())
    {
        sources_.windowMode.setResolution(sources_.window, chosen.resolution().value_or(argon::Resolution()));
    }
    if (switched && chosen.fullscreen())
    {
        sources_.windowMode.setFullscreen(sources_.window, true);
    }
}

// applied and written when something changed (Save, the pause menu's HUD Size)
void SettingsKeeper::storeArgonSettings()
{
    const ViewerSettings chosen = chosenArgonSettings();
    if (chosen == sources_.viewerSettings)
    {
        return;
    }
    const bool relanguaged = chosen.language() != sources_.viewerSettings.language();
    applyArgonSettings(chosen);
    sources_.viewerSettings = chosen;
    sources_.viewerSettings.save(std::filesystem::path(sources_.savesFolder) / "settings.json");
    if (relanguaged)
    {
        sources_.strings = readGameStrings(sources_.gameFolder + "/" + sources_.viewerSettings.localeFolder() + "/", sources_.viewerSettings.language());
        relabelSettings();
    }
}

bool SettingsKeeper::chosenRendererNeedsRestart() const
{
    return argon::gfx::BackendKind(sources_.menu.current(SettingsMenu::kRenderer)) != sources_.renderer.kind();
}

// the lists hold their labels as text: built again from the tables, the profile's choices put back
void SettingsKeeper::relabelSettings()
{
    const std::string chosen = sources_.menu.profileString();
    sources_.menu.clearProfileLists();
    loadSettingsString(chosen);
}

// FE OPTIONS shows Easter Eggs with the cheat menu or any egg, Cheats with the cheat menu (0x2d9d0)
bool SettingsKeeper::easterEggsShown() const
{
    return sources_.progress.cheatMenu || sources_.progressKeeper.anyEgg();
}

bool SettingsKeeper::cheatsShown() const
{
    return sources_.progress.cheatMenu;
}

const KeyBindings &SettingsKeeper::keyBindings() const
{
    return sources_.viewerSettings.keyBindings();
}

// written at once; the keyboard reads them from settings.json's values (input\player_input.hpp)
void SettingsKeeper::storeKeyBindings(const KeyBindings &bindings)
{
    if (bindings == sources_.viewerSettings.keyBindings())
    {
        return;
    }
    sources_.viewerSettings.setKeyBindings(bindings);
    sources_.viewerSettings.save(std::filesystem::path(sources_.savesFolder) / "settings.json");
}
