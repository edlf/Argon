#include "launch/startup.hpp"
#include "launch/test_start.hpp"

#include "app/app.hpp"
#include "files/game_path.hpp"
#include "platform/platform.hpp"

namespace
{

// the game's files and the folder for the viewer's saves: a project folder holding game_data\ (the saves in save_data\
// beside it), or a plain game folder (the saves in its save_data\)
struct GameFolders
{
    std::string game, saves;
    bool found() const
    {
        return !game.empty();
    }
};

bool holdsGame(const fs::path &folder)
{
    return argon::gamePathExists(folder / "data" / "tracks");
}

GameFolders gameFoldersAt(const fs::path &folder)
{
    if (holdsGame(folder / "game_data"))
    {
        return {(folder / "game_data").string(), (folder / "save_data").string()};
    }
    if (holdsGame(folder))
    {
        return {folder.string(), (folder / "save_data").string()};
    }
    return {};
}

GameFolders searchUpFrom(fs::path start)
{
    for (fs::path folder = start; !folder.empty(); folder = folder.parent_path())
    {
        GameFolders folders = gameFoldersAt(folder);
        if (folders.found() || folder == folder.root_path())
        {
            return folders;
        }
    }
    return {};
}

GameFolders findGame(const std::string &hint)
{
    if (!hint.empty())
    {
        return gameFoldersAt(hint);
    }
    GameFolders folders = searchUpFrom(platform::executablePath().parent_path());
    return folders.found() ? folders : searchUpFrom(fs::current_path());
}

void glfwError(int code, const char *msg)
{
    std::fprintf(stderr, "glfw %d: %s\n", code, msg);
}
} // namespace

ViewerStartup::ViewerStartup(App &app, LaunchOptions options) : app_(app), options_(std::move(options)), testRuns_(app, app.testRunSources(), options_)
{
}

bool ViewerStartup::locateGame()
{
    const GameFolders folders = findGame(options_.gameHint);
    if (!folders.found())
    {
        std::fprintf(stderr, "Could not find the game files (game_data\\data\\tracks or data\\tracks). Use --game <folder>.\n");
        platform::showErrorDialog("Argon", "Could not find the game files (game_data\\data\\tracks or data\\tracks).\nRun with --game <folder>.");
        return false;
    }
    app_.useGameFolders(folders.game, folders.saves);
    return true;
}

void ViewerStartup::loadGameLists()
{
    app_.loadGameLists(options_.pilot);
}

// the viewer's settings.json next to the profiles (written with the defaults the first time)
void ViewerStartup::loadSettings()
{
    app_.loadViewerSettings();
}

// screenshot runs stay silent unless --sound
void ViewerStartup::initAudio()
{
    if (app_.screenshotRun() && !options_.forceSound)
    {
        app_.silenceAudio();
    }
    app_.startAudio();
}

void ViewerStartup::openWindow()
{
    glfwSetErrorCallback(glfwError);
    if (!glfwInit())
    {
        throw std::runtime_error("glfwInit failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    app_.openWindow(options_.windowSize);
}

void ViewerStartup::initRenderer()
{
    app_.initRenderer();
    app_.startUi();
}

// ------------------------------------------------------------------ where the viewer starts
// the menu starts without a track: the editor / race loads one when it is picked. A test race (--quick-race, --replay,
// --tourney-race) is started through the menus, so it starts there unless --editor says otherwise
bool ViewerStartup::startsInMenu() const
{
    if (!ARGON_DEVELOP)
    {
        return true;   // the editor is the developer build's
    }
    const MenuStart &menuStart = options_.menu;
    if (menuStart.mode >= 0)
    {
        return menuStart.mode == 1;
    }
    if (options_.raceTest.startsARace())
    {
        return true;
    }
    return options_.startTrack.empty() && !options_.drive.enabled && !app_.screenshotRun();
}

void ViewerStartup::enterStartScene()
{
    const bool menu = startsInMenu();
    app_.rememberEditorTrack(options_.startTrack.empty() ? std::string("bracken") : options_.startTrack);
    if (!menu)
    {
        app_.openEditorScene();
        app_.pickEditorTrack(options_.pickTrack);   // --pick-track: as the editor's track list does
    }
    placeCamera();
    if (ARGON_DEVELOP)
    {
        app_.printTrackSummary();
    }
    app_.applyUnlockAll();
    if (!options_.settings.empty())
    {
        app_.loadSettingsString(options_.settings);
    }
    if (menu)
    {
        TestStart(app_.testStartSources(testRuns_), app_, options_).open();
    }
}

void ViewerStartup::placeCamera()
{
    app_.placeShotCamera();
    if (options_.camera)
    {
        app_.placeFreeCamera(options_.camera->position, options_.camera->yawDeg, options_.camera->pitchDeg);
    }
    if (options_.lookAt)
    {
        app_.aimFreeCameraAt(*options_.lookAt);
    }
}
