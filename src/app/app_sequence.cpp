// Argon: the loading screen's flyby as App runs it (movie\loading_flyby.hpp): started with the track, its view
// each frame onto the shot the scene and the sky read.
#include "app/app.hpp"

// its camera drives the view while the loading screen shows (app_frame.cpp loaderFlybyShown)
void App::updateSequence()
{
    if (const std::optional<FlybyFrame> flyby = loadingFlyby.frameAt(glfwGetTime()))
    {
        showSequenceShot(flyby->shot, flyby->newCamera);
    }
}

// a story scene's camera (movie\story_scene.hpp): shownCamera is the scene's own
void App::useSequenceCamera(const SequenceCamera &camera, int &shownCamera)
{
    const bool newCamera = camera.object != shownCamera;
    shownCamera = camera.object;
    showSequenceShot(viewShotOf(camera), newCamera);
}

// a new camera starts the 0.25 s scene transition (0x97486)
void App::showSequenceShot(const ViewShot &shot, bool newCamera)
{
    if (newCamera)
    {
        startTransition();
    }
    viewShot = shot;
    freeCamera.position = shot.eye;   // sky centre, audio listener
}

void App::startGlorySequence(const std::string &trackFolder)
{
    loadingFlyby.start(readBytes(game + "/data/movies/" + trackFolder + "/" + trackFolder + "_glory.nms"), glfwGetTime());
    updateSequence();
}
