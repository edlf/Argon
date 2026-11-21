// Argon: the front end's stores (app\front_end_stores.hpp): ghosts (saves\ghost_library.hpp), lap records
// (logic\race_lap_records.hpp), custom music (audio\custom_music.hpp) and profiles (saves\profile_store.hpp;
// docs\frontend\profiles.md "Profiles"). The pages: ui\ghostselect.cpp, ui\records.cpp, ui\playlist.cpp,
// ui\choosesave.cpp (FE CHOOSESAVE), ui\keyboard.cpp (FE NAME ENTRY).
#include "app/front_end_stores.hpp"

#include <algorithm>

#include <imgui.h>

#include "app/screenshot_run.hpp"
#include "audio/custom_music.hpp"
#include "craft/craftsim.hpp"
#include "launch/test_script.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/race_lap_records.hpp"
#include "saves/ghost_library.hpp"
#include "saves/profile_fields.hpp"
#include "saves/profile_store.hpp"
#include "track/course_folder.hpp"
#include "ui/game_strings.hpp"

// <track>_<pilot>.ghost
bool GhostShelf::saveGhost(const ghost::Lap &lap)
{
    return ghosts_.save(lap, courseFolder(lap.track) + "_" + argon::pilotFolder(std::clamp(lap.pilot, 0, 15)) + ".ghost");
}

// a ghost's name in the lists: its character (FE GHOSTSELECT 0x24c90: infotext 200 + the character)
std::u16string GhostShelf::ghostLabel(const SavedGhost &ghost) const
{
    return strings_.info(200 + std::clamp(ghost.pilot, 0, 15), argon::pilotName(ghost.pilot));
}

void LapRecordSaver::storeLapRecords()
{
    if (!shot_.active())
    {
        records_.save();
    }
}

void CustomMusicStore::scanCustomMusic()
{
    music_.refresh(ImGui::GetCurrentContext() ? ImGui::GetTime() : 0.0);
}

void CustomMusicStore::storeMusic()
{
    if (const auto current = profiles_.current())
    {
        argon::Profile &profile = *current;
        profile.musicCustom = music_.customChosen();
        profile.playlist = music_.playlistString();
        profiles_.save(profile);
    }
}

// FE NAME ENTRY done (0x3a110): the profile reset (0x83b60: characters 0x7f, tracks 0x7f, Mars off, 1 speed, best
// laps 999999 each); a name starting "CHEAT" (fe 0x4b1, 5 characters, case-sensitive) opens the cheat menu
// (0x83af0: 0x1bbc26)
bool ProfileSwitcher::createProfile(const std::u16string &name)
{
    argon::Profile profile;
    profile.name = name;
    const std::u16string cheat = sources_.strings.frontEnd(0x4b1, "CHEAT");
    profile.cheatMenu = name.compare(0, cheat.size(), cheat) == 0;
    if (!sources_.profiles.save(profile))
    {
        return false;
    }
    useProfile(profile);
    return true;
}

void ProfileSwitcher::deleteProfile(size_t index)
{
    sources_.profiles.remove(index);
}

// the chosen profile's unlocks drive the character and track pages (0x1bb0e0, 0x1bbbfc, 0x1bbc00);
// --unlock-all keeps everything open
void ProfileSwitcher::useProfile(const argon::Profile &profile)
{
    sources_.profiles.use(profile.name);
    sources_.settings.loadSettingsString(profile.options);   // its Game / Sound Settings
    argon::readProgress(profile, sources_.progressKeeper.profileUnlocks(), sources_.progress);
    argon::readFlagMask(profile.eggsOn | sources_.tests.eggs, sources_.progress.eggsOn);   // --egg (tests)
    sources_.musicStore.scanCustomMusic();
    sources_.music.readPlaylistString(profile.playlist);
    sources_.music.chooseCustom(profile.musicCustom && !sources_.music.playlist().empty());   // no songs found: Default (0xbf710)
    sources_.lapRecords.readBestLaps(profile);
    sources_.progressKeeper.applyUnlockAll();
}
