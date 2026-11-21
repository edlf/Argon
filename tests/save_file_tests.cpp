// Argon: tests for the viewer's JSON files: settings.json (viewer_settings.hpp) and the lap records
// (lap_records.hpp). No window or GPU: run with test.bat (or ctest in build\). Each test works in its own folder under
// the system temp folder.
#include "check.hpp"
#include "saves/lap_records.hpp"
#include "saves/profile_fields.hpp"
#include "saves/profile_file.hpp"
#include "saves/profile_store.hpp"
#include "saves/viewer_settings.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace
{
using argon::test::checks;
using argon::test::failures;

fs::path freshFolder(const char *name)
{
    const fs::path folder = fs::temp_directory_path() / "argon_tests" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

void writeFile(const fs::path &file, const std::string &text)
{
    std::ofstream(file) << text;
}

void testSettingsDefaultWhenMissing()
{
    const fs::path file = freshFolder("settings_missing") / "settings.json";
    const ViewerSettings settings = ViewerSettings::load(file);
    CHECK(settings.language() == Language::English && !settings.skipIntroVideos());
}

void testSettingsFileWrittenWhenMissing()
{
    const fs::path file = freshFolder("settings_written") / "settings.json";
    ViewerSettings::load(file);
    CHECK(fs::exists(file));
}

void testSettingsReadsLanguageAndIntro()
{
    const fs::path file = freshFolder("settings_read") / "settings.json";
    writeFile(file, R"({"language": "German", "skipIntroVideos": true})");
    const ViewerSettings settings = ViewerSettings::load(file);
    CHECK(settings.language() == Language::German && settings.skipIntroVideos());
}

void testSettingsLocaleFolder()
{
    const fs::path file = freshFolder("settings_locale") / "settings.json";
    writeFile(file, R"({"language": "Italian"})");
    CHECK(ViewerSettings::load(file).localeFolder() == "Locale/Italian");
}

void testSettingsSpanishIsEnglish()
{
    const fs::path file = freshFolder("settings_spanish") / "settings.json";
    writeFile(file, R"({"language": "Spanish"})");
    CHECK(ViewerSettings::load(file).language() == Language::English);
}

void testSettingsUnknownLanguageIsEnglish()
{
    const fs::path file = freshFolder("settings_unknown") / "settings.json";
    writeFile(file, R"({"language": "Klingon"})");
    CHECK(ViewerSettings::load(file).language() == Language::English);
}

void testSettingsBrokenFileUsesDefaults()
{
    const fs::path file = freshFolder("settings_broken") / "settings.json";
    writeFile(file, "{ not json");
    const ViewerSettings settings = ViewerSettings::load(file);
    CHECK(settings.language() == Language::English && !settings.skipIntroVideos());
}

void testSettingsReadsTheHudSize()
{
    const fs::path file = freshFolder("settings_hud_size") / "settings.json";
    writeFile(file, R"({"hudSize": 50})");
    CHECK(ViewerSettings::load(file).hudScale() == 0.5f);
}

void testSettingsHudSizeOfNoStepIsFull()
{
    const fs::path file = freshFolder("settings_hud_size_odd") / "settings.json";
    writeFile(file, R"({"hudSize": 60})");
    CHECK(ViewerSettings::load(file).hudSizePercent() == 100);
}

void testSettingsHudSizeRoundTrips()
{
    const fs::path file = freshFolder("settings_hud_size_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setHudSizeStep(0);
    written.save(file);
    CHECK(ViewerSettings::load(file).hudSizePercent() == 25);
}

void testSettingsKeyBindingsRoundTrip()
{
    const fs::path file = freshFolder("settings_keys_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    KeyBindings keys;
    keys.bind(KeyAction::Turbo, 1, keyNamed("Left Shift"));
    written.setKeyBindings(keys);
    written.save(file);
    CHECK(ViewerSettings::load(file).keyBindings() == keys);
}

void testSettingsKeyBindingsLeftOutKeepTheirDefaults()
{
    const fs::path file = freshFolder("settings_keys_partial") / "settings.json";
    writeFile(file, R"({"keyBindings": {"turbo": ["Enter", "-"]}})");
    const KeyBindings keys = ViewerSettings::load(file).keyBindings();
    CHECK(keys.key(KeyAction::Turbo, 0) == keyNamed("Enter") && keys.key(KeyAction::Throttle, 0) == keyNamed("W"));
}

void testSettingsReservedKeyIsNotBound()
{
    const fs::path file = freshFolder("settings_keys_reserved") / "settings.json";
    writeFile(file, R"({"keyBindings": {"turbo": ["Esc", "-"]}})");
    CHECK(ViewerSettings::load(file).keyBindings().key(KeyAction::Turbo, 0) == kUnboundKey);
}

void testSettingsResolutionRoundTrips()
{
    const fs::path file = freshFolder("settings_resolution_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setResolution({1280, 720});
    written.save(file);
    CHECK((ViewerSettings::load(file).resolution() == argon::Resolution{1280, 720}));
}

void testSettingsUnreadableResolutionIsNone()
{
    const fs::path file = freshFolder("settings_resolution_bad") / "settings.json";
    writeFile(file, R"({"resolution": "big"})");
    CHECK(!ViewerSettings::load(file).resolution());
}

void testSettingsMissingResolutionIsNone()
{
    const fs::path file = freshFolder("settings_resolution_missing") / "settings.json";
    writeFile(file, R"({"vsync": true})");
    CHECK(!ViewerSettings::load(file).resolution());
}

void testSettingsFullscreenIsOnByDefault()
{
    const fs::path file = freshFolder("settings_fullscreen_default") / "settings.json";
    CHECK(ViewerSettings::load(file).fullscreen());
}

void testSettingsFullscreenRoundTrips()
{
    const fs::path file = freshFolder("settings_fullscreen_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setFullscreen(false);
    written.save(file);
    CHECK(!ViewerSettings::load(file).fullscreen());
}

void testSettingsVsyncIsOnByDefault()
{
    const fs::path file = freshFolder("settings_vsync_default") / "settings.json";
    CHECK(ViewerSettings::load(file).vsync());
}

void testSettingsReadsVsyncOff()
{
    const fs::path file = freshFolder("settings_vsync_off") / "settings.json";
    writeFile(file, R"({"vsync": false})");
    CHECK(!ViewerSettings::load(file).vsync());
}

void testSettingsFpsMeterIsOffByDefault()
{
    const fs::path file = freshFolder("settings_fps_default") / "settings.json";
    CHECK(!ViewerSettings::load(file).fpsMeter());
}

void testSettingsFpsMeterRoundTrips()
{
    const fs::path file = freshFolder("settings_fps_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setFpsMeter(true);
    written.save(file);
    CHECK(ViewerSettings::load(file).fpsMeter());
}

void testSettingsWidescreenIsOnByDefault()
{
    const fs::path file = freshFolder("settings_widescreen_default") / "settings.json";
    CHECK(ViewerSettings::load(file).widescreen());
}

void testSettingsWidescreenRoundTrips()
{
    const fs::path file = freshFolder("settings_widescreen_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setWidescreen(false);
    written.save(file);
    CHECK(!ViewerSettings::load(file).widescreen());
}

void testSettingsSpeedBlurIsOnByDefault()
{
    const fs::path file = freshFolder("settings_speed_blur_default") / "settings.json";
    CHECK(ViewerSettings::load(file).speedBlur());
}

void testSettingsSpeedBlurRoundTrips()
{
    const fs::path file = freshFolder("settings_speed_blur_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setSpeedBlur(false);
    written.save(file);
    CHECK(!ViewerSettings::load(file).speedBlur());
}

void testSettingsMarsSpeedBlurFixIsOnByDefault()
{
    const fs::path file = freshFolder("settings_mars_blur_fix_default") / "settings.json";
    CHECK(ViewerSettings::load(file).marsSpeedBlurFix());
}

void testSettingsMarsSpeedBlurFixRoundTrips()
{
    const fs::path file = freshFolder("settings_mars_blur_fix_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setMarsSpeedBlurFix(false);
    written.save(file);
    CHECK(!ViewerSettings::load(file).marsSpeedBlurFix());
}

void testSettingsKemarTapsAreSixtyFourByDefault()
{
    const fs::path file = freshFolder("settings_kemar_taps_default") / "settings.json";
    CHECK(ViewerSettings::load(file).kemarTaps() == 64);
}

void testSettingsKemarTapsRoundTrip()
{
    const fs::path file = freshFolder("settings_kemar_taps_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setKemarTaps(256);
    written.save(file);
    CHECK(ViewerSettings::load(file).kemarTaps() == 256);
}

void testSettingsKemarTapsBetweenChoicesTakeTheOneBelow()
{
    const fs::path file = freshFolder("settings_kemar_taps_between") / "settings.json";
    writeFile(file, R"({"kemarTaps": 300})");
    CHECK(ViewerSettings::load(file).kemarTaps() == 256);
}

void testSettingsMsaaIsEightByDefault()
{
    const fs::path file = freshFolder("settings_msaa_default") / "settings.json";
    CHECK(ViewerSettings::load(file).msaa() == 8);
}

void testSettingsLanguageRoundTrips()
{
    const fs::path file = freshFolder("settings_language_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setLanguage(Language::German);
    written.save(file);
    CHECK(ViewerSettings::load(file).language() == Language::German);
}

void testSettingsRendererRoundTrips()
{
    const fs::path file = freshFolder("settings_renderer_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setRenderer(argon::gfx::BackendKind::Direct3D12);
    written.save(file);
    CHECK(ViewerSettings::load(file).renderer() == argon::gfx::BackendKind::Direct3D12);
}

void testSettingsMsaaRoundTrips()
{
    const fs::path file = freshFolder("settings_msaa_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setMsaa(8);
    written.save(file);
    CHECK(ViewerSettings::load(file).msaa() == 8);
}

void testSettingsMsaaBetweenChoicesTakesTheOneBelow()
{
    const fs::path file = freshFolder("settings_msaa_between") / "settings.json";
    writeFile(file, R"({"msaa": 6})");
    CHECK(ViewerSettings::load(file).msaa() == 4);
}

void testSettingsPostAntiAliasingIsSmaaByDefault()
{
    const fs::path file = freshFolder("settings_post_aa_default") / "settings.json";
    CHECK(ViewerSettings::load(file).postAntiAliasing() == PostAntiAliasing::Smaa);
}

void testSettingsPostAntiAliasingRoundTrips()
{
    const fs::path file = freshFolder("settings_post_aa_saved") / "settings.json";
    ViewerSettings written = ViewerSettings::load(file);
    written.setPostAntiAliasing(PostAntiAliasing::Smaa);
    written.save(file);
    CHECK(ViewerSettings::load(file).postAntiAliasing() == PostAntiAliasing::Smaa);
}

void testSettingsUnknownPostAntiAliasingIsSmaa()
{
    const fs::path file = freshFolder("settings_post_aa_unknown") / "settings.json";
    writeFile(file, R"({"postAntiAliasing": "taa"})");
    CHECK(ViewerSettings::load(file).postAntiAliasing() == PostAntiAliasing::Smaa);
}

void testLapRecordsRoundTrip()
{
    const std::string file = (freshFolder("laps_round_trip") / "laptimes.json").string();
    LapRecordBook written;
    written.at(3, 2) = LapRecord{61.25f, u"PLAYER1"};
    written.save(file);
    LapRecordBook read;
    read.load(file);
    CHECK(read.at(3, 2).time == 61.25f && read.at(3, 2).name == u"PLAYER1");
}

void testLapRecordsUnsetStayUnset()
{
    const std::string file = (freshFolder("laps_unset") / "laptimes.json").string();
    LapRecordBook written;
    written.at(0, 0) = LapRecord{50.f, u"A"};
    written.save(file);
    LapRecordBook read;
    read.load(file);
    CHECK(!read.at(17, 4).set());
}

void testLapRecordsSkipOutOfRange()
{
    const fs::path file = freshFolder("laps_range") / "laptimes.json";
    writeFile(file, R"({"lapRecords": [{"track": 18, "speed": 0, "time": 40, "name": "X"}]})");
    LapRecordBook read;
    read.load(file.string());
    CHECK(!read.at(17, 0).set());
}

void testLapRecordsMissingFileIsEmpty()
{
    LapRecordBook read;
    read.load((freshFolder("laps_missing") / "laptimes.json").string());
    CHECK(!read.at(0, 0).set());
}

void testProfileBestLapsRoundTrip()
{
    ProfileBestLaps written;
    written.set(17, 4, 72.5f);
    CHECK(ProfileBestLaps::read(written.text()).at(17, 4) == 72.5f);
}

void testProfileBestLapsShortTextLeavesTheRestUnset()
{
    const ProfileBestLaps read = ProfileBestLaps::read("40.000,41.000,");
    CHECK(read.at(0, 1) == 41.f && read.at(0, 2) == kUnsetLap);
}

void testProfileBestLapsAreWrittenAsTheProfilesAlwaysWere()
{
    ProfileBestLaps written;
    written.set(0, 0, 55.8159f);
    CHECK(written.text().rfind("55.816,999999.000,", 0) == 0);
}

argon::Profile sampleProfile(const fs::path &file)
{
    argon::Profile p;
    p.file = file.string();
    p.name = u"Kiri \"K\"";
    p.characters = 0x00ff;
    p.tracks = 0x1234abcd;
    p.mars = true;
    p.speeds = 3;
    p.eggs = 0x81;
    p.cheatMenu = true;
    p.playlist = "sound\\track|other";
    p.options = "8=7;15=1;";
    p.cash = "1,2,3";
    return p;
}

bool sameProfile(const argon::Profile &a, const argon::Profile &b)
{
    return a.name == b.name && a.characters == b.characters && a.tracks == b.tracks && a.mars == b.mars && a.speeds == b.speeds &&
           a.eggs == b.eggs && a.cheatMenu == b.cheatMenu && a.playlist == b.playlist && a.options == b.options && a.cash == b.cash;
}

void testProfileRoundTrip()
{
    const argon::Profile written = sampleProfile(freshFolder("profile_round") / "kiri.json");
    argon::writeProfileFile(written);
    CHECK(sameProfile(argon::readProfileFile(written.file), written));
}

// the hand-written writer's format (before saves\profile_file.cpp): masks as "0x..." strings, speeds a number
void testProfileReadsTheOldFormat()
{
    const fs::path file = freshFolder("profile_old") / "old.json";
    writeFile(file, "{\n  \"name\": \"Max\",\n  \"characters\": \"0x007f\",\n  \"tracks\": \"0x0000007f\",\n  \"mars\": false,\n"
                    "  \"speeds\": 2,\n  \"percent\": 4,\n  \"redshiftWins\": \"0x3\"\n}\n");
    const argon::Profile p = argon::readProfileFile(file);
    CHECK(p.valid && p.name == u"Max" && p.characters == 0x7f && p.speeds == 2 && p.redshiftWins == 3);
}

void testABrokenProfileIsInvalid()
{
    const fs::path file = freshFolder("profile_broken") / "broken.json";
    writeFile(file, "{ \"name\": ");
    CHECK(!argon::readProfileFile(file).valid);
}

void testACommaListWritesEachValueWithAComma()
{
    const int values[3] = {4, -1, 0};
    CHECK(argon::commaList(values) == "4,-1,0,");
}

void testAShortCommaListReadsZerosAfterItsEnd()
{
    int values[4] = {9, 9, 9, 9};
    argon::readCommaList("3,5", values);
    CHECK(values[0] == 3 && values[1] == 5 && values[2] == 0 && values[3] == 0);
}

void testTrophiesRoundTripThroughTheirHexText()
{
    argon::TrophyTable trophies{};
    trophies[0][0] = trophies[6][3] = trophies[15][4] = true;
    const std::string text = argon::trophyText(trophies);
    CHECK(text.size() == 24 && argon::readTrophies(text) == trophies);
}

void testTrophyTextOfAnotherLengthReadsAsNone()
{
    CHECK(argon::readTrophies("ff") == argon::TrophyTable{});
}

void testFlagsRoundTripThroughTheirMask()
{
    const bool flags[4] = {true, false, false, true};
    bool back[4]{};
    argon::readFlagMask(argon::flagMask(flags), back);
    CHECK(argon::flagMask(flags) == 0x9 && back[0] && !back[1] && back[3]);
}

void testANewProfileGetsAFileNamedAfterIt()
{
    argon::ProfileStore store;
    store.setFolder((freshFolder("store_new") / "profiles").string());
    argon::Profile profile;
    profile.name = u"Ana B";
    const bool saved = store.save(profile);
    CHECK(saved && fs::path(profile.file).filename() == "Ana_B.json" && store.writes() == 1);
}

void testASecondProfileOfTheSameNameGetsANumberedFile()
{
    argon::ProfileStore store;
    store.setFolder((freshFolder("store_twice") / "profiles").string());
    argon::Profile first, second;
    first.name = second.name = u"Ana";
    store.save(first);
    store.save(second);
    CHECK(fs::path(second.file).filename() == "Ana_2.json");
}

void testTheStoreFindsTheProfileInUse()
{
    argon::ProfileStore store;
    store.setFolder((freshFolder("store_current") / "profiles").string());
    argon::Profile profile;
    profile.name = u"Max";
    store.save(profile);
    store.load();
    const bool noneYet = !store.current().has_value();
    store.use(u"Max");
    CHECK(noneYet && store.current() && store.current()->get().name == u"Max");
}

void testRemovingAProfileDeletesItsFile()
{
    argon::ProfileStore store;
    store.setFolder((freshFolder("store_remove") / "profiles").string());
    argon::Profile profile;
    profile.name = u"Gone";
    store.save(profile);
    store.load();
    store.remove(0);
    CHECK(store.all().empty() && !fs::exists(profile.file));
}

argon::GameProgress sampleProgress()
{
    argon::GameProgress progress;
    progress.characters = 0x01ff;
    progress.tracks = 0x00ff00ffu;
    progress.mars = false;
    progress.speeds = 2;
    progress.tourneyLevel = 3;
    progress.tourney[2][1] = 4;
    progress.tourney[15][4] = -1;
    progress.trophies[2][1] = true;
    progress.redshiftWins = 5;
    progress.cash[3] = 125000;
    progress.upgrades[3][2] = 6;
    progress.hiTourney[1][17] = 9000;
    progress.hiQuick[0][0] = 42;
    progress.eggs[2] = progress.eggsOn[2] = true;
    progress.cheats[4] = true;
    progress.cheatMenu = true;
    return progress;
}

bool sameProgress(const argon::GameProgress &a, const argon::GameProgress &b)
{
    return a.characters == b.characters && a.tracks == b.tracks && a.mars == b.mars && a.speeds == b.speeds &&
           a.tourneyLevel == b.tourneyLevel && a.tourney[2][1] == b.tourney[2][1] && a.tourney[15][4] == b.tourney[15][4] &&
           a.trophies[2][1] == b.trophies[2][1] && a.redshiftWins == b.redshiftWins && a.cash[3] == b.cash[3] &&
           a.upgrades[3][2] == b.upgrades[3][2] && a.hiTourney[1][17] == b.hiTourney[1][17] && a.hiQuick[0][0] == b.hiQuick[0][0] &&
           a.eggs[2] == b.eggs[2] && a.eggsOn[2] == b.eggsOn[2] && a.cheats[4] == b.cheats[4] && a.cheatMenu == b.cheatMenu;
}

void testTheProgressRoundTripsThroughTheProfile()
{
    argon::Profile profile;
    argon::writeProgress(sampleProgress(), argon::ProfileUnlocks::Kept, profile);
    argon::GameProgress back;
    argon::readProgress(profile, argon::ProfileUnlocks::Kept, back);
    CHECK(sameProgress(back, sampleProgress()));
}

void testUnlockAllLeavesTheProfilesUnlocks()
{
    argon::Profile profile;
    profile.characters = 0x007f;
    argon::writeProgress(sampleProgress(), argon::ProfileUnlocks::Ignored, profile);
    CHECK(profile.characters == 0x007f && profile.eggs == 0 && profile.eggsOn == 0x4 && profile.cash.rfind("0,0,0,125000,", 0) == 0);
}

void testAnActiveCheatLeavesItsUnlocksInTheProfile()
{
    argon::GameProgress progress = sampleProgress();
    progress.cheatsOn[0] = progress.cheatsOn[2] = true;
    argon::Profile profile;
    argon::writeProgress(progress, argon::ProfileUnlocks::Ignored, profile);
    CHECK(profile.characters == 0xffff && profile.tracks == 0xffffffffu && profile.mars);
}

void testReadingClampsTheUpgradeLevels()
{
    argon::Profile profile;
    profile.upgrades = "9,-2,3";
    argon::GameProgress progress;
    argon::readProgress(profile, argon::ProfileUnlocks::Kept, progress);
    CHECK(progress.upgrades[0][0] == 7 && progress.upgrades[0][1] == 0 && progress.upgrades[0][2] == 3);
}
} // namespace

int main()
{
    testSettingsDefaultWhenMissing();
    testSettingsFileWrittenWhenMissing();
    testSettingsReadsLanguageAndIntro();
    testSettingsLocaleFolder();
    testSettingsKeyBindingsRoundTrip();
    testSettingsKeyBindingsLeftOutKeepTheirDefaults();
    testSettingsReservedKeyIsNotBound();
    testSettingsSpanishIsEnglish();
    testSettingsUnknownLanguageIsEnglish();
    testSettingsBrokenFileUsesDefaults();
    testSettingsReadsTheHudSize();
    testSettingsHudSizeOfNoStepIsFull();
    testSettingsHudSizeRoundTrips();
    testSettingsResolutionRoundTrips();
    testSettingsUnreadableResolutionIsNone();
    testSettingsMissingResolutionIsNone();
    testSettingsFullscreenIsOnByDefault();
    testSettingsFullscreenRoundTrips();
    testSettingsVsyncIsOnByDefault();
    testSettingsReadsVsyncOff();
    testSettingsFpsMeterIsOffByDefault();
    testSettingsFpsMeterRoundTrips();
    testSettingsWidescreenIsOnByDefault();
    testSettingsWidescreenRoundTrips();
    testSettingsSpeedBlurIsOnByDefault();
    testSettingsSpeedBlurRoundTrips();
    testSettingsMarsSpeedBlurFixIsOnByDefault();
    testSettingsMarsSpeedBlurFixRoundTrips();
    testSettingsKemarTapsAreSixtyFourByDefault();
    testSettingsKemarTapsRoundTrip();
    testSettingsKemarTapsBetweenChoicesTakeTheOneBelow();
    testSettingsMsaaIsEightByDefault();
    testSettingsLanguageRoundTrips();
    testSettingsRendererRoundTrips();
    testSettingsMsaaRoundTrips();
    testSettingsMsaaBetweenChoicesTakesTheOneBelow();
    testSettingsPostAntiAliasingIsSmaaByDefault();
    testSettingsPostAntiAliasingRoundTrips();
    testSettingsUnknownPostAntiAliasingIsSmaa();
    testLapRecordsRoundTrip();
    testLapRecordsUnsetStayUnset();
    testLapRecordsSkipOutOfRange();
    testLapRecordsMissingFileIsEmpty();
    testProfileBestLapsRoundTrip();
    testProfileBestLapsShortTextLeavesTheRestUnset();
    testProfileBestLapsAreWrittenAsTheProfilesAlwaysWere();
    testProfileRoundTrip();
    testProfileReadsTheOldFormat();
    testABrokenProfileIsInvalid();
    testACommaListWritesEachValueWithAComma();
    testAShortCommaListReadsZerosAfterItsEnd();
    testTrophiesRoundTripThroughTheirHexText();
    testTrophyTextOfAnotherLengthReadsAsNone();
    testFlagsRoundTripThroughTheirMask();
    testANewProfileGetsAFileNamedAfterIt();
    testASecondProfileOfTheSameNameGetsANumberedFile();
    testTheStoreFindsTheProfileInUse();
    testRemovingAProfileDeletesItsFile();
    testTheProgressRoundTripsThroughTheProfile();
    testUnlockAllLeavesTheProfilesUnlocks();
    testAnActiveCheatLeavesItsUnlocksInTheProfile();
    testReadingClampsTheUpgradeLevels();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
