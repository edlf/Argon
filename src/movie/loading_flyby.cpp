// Argon: the loading screen's flyby (movie\loading_flyby.hpp).
#include "movie/loading_flyby.hpp"

void LoadingFlyby::start(const std::vector<uint8_t> &file, double now)
{
    on_ = sequence_.load(file);
    startedAt_ = now;
    shownCamera_ = -1;
}

std::optional<FlybyFrame> LoadingFlyby::frameAt(double now)
{
    if (!on_)
    {
        return std::nullopt;
    }
    const std::optional<SequenceCamera> camera = sequence_.cameraAt(float(now - startedAt_));
    if (!camera)
    {
        return std::nullopt;
    }
    FlybyFrame frame;
    frame.shot = viewShotOf(*camera);
    frame.newCamera = camera->object != shownCamera_;
    shownCamera_ = camera->object;
    return frame;
}
