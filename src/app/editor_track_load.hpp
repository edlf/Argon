// Argon: the editor's track pick, loaded as the race's loader does (read on a worker, committed a slice a frame) but
// behind its own plain loading screen: black, "Loading", the track's folder and a progress bar.
#pragma once
#include <memory>
#include <string>

#include "track/track_load.hpp"

class HudPainter;

class EditorTrackLoad
{
  public:
    void start(const std::string &trackName, const TrackLoad::Use &use, TrackRequest request);
    void step();   // one frame's slice; the load is dropped once the track is in place
    bool loading() const
    {
        return load_ != nullptr;
    }
    void draw(HudPainter &painter) const;   // over everything, while loading

  private:
    std::unique_ptr<TrackLoad> load_;
    std::string trackName_;
};
