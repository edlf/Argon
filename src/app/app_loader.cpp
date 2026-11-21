// Argon: loading the race behind the loading screen (Quick Race: FE COURSE SELECT -> LOADER -> the
// race). The loading screen itself: ui\loader.cpp.

#include "app/app.hpp"
#include "logic/race_laps.hpp"
#include "ui/fe_shell.hpp"

// the editor's track pick (the race's loader without its page): read on a worker, committed a slice a frame behind
// EditorTrackLoad's screen; another pick waits for it
void App::startEditorLoad(int index)
{
    if (index < 0 || index >= int(trackList.size()))
    {
        loadTrack(index);   // clears the track
        return;
    }
    startEditorWeather(index);
    editorLoad.start(trackList[size_t(index)], trackArrival.use(), trackArrival.request(index));
}

void App::loadRaceNow()
{
    if (!beginRaceLoad())
    {
        return;
    }
    trackLoad->finish();
    trackLoad.reset();
    startLoadedRace();
}

// the player's pilot, then the track read on a worker thread (with the grid's craft and the loaded ghost's)
bool App::beginRaceLoad()
{
    std::string folder = argon::pilotFolder(picks.loadPilot);
    for (int i = 0; i < int(startGrid.pilots.size()); ++i)
    {
        if (startGrid.pilots[i] == folder)
        {
            startGrid.playerPilot = i;
        }
    }
    int idx = -1;
    for (int i = 0; i < int(trackList.size()); ++i)
    {
        if (trackList[i] == courseFolder(picks.loadTrackId))
        {
            idx = i;
        }
    }
    if (idx < 0)
    {
        return false;
    }
    switchOnRaceEggs();
    startRaceWeather(picks.loadTrackId);
    setRaceLaps();
    TrackRequest request = trackArrival.request(idx);
    const ghost::Lap &loaded = modes.timeAttack().loadedGhost();
    if (logic().hasGhosts() && loaded.valid())
    {
        request.extraCraft.push_back(argon::pilotFolder(std::clamp(loaded.pilot, 0, 15)));
    }
    trackLoad = std::make_unique<TrackLoad>(trackArrival.use(), std::move(request), std::launch::async);
    return true;
}

// 0x3b0b0 as each game mode sets the race up: g_race_laps (0x52cdd8) from the speed class, or the Multiplayer Laps
// option (0x10) in multiplayer (logic\race_laps.hpp)
void App::setRaceLaps()
{
    session.hudLaps = argon::race::raceLaps(playerPads.multiplayerRace(), settingsMenu.current(SettingsMenu::kLaps), picks.speedClass);
}

bool App::stepRaceLoad()
{
    if (!trackLoad->step(TrackLoad::kFrameSliceMs))
    {
        return false;
    }
    trackLoad.reset();
    startLoadedRace();
    return true;
}

void App::startLoadedRace()
{
    if (raceEggs.on(Egg::AtlantisMode))
    {
        useAtlantisFog(track.data.props);
    }
    audio.holdRaceSounds();   // the race's sounds hold until the race starts (the game plays levelload_music here)
    startGlorySequence(courseFolder(picks.loadTrackId));   // the flyby behind the loading screen (app\app_sequence.cpp)
}

// Atlantis Mode (track_load_prop_dat 0xb0453): the track's fog replaced by colour (0x30, 0x60, 0x40), density 0.005,
// mode 1
void App::useAtlantisFog(argon::TrackProps &props)
{
    props.fogColour[0] = 0x30 / 255.f;
    props.fogColour[1] = 0x60 / 255.f;
    props.fogColour[2] = 0x40 / 255.f;
    props.fogDensity = 0.005f;
    props.fogMode = 1;
}

// character, track and speed picked: the loading screen (LOADER, ui\loader.cpp) loads the track and starts the
// race on A
void App::startQuickRace(int pilotId, int trackId)
{
    picks.loadPilot = pilotId;
    picks.loadTrackId = trackId;
    fe->loader().open();
    startTransition();   // 0x80556: the front end fades out over the loading screen
}

void App::finishQuickRace()
{
    loadingFlyby.stop();
    leaveEditorMode();
    audio.resumeRaceSounds();   // the race's sounds, held since the load finished
    audio.playUi(5);   // SF_UI_GOTOGAME ui_xition3_LR
    startTransition();   // 0x97486: the loading screen fades out over the race
    closeMenu();
    showPanel = false;
    if (trackIndex < 0)
    {
        openMenu();
        return;
    }
    logic().nextLoaderHint();
    if (route.valid())
    {
        resetDriver();
        session.driving = true;
        session.chaseCam = true;
        session.playerDrive = true;
        if (logic().hasGridIntro())   // the first race after a load: the grid intro, the countdown
        {
            startGridIntro();
        }
        else
        {
            startRaceCountdown();
        }
    }
}

// 0x1b9538: the game's track ids (FE COURSE SELECT picks 0..7, + 8 for the second route, Mars 0x10 / 0x11)
bool App::onMarsTrack() const
{
    constexpr int kMars = 0x10, kTerraformedMars = 0x11;
    const int id = courseId(track.name);
    return id == kMars || id == kTerraformedMars;
}

// the game's track id for a track folder, -1 for a folder that is no course
int App::courseId(const std::string &folder) const
{
    for (int id = 0; id < 18; ++id)
    {
        if (courseFolder(id) == folder)
        {
            return id;
        }
    }
    return -1;
}
