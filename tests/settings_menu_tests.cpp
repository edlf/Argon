// Argon: tests for the settings lists (ui\settings_menu.hpp): the profile's options string, the viewer's rows and
// what they choose. No window or GPU: run with test.bat (or ctest in build\).
#include "check.hpp"
#include "ui/game_strings.hpp"
#include "ui/settings_menu.hpp"
#include "ui/viewer_text.hpp"

#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

const GameStrings kNoTables;
const std::vector<Language> kLanguages{Language::English, Language::French};
const std::vector<argon::Resolution> kResolutions{{1280, 720}, {1920, 1080}};

SettingsMenu profileMenu()
{
    SettingsMenu menu;
    menu.buildProfileLists(kNoTables);
    return menu;
}

SettingsMenu viewerMenu(const ViewerSettings &settings)
{
    SettingsMenu menu;
    menu.buildViewerList(kNoTables, {settings, kLanguages, kResolutions});
    return menu;
}

void testProfileStringKeepsTheChoices()
{
    SettingsMenu written = profileMenu();
    written.select(SettingsMenu::kHud, 1);
    written.select(SettingsMenu::kMusicVolume, 7);
    SettingsMenu read = profileMenu();
    read.readProfileString(written.profileString());
    CHECK(read.current(SettingsMenu::kHud) == 1 && read.current(SettingsMenu::kMusicVolume) == 7);
}

void testProfileStringLeavesUnnamedRowsAtTheirDefaults()
{
    SettingsMenu menu = profileMenu();
    menu.select(SettingsMenu::kBrightness, 0);
    menu.readProfileString("15=1;");
    CHECK(menu.current(SettingsMenu::kBrightness) == 4);
}

void testProfileStringOptionOutOfRangeIsIgnored()
{
    SettingsMenu menu = profileMenu();
    menu.readProfileString("15=9;");
    CHECK(menu.current(SettingsMenu::kHud) == 2);
}

void testVolumeBeforeTheListsIsTheDefaultStep()
{
    const SettingsMenu menu;
    CHECK(menu.volumeMillibels(SettingsMenu::kMusicVolume) == -400.f);
}

void testHudSizeIsAViewerSetting()
{
    const SettingsMenu menu = viewerMenu(ViewerSettings());
    CHECK(menu.isViewerSetting(SettingsMenu::kHudSize) && !menu.isViewerSetting(SettingsMenu::kHud));
}

void testViewerRowsStartFromTheSettings()
{
    ViewerSettings settings;
    settings.setFpsMeter(true);
    settings.setLanguage(Language::French);
    const SettingsMenu menu = viewerMenu(settings);
    CHECK(menu.current(SettingsMenu::kFpsMeter) == 1 && menu.current(SettingsMenu::kLanguage) == 1);
}

void testChosenViewerSettingsTakeTheRows()
{
    const ViewerSettings settings;
    SettingsMenu menu = viewerMenu(settings);
    menu.select(SettingsMenu::kResolution, 1);
    menu.select(SettingsMenu::kFmv, 0);
    const ViewerSettings chosen = menu.chosenViewerSettings({settings, kLanguages, kResolutions});
    const argon::Resolution wide{1920, 1080};
    CHECK(chosen.resolution() == wide && chosen.skipIntroVideos());
}

void testViewerRowLabelIsTheViewersOwnText()
{
    const SettingsMenu menu = viewerMenu(ViewerSettings());
    const SettingRow &save = menu.rows(SettingsList::ArgonGeneral).front();
    CHECK(SettingsMenu::label(save, kNoTables) == viewer_text::kSave.label.in(Language::English));
}
bool listHasRow(const SettingsMenu &menu, SettingsList list, int type)
{
    for (const SettingRow &row : menu.rows(list))
    {
        if (row.type == type)
        {
            return true;
        }
    }
    return false;
}

void testArgonOptionsListsSplitTheViewerRows()
{
    const SettingsMenu menu = viewerMenu(ViewerSettings());
    CHECK(listHasRow(menu, SettingsList::ArgonGraphics, SettingsMenu::kMsaa) && listHasRow(menu, SettingsList::ArgonAudio, SettingsMenu::kHrtf) &&
          listHasRow(menu, SettingsList::ArgonFixes, SettingsMenu::kMarsBlurFix) && !listHasRow(menu, SettingsList::ArgonGeneral, SettingsMenu::kMsaa));
}

void testEveryArgonOptionsListStartsWithSave()
{
    const SettingsMenu menu = viewerMenu(ViewerSettings());
    bool startsWithSave = true;
    for (const SettingsList list : {SettingsList::ArgonGeneral, SettingsList::ArgonGraphics, SettingsList::ArgonAudio, SettingsList::ArgonFixes})
    {
        startsWithSave = startsWithSave && menu.rows(list).front().type == SettingsMenu::kSave;
    }
    CHECK(startsWithSave);
}

void testGameFixesRowIsAViewerSetting()
{
    const SettingsMenu menu = viewerMenu(ViewerSettings());
    CHECK(menu.isViewerSetting(SettingsMenu::kMarsBlurFix));
}

void testBindingAKeyTakesItFromItsOldAction()
{
    KeyBindings keys;
    keys.bind(KeyAction::Turbo, 0, keyNamed("W"));
    CHECK(keys.key(KeyAction::Throttle, 0) == kUnboundKey && keys.key(KeyAction::Turbo, 0) == keyNamed("W"));
}

void testKeyNamesReadBack()
{
    bool readBack = true;
    for (const int key : nameableKeys())
    {
        readBack = readBack && keyNamed(keyName(key)) == key;
    }
    CHECK(readBack);
}

void testUnboundKeyIsNamedWithADash()
{
    CHECK(keyName(kUnboundKey) == "-");
}
}  // namespace

int main()
{
    testProfileStringKeepsTheChoices();
    testProfileStringLeavesUnnamedRowsAtTheirDefaults();
    testProfileStringOptionOutOfRangeIsIgnored();
    testVolumeBeforeTheListsIsTheDefaultStep();
    testHudSizeIsAViewerSetting();
    testViewerRowsStartFromTheSettings();
    testChosenViewerSettingsTakeTheRows();
    testViewerRowLabelIsTheViewersOwnText();
    testArgonOptionsListsSplitTheViewerRows();
    testEveryArgonOptionsListStartsWithSave();
    testGameFixesRowIsAViewerSetting();
    testBindingAKeyTakesItFromItsOldAction();
    testKeyNamesReadBack();
    testUnboundKeyIsNamedWithADash();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
