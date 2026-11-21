// Argon: tests for the front end's stores (app/front_end_stores.hpp: a profile made, used and deleted, the cheat menu's
// name, the tests' eggs through a profile's load, the lap records not written by a screenshot run). Each test writes
// to its own folder under the system's temporary folder and removes it. Run with test.bat / test.sh.
#include "app/front_end_stores.hpp"
#include "check.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

#include "app/screenshot_run.hpp"
#include "audio/custom_music.hpp"
#include "launch/test_script.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/race_lap_records.hpp"
#include "saves/profile_store.hpp"
#include "ui/game_strings.hpp"

namespace
{
using argon::test::checks;
using argon::test::failures;

namespace fs = std::filesystem;

constexpr int kAlwaysRain = 4;

// what a profile's use asks of the settings: only the options string is kept
class RecordedSettings final : public SettingsStore
{
  public:
    std::string loaded;

    void loadSettingsString(const std::string &options) override
    {
        loaded = options;
    }
    void buildSettings() override
    {
    }
    void buildArgonSettings() override
    {
    }
    void changeSetting(int, int) override
    {
    }
    void storeSettings() override
    {
    }
    void storeArgonSettings() override
    {
    }
    bool chosenRendererNeedsRestart() const override
    {
        return false;
    }
    bool easterEggsShown() const override
    {
        return false;
    }
    bool cheatsShown() const override
    {
        return false;
    }
};

// the stores over a save folder of their own
struct Saves
{
    explicit Saves(const std::string &name) : folder((fs::temp_directory_path() / ("argon_front_end_stores_" + name)).string())
    {
        fs::remove_all(folder);
        profiles.setFolder(folder + "/profiles");
    }
    ~Saves()
    {
        std::error_code ignored;
        fs::remove_all(folder, ignored);
    }
    Saves(const Saves &) = delete;
    Saves &operator=(const Saves &) = delete;

    std::string folder;
    argon::ProfileStore profiles;
    GameStrings strings;
    RecordedSettings settings;
    argon::GameProgress progress;
    ProgressKeeper progressKeeper{progress, profiles};
    TestScript tests;
    CustomMusic music;
    CustomMusicStore musicStore{music, profiles};
    RaceLapRecords lapRecords{profiles, strings, folder};
    ProfileSwitcher switcher{ProfileSwitcherSources{profiles, strings, settings, progress, progressKeeper, tests, music, musicStore, lapRecords}};
};

bool isInUse(const argon::ProfileStore &profiles, const std::u16string &name)
{
    return profiles.currentName() == name;
}

// the profiles as the folder holds them
size_t savedProfiles(argon::ProfileStore &profiles)
{
    profiles.load();
    return profiles.all().size();
}

void testANewProfileIsSavedAndUsed()
{
    Saves saves("new");
    const bool made = saves.switcher.createProfile(u"Ada");
    CHECK(made && isInUse(saves.profiles, u"Ada") && savedProfiles(saves.profiles) == 1);
}

void testANameStartingCheatOpensTheCheatMenu()
{
    Saves saves("cheat");
    saves.switcher.createProfile(u"CHEATER");
    CHECK(saves.progress.cheatMenu);
}

void testAnOrdinaryNameLeavesTheCheatMenuShut()
{
    Saves saves("ordinary");
    saves.switcher.createProfile(u"Ada");
    CHECK(!saves.progress.cheatMenu);
}

void testAProfilesUseLoadsItsSettings()
{
    Saves saves("settings");
    argon::Profile profile;
    profile.name = u"Ada";
    profile.options = "0=1;1=2";
    saves.switcher.useProfile(profile);
    CHECK(saves.settings.loaded == "0=1;1=2");
}

void testTheTestsEggsStayOnThroughAProfilesLoad()
{
    Saves saves("eggs");
    saves.tests.eggs = 1u << kAlwaysRain;
    argon::Profile profile;
    profile.name = u"Ada";
    profile.eggsOn = 0;
    saves.switcher.useProfile(profile);
    CHECK(saves.progress.eggsOn[kAlwaysRain]);
}

void testADeletedProfileLeavesTheList()
{
    Saves saves("delete");
    saves.switcher.createProfile(u"Ada");
    savedProfiles(saves.profiles);
    saves.switcher.deleteProfile(0);
    CHECK(savedProfiles(saves.profiles) == 0);
}

void testAScreenshotRunWritesNoLapRecords()
{
    Saves saves("shot");
    ScreenshotRun shot;
    shot.use(ShotRequest{"shot.png", "", std::nullopt});
    LapRecordSaver saver(saves.lapRecords, shot);
    saver.storeLapRecords();
    CHECK(!fs::exists(saves.lapRecords.file()));
}
}  // namespace

int main()
{
    testANewProfileIsSavedAndUsed();
    testANameStartingCheatOpensTheCheatMenu();
    testAnOrdinaryNameLeavesTheCheatMenuShut();
    testAProfilesUseLoadsItsSettings();
    testTheTestsEggsStayOnThroughAProfilesLoad();
    testADeletedProfileLeavesTheList();
    testAScreenshotRunWritesNoLapRecords();
    std::printf("front_end_stores_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
