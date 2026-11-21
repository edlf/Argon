// Argon: a small Vulkan + Dear ImGui viewer for Quantum Redshift (command line: launch\launch_options.hpp).
//
// Controls: hold right mouse to look, WASD move, Q/E down/up, Shift = fast, mouse wheel = speed.
#include "app/app.hpp"
#include "launch/startup.hpp"
#include "platform/platform.hpp"

int runViewer(App &app, ViewerStartup &startup, bool playCountdown)
{
    try
    {
        startup.openWindow();
        startup.initRenderer();
        startup.enterStartScene();
        startup.preRunDrive();
        if (playCountdown)
        {
            app.playCountdown();
        }
        app.run();
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "error: %s\n", e.what());
        if (!app.screenshotRun())
        {
            platform::showErrorDialog("Argon", e.what());
        }
        app.shutdown();
        return 1;
    }
    app.shutdown();
    return 0;
}

int main(int argc, char **argv)
{
    App app;
    LaunchOptions options = parseLaunchOptions(argc, argv);
    if (options.exitCode)
    {
        return *options.exitCode;
    }
    app.useLaunchSettings(options.viewer);
    const bool playCountdown = options.forceSound;
    ViewerStartup startup(app, std::move(options));
    if (!startup.locateGame())
    {
        return 1;
    }
    startup.loadGameLists();
    startup.loadSettings();
    startup.initAudio();
    return runViewer(app, startup, playCountdown);
}
