// Argon: App's side of the settings: the languages translated, the FPS meter and fullscreen switched from the keys.
// What the Options pages' choices do: app\settings_keeper.hpp.

#include "app/app.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"
#include "files/string_table.hpp"

#include <algorithm>
#include <cstdio>

// English always; Japanese, French, German and Italian when their fe_strings.str is there and is no placeholder
// (the USA disc's European tables are: docs\frontend\intro-title.md)
void App::findTranslatedLanguages()
{
    translatedLanguages = {Language::English};
    for (const Language language : {Language::French, Language::German, Language::Italian, Language::Japanese})
    {
        ViewerSettings probe;
        probe.setLanguage(language);
        const std::string table = game + "/" + probe.localeFolder() + "/fe_strings.str";
        if (argon::isPlaceholderTranslation(argon::parseStringTable(readBytes(table))))
        {
            std::fprintf(stderr, "language: %s has no translation (%s is missing or placeholder text)\n", nameOf(language), table.c_str());
            continue;
        }
        translatedLanguages.push_back(language);
    }
    if (std::find(translatedLanguages.begin(), translatedLanguages.end(), viewerSettings.language()) == translatedLanguages.end())
    {
        std::fprintf(stderr, "language: settings.json's %s has no translation: English\n", nameOf(viewerSettings.language()));
        viewerSettings.setLanguage(Language::English);
    }
}

// settings.json's FPS Meter; the developer build's Debug menu shows it too while open
bool App::fpsMeterShown() const
{
#if ARGON_DEVELOP
    if (showPanel && !fe->isOpen())
    {
        return true;
    }
#endif
    return viewerSettings.fpsMeter();
}

// written at once, as F11 (an Argon Options page left with B keeps it: its row follows)
void App::toggleFpsMeter()
{
    const bool on = !viewerSettings.fpsMeter();
    viewerSettings.setFpsMeter(on);
    viewerSettings.save(std::filesystem::path(saves) / "settings.json");
    settingsMenu.select(SettingsMenu::kFpsMeter, on ? 1 : 0);
}

// written at once (an Argon Options page left with B keeps it: its row follows)
void App::toggleFullscreen()
{
    const bool on = !windowMode.fullscreen();
    windowMode.setFullscreen(window, on);
    viewerSettings.setFullscreen(on);
    viewerSettings.save(std::filesystem::path(saves) / "settings.json");
    settingsMenu.select(SettingsMenu::kFullscreen, on ? 1 : 0);
}
