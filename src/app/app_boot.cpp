// Argon: App's start-up work (launch\startup.cpp decides it): the save folders its stores keep their files in, the
// game's track and craft lists, its audio, and the start track's summary.
#include "app/app.hpp"
#include "files/game_path.hpp"
#include "platform/platform.hpp"

#include <algorithm>

namespace
{
std::vector<std::string> modelFolders(const fs::path &dir)
{
    std::vector<std::string> names;
    for (auto &entry : fs::directory_iterator(argon::findGamePath(dir)))
    {
        if (argon::gamePathExists(entry.path() / "model.cmm"))
        {
            names.push_back(entry.path().filename().string());
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

// MIT KEMAR's tables: hrtf_kemar.bin in <project> (beside game_data and save_data), else beside the executable; the
// project's when neither has it
fs::path kemarFileFor(const std::string &saves)
{
    const fs::path inProject = argon::findGamePath(fs::path(saves).parent_path() / "hrtf_kemar.bin");
    const fs::path besideExecutable = platform::executablePath().parent_path() / "hrtf_kemar.bin";
    return !argon::gamePathExists(inProject) && argon::gamePathExists(besideExecutable) ? besideExecutable : inProject;
}
}  // namespace

void App::useGameFolders(const std::string &gameFolder, const std::string &savesFolder)
{
    game = gameFolder;
    saves = savesFolder;
    profiles.setFolder(saves + "/profiles");
    ghosts.setFolder(fs::path(saves) / "ghosts");
    customMusic.setFolder(fs::path(saves) / "custom_music");
}

void App::loadGameLists(const std::string &pilot)
{
    trackList = modelFolders(fs::path(game) / "data" / "tracks");
    startGrid.pilots = modelFolders(fs::path(game) / "data" / "craft");
    for (int i = 0; i < int(startGrid.pilots.size()); ++i)
    {
        if (startGrid.pilots[i] == pilot)
        {
            startGrid.playerPilot = i;
        }
    }
    try
    {
        craftStats.load(game + "/config/character_stats.txt");
    }
    catch (const std::runtime_error &e)
    {
        std::fprintf(stderr, "craft stats: %s\n", e.what());
    }
}

// screenshot runs stay silent (silenceAudio, unless --sound)
void App::startAudio()
{
    if (!audioEnabled)
    {
        return;
    }
    const argon::mix::SpeakerLayout speakers = viewerSettings.surroundSound() ? argon::mix::SpeakerLayout::Surround : argon::mix::SpeakerLayout::Stereo;
    try
    {
        audio.init(speakers);
    }
    catch (const std::runtime_error &e)
    {
        std::fprintf(stderr, "audio: %s\n", e.what());
    }
    audio.setKemarFile(kemarFileFor(saves));
    audio.setHrtfSet(viewerSettings.hrtf());
    audio.setKemarTaps(viewerSettings.kemarTaps());
}

void App::printTrackSummary() const
{
    for (auto &warning : track.warnings)
    {
        std::fprintf(stderr, "warning: %s\n", warning.c_str());
    }
    std::printf("track %s: %zu draws, %zu tris, %zu textures, %.2fs\n", track.name.c_str(), track.draws.size(), track.triangles, track.textures.size(), track.loadSeconds);
    std::printf("audio: %s, %zu emitters, %zu one-shots, engine %s %d layers, %zu ships, %zu pickup models\n",
                audio.ok() ? "on" : "off",
                audio.emitters().size(),
                audio.oneShots().size(),
                enginePilot.c_str(),
                audio.engineLayers(),
                props.craftFolders().size(),
                props.pickupFolders().size());
    std::printf("route: %s\n", routeReport.summary().c_str());
    std::printf("viability: %s\n", viabilityText().c_str());
    for (const argon::RouteIssue &issue : routeReport.issues())
    {
        std::printf("  %s  [%.0f-%.0f m, at %.0f %.0f %.0f]\n", issue.text().c_str(), issue.from, issue.to, issue.pos.x, issue.pos.y, issue.pos.z);
    }
}

// screenshot runs stay silent unless --sound: before startAudio
void App::silenceAudio()
{
    audioEnabled = false;
}

void App::playCountdown()
{
    audio.playCountdown();
}

// the track list's index of a track folder (the last one of that name), -1 when it has none
int App::trackListIndex(const std::string &trackName) const
{
    int index = -1;
    for (int i = 0; i < int(trackList.size()); ++i)
    {
        if (trackList[size_t(i)] == trackName)
        {
            index = i;
        }
    }
    return index;
}

// the editor's track when Game Demos opens it (the list's first when the folder is not there)
void App::rememberEditorTrack(const std::string &trackName)
{
    editor.rememberTrack(std::max(0, trackListIndex(trackName)));
}

// the editor on the remembered track, with its weather, at start-up
void App::openEditorScene()
{
    enterEditorMode();
    loadTrack(editor.lastTrack());
    startEditorWeather(editor.lastTrack());
}

// as the editor's track list picks one: loaded behind its loading screen on the next frame
void App::pickEditorTrack(const std::string &trackName)
{
    const int index = trackListIndex(trackName);
    if (index >= 0)
    {
        pendingTrack = index;
    }
}

void App::applyUnlockAll()
{
    progressKeeper.applyUnlockAll();
}

void App::loadSettingsString(const std::string &settings)
{
    settingsKeeper.loadSettingsString(settings);
}

// a --shot run from the top: the camera over the track, no fog
void App::placeShotCamera()
{
    if (screenshotRun() && shotRun.fromTop())
    {
        cameraToTop();
        toggles.fog = false;
    }
}

void App::placeFreeCamera(const glm::vec3 &position, float yawDeg, float pitchDeg)
{
    freeCamera.position = position;
    freeCamera.yaw = glm::radians(yawDeg);
    freeCamera.pitch = glm::radians(pitchDeg);
}

void App::aimFreeCameraAt(const glm::vec3 &target)
{
    freeCamera.lookAtFrom(target, glm::vec3(22, 14, 22));
}
