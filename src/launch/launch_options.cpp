#include "launch/launch_options.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "logic/race_eggs.hpp"
#include "logic/split_screen.hpp"
#include "logic/tourney_rules.hpp"
#include "ui/mainmenu.hpp"
#include "ui/page_id.hpp"

namespace
{

class ArgReader
{
  public:
    ArgReader(int argc, char **argv) : argc_(argc), argv_(argv)
    {
    }
    bool done() const
    {
        return at_ >= argc_;
    }
    std::string take()
    {
        return at_ < argc_ ? std::string(argv_[at_++]) : std::string();
    }
    int takeInt()
    {
        return std::atoi(take().c_str());
    }
    float takeFloat()
    {
        return float(std::atof(take().c_str()));
    }
    int takeInt(int lo, int hi)
    {
        return std::clamp(takeInt(), lo, hi);
    }

  private:
    int argc_;
    char **argv_;
    int at_ = 1;
};

#if ARGON_DEVELOP

// tests: every character's tournament track order
int printTourneyOrder()
{
    static const char *kLevels[5] = {"Novice", "Amateur", "Expert", "Master", "Redshift"};
    for (int c = 0; c < 16; ++c)
    {
        for (int l = 0; l < 5; ++l)
        {
            std::printf("char %2d %-8s:", c, kLevels[l]);
            for (int r = 0; r < argon::tourney::racesIn(l); ++r)
            {
                std::printf(" %2d", argon::tourney::trackOf(l, c, r));
            }
            std::printf("\n");
        }
    }
    return 0;
}

class LaunchParser
{
  public:
    explicit LaunchParser(ArgReader &args) : args_(args)
    {
    }

    LaunchOptions parse()
    {
        while (!args_.done() && !options_.exitCode)
        {
            parseOne(args_.take());
        }
        return options_;
    }

  private:
    void parseOne(const std::string &arg)
    {
        if (parseStartup(arg) || parseScreenshot(arg) || parseRender(arg) || parseDrive(arg) || parseMenu(arg) || parseRaceTest(arg))
        {
            return;
        }
        parseCommand(arg);
    }

    bool parseStartup(const std::string &arg)
    {
        if (arg == "--game")
        {
            options_.gameHint = args_.take();
        }
        else if (arg == "--track")
        {
            options_.startTrack = args_.take();
        }
        else if (arg == "--pilot")
        {
            options_.pilot = args_.take();
        }
        else if (arg == "--size")
        {
            const int width = args_.takeInt();
            options_.windowSize = argon::Resolution{width, args_.takeInt()};
        }
        else if (arg == "--mute")
        {
            options_.viewer.mute = true;
        }
        else if (arg == "--sound")
        {
            options_.forceSound = true;   // keep audio on in --shot runs (testing)
        }
        else if (arg == "--validate")
        {
            options_.viewer.renderer.validate = true;
        }
        else if (arg == "--settings")
        {
            options_.settings = args_.take();   // tests: "type=choice;..." like a profile's "options"
        }
        else if (arg == "--unlock-all")
        {
            options_.viewer.profile.unlockAll = true;
        }
        else if (arg == "--new-profile")
        {
            options_.viewer.profile.newProfile = true;
        }
        else
        {
            return false;
        }
        return true;
    }

    bool parseScreenshot(const std::string &arg)
    {
        if (arg == "--shot")
        {
            options_.viewer.shot.path = args_.take();
        }
        else if (arg == "--cam")
        {
            options_.viewer.shot.camera = args_.take();
        }
        else if (arg == "--frames")
        {
            options_.viewer.shot.frames = std::max(1, args_.takeInt());
        }
        else if (arg == "--campos")
        {
            options_.camera = takeCamera();
        }
        else if (arg == "--look")
        {
            options_.lookAt = takeVec3();   // aim the camera at a game-space point from ~35 m away
        }
        else
        {
            return false;
        }
        return true;
    }

    bool parseRender(const std::string &arg)
    {
        if (arg == "--alpha")
        {
            options_.viewer.scene.alphaMode = alphaModeNamed(args_.take());
        }
        else if (arg == "--nomaterials")
        {
            options_.viewer.scene.noMaterials = true;
        }
        else if (arg == "--renderer")
        {
            const std::string name = args_.take();
            options_.viewer.renderer.backend = argon::gfx::backendNamed(name);
            if (!options_.viewer.renderer.backend)
            {
                std::fprintf(stderr, "--renderer %s: vulkan or d3d12\n", name.c_str());
                options_.exitCode = 2;
            }
        }
        else if (arg == "--msaa")
        {
            options_.viewer.renderer.msaa = std::max(1, args_.takeInt());
        }
        else if (arg == "--post-aa")
        {
            const std::string name = args_.take();
            options_.viewer.renderer.postAntiAliasing = postAntiAliasingNamed(name);
            if (!options_.viewer.renderer.postAntiAliasing)
            {
                std::fprintf(stderr, "--post-aa %s: off, fxaa or smaa\n", name.c_str());
                options_.exitCode = 2;
            }
        }
        else if (arg == "--oit")
        {
            const std::string choice = args_.take();
            if (choice != "on" && choice != "off")
            {
                std::fprintf(stderr, "--oit %s: on or off\n", choice.c_str());
                options_.exitCode = 2;
            }
            options_.viewer.renderer.orderIndependent = choice == "on";
        }
        else if (arg == "--nocull")
        {
            options_.viewer.scene.noCull = true;
        }
        else if (arg == "--cw")
        {
            options_.viewer.scene.frontCounterClockwise = false;
        }
        else if (arg == "--ccw")
        {
            options_.viewer.scene.frontCounterClockwise = true;
        }
        else if (arg == "--nosky")
        {
            options_.viewer.scene.noSky = true;
        }
        else if (arg == "--nohud")
        {
            options_.viewer.scene.noHud = true;
        }
        else if (arg == "--nopanel")
        {
            options_.viewer.scene.noPanel = true;
        }
        else if (arg == "--trail-mode")
        {
            options_.viewer.scene.trailMode = args_.takeInt(0, 4);
        }
        else if (arg == "--dump-materials")
        {
            options_.viewer.scene.dumpMaterials = true;
        }
        else if (arg == "--pick-track")
        {
            options_.pickTrack = args_.take();   // tests: the editor picks this track after the start (its loading screen)
        }
        else if (arg == "--hide-mesh")
        {
            options_.viewer.tests.hideMeshes.push_back(args_.takeInt());
        }
        else if (arg == "--emitters")
        {
            options_.viewer.scene.emitters = true;
        }
        else
        {
            return false;
        }
        return true;
    }

    bool parseDrive(const std::string &arg)
    {
        DriveStart &driveStart = options_.drive;
        if (arg == "--drive")
        {
            driveStart.enabled = true;
        }
        else if (arg == "--drive-time")
        {
            driveStart.enabled = true;
            driveStart.preRunSeconds = args_.takeFloat();
        }
        else if (arg == "--drive-log")
        {
            driveStart.logEverySeconds = args_.takeFloat();
        }
        else if (arg == "--turbo-test")
        {
            driveStart.turboTestSeconds = args_.takeFloat();   // tests: start with N s, hold turbo
        }
        else if (arg == "--speed")
        {
            options_.viewer.race.speedClass = std::clamp(args_.takeInt() - 1, 0, 4);
        }
        else if (arg == "--view")
        {
            options_.viewer.race.driveView = std::max(0, args_.takeInt());
        }
        else if (arg == "--autofire")
        {
            options_.viewer.tests.autoFire = true;
        }
        else if (arg == "--mp-seats")
        {
            options_.viewer.tests.seats = args_.takeInt(0, argon::split::kMaxPlayers);
        }
        else if (arg == "--multiplayer")
        {
            options_.viewer.race.multiplayerPlayers = args_.takeInt(1, argon::split::kMaxPlayers);
        }
        else if (arg == "--split")
        {
            options_.viewer.race.splitViews = args_.takeInt(1, argon::split::kMaxPlayers);
        }
        else if (arg == "--time-limit")
        {
            options_.viewer.race.timeLimit = args_.takeFloat();
        }
        else if (arg == "--knockout-at")
        {
            options_.viewer.tests.knockOutAt = args_.takeFloat();   // slot 1 knocks the player out N s into the race
        }
        else if (arg == "--powerups")
        {
            options_.viewer.race.powerups = takePowerups();   // red blue yellow (0..3)
        }
        else if (arg == "--special-at")
        {
            options_.viewer.tests.specialAt = args_.takeFloat();   // the player holds a special (levels 3 / 3 / 3) N s in
        }
        else
        {
            return false;
        }
        return true;
    }

    bool parseMenu(const std::string &arg)
    {
        MenuStart &menuStart = options_.menu;
        if (arg == "--menu")
        {
            menuStart.mode = 1;
        }
        else if (arg == "--editor")
        {
            menuStart.mode = 0;
        }
        else if (arg == "--menu-sel")
        {
            options_.viewer.menu.mainMenuRow = args_.takeInt(0, MainMenuPage::kRows - 1);
            menuStart.skipTitle = true;
        }
        else if (arg == "--no-title")
        {
            menuStart.skipTitle = true;
        }
        else if (arg == "--no-intro")
        {
            menuStart.skipIntro = true;
        }
        else if (arg == "--message")
        {
            menuStart.messageBox = args_.takeInt(1, 8);
        }
        else if (arg == "--route-choice")
        {
            menuStart.routeChoice = true;
        }
        else if (arg == "--new-record")
        {
            options_.viewer.menu.newRecords.push_back(args_.takeInt(0, 17));
        }
        else if (arg == "--menu-page")
        {
            menuStart.page = args_.takeInt(0, int(menu::PageCount) - 1);
        }
        else if (arg == "--course")
        {
            options_.viewer.menu.course = args_.takeInt(0, 8);
        }
        else if (arg == "--char")
        {
            options_.viewer.menu.character = args_.takeInt(0, 15);
        }
        else if (arg == "--no-anim")
        {
            options_.viewer.menu.freezePoses = true;
        }
        else
        {
            return false;
        }
        return true;
    }

    bool parseRaceTest(const std::string &arg)
    {
        RaceTestStart &raceTest = options_.raceTest;
        if (arg == "--quick-race")
        {
            raceTest.quickRaceTrack = args_.takeInt();
        }
        else if (arg == "--replay")
        {
            raceTest.replay = args_.takeInt(1, 3);
        }
        else if (arg == "--ghost-replay")
        {
            raceTest.ghostReplay = args_.takeInt(0, 1);
            raceTest.ghostReplaySeconds = args_.takeFloat();
        }
        else if (arg == "--view-replay")
        {
            options_.viewer.tests.viewReplay = true;   // the race played back from its recording at its end
        }
        else if (arg == "--save-replay")
        {
            options_.viewer.tests.saveReplayStem = args_.take();   // <stem>.info / .index / .events of each finished recording
        }
        else if (arg == "--attract-after")
        {
            options_.viewer.race.attractIdleSeconds = args_.takeFloat();
        }
        else if (arg == "--race-go")
        {
            raceTest.continuePastLoader = true;
        }
        else if (arg == "--time-attack")
        {
            options_.viewer.race.timeAttack = true;
        }
        else if (arg == "--tourney-race")
        {
            raceTest.tourneyLevel = args_.takeInt(0, 4);
        }
        else if (arg == "--tourney-done")
        {
            raceTest.tourneyRacesWon = args_.takeInt(0, 8);
        }
        else if (arg == "--trophy")
        {
            raceTest.trophies |= 1u << args_.takeInt(0, 4);
        }
        else if (arg == "--loader-at")
        {
            raceTest.loaderAt = args_.takeFloat();
        }
        else if (arg == "--story-at")
        {
            raceTest.storyAt = args_.takeFloat();
        }
        else if (arg == "--race-end-scene")
        {
            raceTest.raceEndScene = true;
        }
        else if (arg == "--ai-race")
        {
            raceTest.aiDrivesPlayer = true;
        }
        else if (arg == "--first-branch")
        {
            raceTest.firstLinksOnly = true;
        }
        else if (arg == "--egg")
        {
            options_.viewer.tests.eggs |= 1u << args_.takeInt(0, RaceEggs::kCount - 1);
        }
        else if (arg == "--intro-at")
        {
            raceTest.introAt = args_.takeFloat();
        }
        else if (arg == "--countdown-at")
        {
            raceTest.countdownAt = args_.takeFloat();
        }
        else if (arg == "--upgrade-menu")
        {
            raceTest.upgradeMenu = true;
        }
        else if (arg == "--cash")
        {
            raceTest.cash = std::max(0, args_.takeInt());
        }
        else if (arg == "--weapon-level")
        {
            raceTest.weaponLevel = args_.takeInt(0, 7);
        }
        else if (arg == "--finish")
        {
            raceTest.finishPage = args_.takeInt(0, 6);
        }
        else if (arg == "--finish-leave")
        {
            options_.viewer.tests.leaveAt = args_.takeFloat();   // two A presses on the race-end page N s after start-up
        }
        else if (arg == "--pause")
        {
            raceTest.pausePage = args_.takeInt(0, 4);
        }
        else
        {
            return false;
        }
        return true;
    }

    void parseCommand(const std::string &arg)
    {
        if (arg == "--tourney-order")
        {
            options_.exitCode = printTourneyOrder();
        }
    }

    static int alphaModeNamed(const std::string &name)
    {
        return name == "off" ? 0 : name == "all" ? 2 : 1;
    }

    glm::vec3 takeVec3()
    {
        glm::vec3 v;
        for (int k = 0; k < 3; ++k)
        {
            v[k] = args_.takeFloat();
        }
        return v;
    }

    CameraStart takeCamera()
    {
        CameraStart cameraStart;
        cameraStart.position = takeVec3();
        cameraStart.yawDeg = args_.takeFloat();
        cameraStart.pitchDeg = args_.takeFloat();
        return cameraStart;
    }

    std::array<int, 3> takePowerups()
    {
        std::array<int, 3> levels{};
        for (int &level : levels)
        {
            level = args_.takeInt(0, 3);
        }
        return levels;
    }

    ArgReader &args_;
    LaunchOptions options_;
};

#endif   // ARGON_DEVELOP

} // namespace

#if ARGON_DEVELOP
LaunchOptions parseLaunchOptions(int argc, char **argv)
{
    ArgReader args(argc, argv);
    return LaunchParser(args).parse();
}
#else
// without the developer build only the game folder is read (the "could not find the game files" message asks for it)
LaunchOptions parseLaunchOptions(int argc, char **argv)
{
    ArgReader args(argc, argv);
    LaunchOptions options;
    while (!args.done())
    {
        if (args.take() == "--game")
        {
            options.gameHint = args.take();
        }
    }
    return options;
}
#endif
