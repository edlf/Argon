// Argon: the renderer's start: the backend and MSAA asked for, then what draws with it (render\renderer.hpp) and the
// models that load through it.
#include "app/app.hpp"

// the backend and MSAA --renderer / --msaa or settings.json ask for, then what the renderer keeps for the whole run.
// --renderer is followed as given; settings.json's Vulkan falls back to Direct3D 12 on Windows, and Argon Options then
// shows the renderer that started
void App::initRenderer()
{
    const int msaa = rendererRequest.msaa.value_or(viewerSettings.msaa());
    if (rendererRequest.backend)
    {
        renderer.start({*rendererRequest.backend, rendererRequest.validate, msaa, viewerSettings.vsync()}, window);
    }
    else
    {
        renderer.startOrFallBack({viewerSettings.renderer(), rendererRequest.validate, msaa, viewerSettings.vsync()}, window);
        viewerSettings.setRenderer(renderer.kind());
    }
    painter.attach(renderer.device(), renderer.ui(), game);
    movie = std::make_unique<MoviePlayer>(renderer.device(), renderer.ui(), audio);
    renderer.createResources(game, kViewSlots);
    frameCopies.create();
    models.create({renderer.device(), renderer.white, track, game, dumpMaterials});
    props.create(models, renderer.propPool, game);
    retiredTracks.create(models, renderer.device());
}
