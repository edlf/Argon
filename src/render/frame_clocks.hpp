// Argon: the clocks the frame's drawing runs on (app\scene_frame.hpp keeps them): an animated model's pose at a clock,
// written into this frame's buffer, and the GLFW clock since start-up (held while the window is unfocused).
#pragma once

struct ModelClock;

class FrameClocks
{
  public:
    virtual ModelClock modelClock(float seconds) const = 0;
    virtual double secondsSinceStart() const = 0;

  protected:
    ~FrameClocks() = default;
};
