// The FPS meter (a viewer extra, not part of the game's UI): top-right; shown with Argon Options' FPS Meter (F2) or
// while the Debug menu is open.
#pragma once
#include "gfx/device.hpp"

#include <cstdint>

class FpsMeter
{
  public:
    void recordFrame(float seconds);   // unclamped frame time
    void draw(const argon::gfx::Device &device);

  private:
    static constexpr int kHistory = 120;

    float averageMs() const;
    float worstMs() const;
    void drawGraph(float x0, float y0, float worst) const;
    void sampleMemoryUsage(const argon::gfx::Device &device);

    float frameMs_[kHistory]{};
    int next_ = 0;
    double memorySampledAt_ = -1.0;
    char memoryText_[64]{};
};
