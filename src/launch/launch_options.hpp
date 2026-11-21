// The command line, read into LaunchOptions: what the viewer itself is set to (ViewerLaunch, which App takes once
// after parsing: App::useLaunchSettings) and what to do at start-up (ViewerStartup). Only the developer build
// (ARGON_DEVELOP) reads them all; otherwise only --game.
//
//   argon [--game <game folder>] [--track bracken] [--validate]
//            [--shot out.png [--cam start|top] [--frames N]] [--drive] [--drive-time seconds]
#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "gfx/backend_kind.hpp"
#include "launch/test_script.hpp"
#include "render/post_anti_aliasing_mode.hpp"
#include "window/resolution.hpp"

// the renderer as asked (else settings.json's)
struct RendererRequest
{
    std::optional<argon::gfx::BackendKind> backend;   // --renderer
    std::optional<int> msaa;                          // --msaa N
    std::optional<PostAntiAliasing> postAntiAliasing; // --post-aa
    std::optional<bool> orderIndependent;             // --oit on / off
    bool validate = false;                            // --validate: Vulkan's validation layers, D3D12's debug layer
};

// --shot <png> after --frames N, from the --cam view
struct ShotRequest
{
    std::string path, camera;
    std::optional<int> frames;
};

// what the scene draws, the viewer's toggles and panels
struct SceneRequest
{
    std::optional<int> alphaMode;                 // --alpha off / all (else the materials')
    std::optional<bool> frontCounterClockwise;    // --cw / --ccw
    std::optional<int> trailMode;                 // --trail-mode 0..4
    bool noMaterials = false, noCull = false, noSky = false, emitters = false;
    bool noHud = false, noPanel = false, dumpMaterials = false;
};

// the race as the tests ask for it
struct RaceRequest
{
    std::optional<int> speedClass;                 // --speed 1..5, as 0..4
    std::optional<int> driveView;                  // --view
    std::optional<int> multiplayerPlayers;         // --multiplayer n: game mode 4
    std::optional<int> splitViews;                 // --split n: the views of racers 0..n-1 (after --multiplayer)
    bool timeAttack = false;                       // --time-attack (after --multiplayer)
    std::optional<float> timeLimit;                // --time-limit: the race's (the game's 1200 s)
    std::optional<float> attractIdleSeconds;       // --attract-after
    std::optional<std::array<int, 3>> powerups;    // --powerups r b y: the player's levels now and at race start
};

// the menus' picks and models as the tests ask for them
struct MenuPicks
{
    std::optional<int> course, character;   // --course, --char: FE COURSE SELECT's row, FE CHARSELECT's pick
    std::optional<int> mainMenuRow;         // --menu-sel
    std::vector<int> newRecords;            // --new-record t: FE FASTESTLAP's save mode for track t
    bool freezePoses = false;               // --no-anim: the menus' models hold still
};

// the profile's progress as the tests ask for it
struct ProfileRequest
{
    bool unlockAll = false;    // --unlock-all: the profile's unlocks ignored
    bool newProfile = false;   // --new-profile: the seven original characters, one speed
};

// what the command line sets on the viewer itself
struct ViewerLaunch
{
    bool mute = false;
    RendererRequest renderer;
    ShotRequest shot;
    SceneRequest scene;
    RaceRequest race;
    MenuPicks menu;
    ProfileRequest profile;
    TestScript tests;
};

struct CameraStart
{
    glm::vec3 position{0.f};
    float yawDeg = 0.f, pitchDeg = 0.f;   // yaw 0 = game -Z, 90 = +X
};

// the tests' shortcuts into a race from the menus (with --quick-race / --race-go)
struct RaceTestStart
{
    int quickRaceTrack = -1;
    int replay = -1;                   // --replay n: game_data/replays/replay<n> (logic/replay_race.hpp)
    int ghostReplay = -1;              // --ghost-replay g: after the --drive-time pre-run, the pause menu's ghost replay
    float ghostReplaySeconds = 0.f;    //   stepped this long
    bool continuePastLoader = false;   // --race-go
    bool aiDrivesPlayer = false;
    bool firstLinksOnly = false;       // --first-branch: no route choice, every craft takes each fork's first link
    int tourneyLevel = -1;
    int tourneyRacesWon = -1;     // --tourney-done n: the level's races won before it
    unsigned trophies = 0;        // --trophy l (repeatable): --char holds that level's trophy, bit l
    float loaderAt = -1.f;        // --loader-at t: the loading screen's clock (the Nemesis Challenge intro)
    float storyAt = -1.f;         // --story-at t: a story scene's clock starts at t
    bool raceEndScene = false;    // --race-end-scene: with --race-go, the tournament won and the race left (0x7fa50)
    float introAt = -1.f, countdownAt = -1.f;
    int finishPage = -1, pausePage = -1, cash = -1;
    int weaponLevel = -1;   // --weapon-level n: --char's Weapon Strength (0..7) on the race end's pages
    bool upgradeMenu = false;

    // a race started through the menus (ViewerStartup::startTestRace): a quick race, an attract demo or a tournament's
    bool startsARace() const
    {
        return quickRaceTrack >= 0 || replay > 0 || tourneyLevel >= 0;
    }
};

struct MenuStart
{
    int mode = -1;                 // 1 the main menu, 0 the editor, -1 decided by the other options
    int page = 0;                  // menu::Page
    bool skipTitle = false;
    bool skipIntro = false;        // --no-intro: as the game launched by another title (also settings.json, screenshot runs)
    int messageBox = 0;            // 1 info, 2 yes / no, 3 controller, 4 Saving, 5 timed, 6..8 no free blocks
    bool routeChoice = false;      // --route-choice: FE COURSE SELECT's route buttons open (page 4)
};

struct DriveStart
{
    bool enabled = false;
    float preRunSeconds = 0.f;
    float logEverySeconds = 0.f;
    float turboTestSeconds = -1.f;
};

struct LaunchOptions
{
    std::string gameHint, startTrack, pilot, settings;
    std::string pickTrack;   // --pick-track: the editor loads it behind its loading screen
    std::optional<argon::Resolution> windowSize;   // --size: the window's, as asked (settings.json's resolution kept)
    bool forceSound = false;
    std::optional<CameraStart> camera;
    std::optional<glm::vec3> lookAt;
    MenuStart menu;
    RaceTestStart raceTest;
    DriveStart drive;
    ViewerLaunch viewer;
    std::optional<int> exitCode;   // a command that ran at parse time (--decode, --tourney-order): exit with this
};

LaunchOptions parseLaunchOptions(int argc, char **argv);
