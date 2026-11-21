// Argon: what the front-end pages ask of the game around them, each a small interface App implements (pages get only
// the ones they use, with the state they read, in their own Services: ui\fe_page.hpp's PageKit has what every page
// shares).
#pragma once
#include "input/key_bindings.hpp"
#include "logic/profile_progress.hpp"
#include "saves/profile_file.hpp"

#include <string>

struct SavedGhost;
namespace ghost
{
struct Lap;
}

// the front end over the race and the editor (ui\menu.cpp)
class MenuDoor
{
  public:
    virtual void openMenu() = 0;      // the shell open on FE MAIN, the race or editor left
    virtual void closeMenu() = 0;     // back to the race or editor
    virtual void quit() = 0;          // the window closes (run() saves the profile)
#if ARGON_DEVELOP
    virtual void startEditor() = 0;   // the viewer's editor on the last track
#endif

  protected:
    ~MenuDoor() = default;
};

// the profile's progress written back, and the cheats that change it (logic\progress_keeper.hpp)
class ProgressStore
{
  public:
    virtual void storeProgress() = 0;
    virtual void activateCheat(int cheat) = 0;   // cheats 0..4: for good, with their unlocks
    virtual void switchCheat(int cheat) = 0;     // cheats 5 / 6: Infinite Shield / Turbo on or off

  protected:
    ~ProgressStore() = default;
};

// FE NAME ENTRY's code: a cheat unlocked, the cheat menu (app\app_unlocks.cpp)
class CheatCodes
{
  public:
    virtual void cheatCodeDone(const std::u16string &code) = 0;

  protected:
    ~CheatCodes() = default;
};

// the lap record table written back, not in a --shot run (app\app_laprecords.cpp)
class LapRecordStore
{
  public:
    virtual void storeLapRecords() = 0;

  protected:
    ~LapRecordStore() = default;
};

// the profiles made, removed and used (app\app_profile.cpp)
class ProfileKeeper
{
  public:
    virtual bool createProfile(const std::u16string &name) = 0;   // saved and used; false when it cannot be written
    virtual void deleteProfile(size_t index) = 0;
    virtual void useProfile(const argon::Profile &profile) = 0;

  protected:
    ~ProfileKeeper() = default;
};

// custom music: the folder read again, the choice and playlist into the profile (app\app_music.cpp)
class MusicStore
{
  public:
    virtual void scanCustomMusic() = 0;   // at most once a second
    virtual void storeMusic() = 0;

  protected:
    ~MusicStore() = default;
};

// Time Attack's ghosts as the pages save and name them (app\app_ghosts.cpp)
class GhostKeeper
{
  public:
    virtual bool saveGhost(const ghost::Lap &lap) = 0;   // as its character's ghost on its track
    virtual std::u16string ghostLabel(const SavedGhost &ghost) const = 0;

  protected:
    ~GhostKeeper() = default;
};

// the races the front end starts (app\app_loader.cpp, app\app_tourney.cpp)
class RaceStarter
{
  public:
    virtual void startQuickRace(int pilotId, int trackId) = 0;
    virtual void startTourneyRace(int level) = 0;
    virtual void startMultiplayer(int players) = 0;   // game mode 4 with players 1..n

  protected:
    ~RaceStarter() = default;
};

// what the profile has opened (logic\progress_keeper.hpp)
class UnlockRules
{
  public:
    virtual int speedMax() const = 0;                               // the highest open speed
    virtual int tourneyDone(int level, int character) const = 0;    // the races won in a tournament, 0 when none or done
    virtual bool charUnlocked(int id) const = 0;
    virtual argon::ProfileProgress profileProgress() const = 0;    // the percentage parts

  protected:
    ~UnlockRules() = default;
};

// Game / Sound Settings and Argon Options: their rows built, changed, applied and stored (app\app_settings.cpp)
class SettingsStore
{
  public:
    virtual void loadSettingsString(const std::string &options) = 0;   // a profile's "options": its Game / Sound Settings
    virtual void buildSettings() = 0;
    virtual void buildArgonSettings() = 0;   // the rows from settings.json's values
    virtual void changeSetting(int type, int option) = 0;
    virtual void storeSettings() = 0;
    virtual void storeArgonSettings() = 0;   // the rows applied and written to settings.json
    virtual bool chosenRendererNeedsRestart() const = 0;
    virtual bool easterEggsShown() const = 0;   // FE OPTIONS' rows
    virtual bool cheatsShown() const = 0;

  protected:
    ~SettingsStore() = default;
};

// Key Bindings: the craft's keys as settings.json keeps them, and written there (app\settings_keeper.cpp)
class KeyBindingStore
{
  public:
    virtual const KeyBindings &keyBindings() const = 0;
    virtual void storeKeyBindings(const KeyBindings &bindings) = 0;

  protected:
    ~KeyBindingStore() = default;
};

// the loading screen's race load and flyby (app\app_loader.cpp, app\app_sequence.cpp, ui\menu.cpp, app\app_split.cpp)
class RaceLoading
{
  public:
    virtual bool beginRaceLoad() = 0;   // false when the track is not there
    virtual bool stepRaceLoad() = 0;    // a frame's slice; true once the race is ready
    virtual void finishQuickRace() = 0;
    virtual void startGlorySequence(const std::string &trackFolder) = 0;   // <t>_glory.nms behind the loading screen
    virtual void updateSequence() = 0;
    virtual void startTransition() = 0;

  protected:
    ~RaceLoading() = default;
};
