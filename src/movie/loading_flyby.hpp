// Argon: the loading screen's flyby, an NMS sequence (movie\nms_sequence.hpp, docs\sequences\README.md): the track's
// data\movies\<t>\<t>_glory.nms behind the screen's text (loader_run 0x80090 -> 0x95e70 loads it,
// neon_start_glory_sequence 0x95ef0 -> 0x96690 starts it); each frame 0x98b70 evaluates it and 0x9ae4c takes its
// camera (0x972f0 kind 1: the eye, look-at and fov).
#pragma once
#include <cstdint>
#include <optional>
#include <vector>

#include "logic/drive_camera.hpp"
#include "movie/nms_sequence.hpp"

// a sequence camera as the shot the scene draws from (0x972f0 kind 1: the eye, look-at and fov)
inline ViewShot viewShotOf(const SequenceCamera &camera)
{
    return ViewShot{camera.eye, camera.look, glm::vec3(0, 1, 0), camera.fovDeg};
}

// the flyby's view at a moment
struct FlybyFrame
{
    ViewShot shot;
    bool newCamera = false;   // another of its cameras took over: the 0.25 s scene transition starts (0x97486)
};

class LoadingFlyby
{
  public:
    // the sequence's file (empty or not playable: no flyby), its clock from `now`
    void start(const std::vector<uint8_t> &file, double now);
    // the loading screen's story scene took over, or the race starts
    void stop()
    {
        on_ = false;
    }
    bool on() const
    {
        return on_;
    }
    // the view at `now` while it plays
    std::optional<FlybyFrame> frameAt(double now);

  private:
    NmsSequence sequence_;
    bool on_ = false;
    double startedAt_ = 0;
    int shownCamera_ = -1;
};
