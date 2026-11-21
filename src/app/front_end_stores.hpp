// Argon: what the front end's pages save and load through (ui\page_hosts.hpp's stores), each over the parts it keeps:
// the ghosts (GhostShelf), the lap records (LapRecordSaver), the custom music's choice (CustomMusicStore) and the
// profiles (ProfileSwitcher: a new one, one deleted, one used). App owns one of each and hands them to the pages.
#pragma once
#include <string>

#include "ui/page_hosts.hpp"

class CustomMusic;
class GameStrings;
class GhostLibrary;
class ProgressKeeper;
class RaceLapRecords;
class ScreenshotRun;
struct TestScript;
namespace argon
{
class ProfileStore;
struct GameProgress;
}

// a ghost saved as its character's on its track, and its name in the lists
class GhostShelf final : public GhostKeeper
{
  public:
    GhostShelf(GhostLibrary &ghosts, const GameStrings &strings) : ghosts_(ghosts), strings_(strings)
    {
    }
    bool saveGhost(const ghost::Lap &lap) override;
    std::u16string ghostLabel(const SavedGhost &ghost) const override;

  private:
    GhostLibrary &ghosts_;
    const GameStrings &strings_;
};

// the lap records written back; a --shot run writes no saves
class LapRecordSaver final : public LapRecordStore
{
  public:
    LapRecordSaver(RaceLapRecords &records, const ScreenshotRun &shot) : records_(records), shot_(shot)
    {
    }
    void storeLapRecords() override;

  private:
    RaceLapRecords &records_;
    const ScreenshotRun &shot_;
};

// the custom music folder read again, and its choice and playlist written into the profile in use
class CustomMusicStore final : public MusicStore
{
  public:
    CustomMusicStore(CustomMusic &music, argon::ProfileStore &profiles) : music_(music), profiles_(profiles)
    {
    }
    void scanCustomMusic() override;
    void storeMusic() override;

  private:
    CustomMusic &music_;
    argon::ProfileStore &profiles_;
};

struct ProfileSwitcherSources
{
    argon::ProfileStore &profiles;
    const GameStrings &strings;          // the cheat menu's "CHEAT" name
    SettingsStore &settings;             // the profile's Game / Sound Settings
    argon::GameProgress &progress;
    ProgressKeeper &progressKeeper;      // the profile's unlocks, --unlock-all
    const TestScript &tests;             // --egg: eggs on whatever the profile says
    CustomMusic &music;                  // its playlist and Music Source
    MusicStore &musicStore;              // the folder read again first
    RaceLapRecords &lapRecords;          // its best laps
};

class ProfileSwitcher final : public ProfileKeeper
{
  public:
    explicit ProfileSwitcher(const ProfileSwitcherSources &sources) : sources_(sources)
    {
    }
    bool createProfile(const std::u16string &name) override;
    void deleteProfile(size_t index) override;
    void useProfile(const argon::Profile &profile) override;

  private:
    ProfileSwitcherSources sources_;
};
