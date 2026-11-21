// Argon: a --shot run: the frame written after --frames N (from --cam's view: start or top), then the viewer exits.
// It writes no saves (profile, lap records) and starts in the editor unless a test race or --menu says otherwise.
#pragma once
#include <string>

#include "launch/launch_options.hpp"

class ScreenshotRun
{
  public:
    // the command line's --shot / --cam / --frames
    void use(const ShotRequest &request)
    {
        if (!request.path.empty())
        {
            path_ = request.path;
        }
        if (!request.camera.empty())
        {
            camera_ = request.camera;
        }
        frames_ = request.frames.value_or(frames_);
    }
    bool active() const
    {
        return !path_.empty();
    }
    const std::string &path() const
    {
        return path_;
    }
    // --cam top: the editor's camera over the track, no fog
    bool fromTop() const
    {
        return camera_ == "top";
    }
    // one frame presented; true on the frame that is written (the run then ends)
    bool countFrame()
    {
        return active() && ++framesDrawn_ >= frames_;
    }

  private:
    std::string path_, camera_ = "start";
    int frames_ = 5, framesDrawn_ = 0;
};
