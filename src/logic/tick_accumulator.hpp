// Argon: the frames' time cut into the game's 60 Hz ticks (the game steps its objects once per 60 Hz frame). What is
// left over is the share of a tick the views blend the craft's last two poses by.
#pragma once
#include <algorithm>

namespace argon
{

constexpr float kTicksPerSecond = 60.f;
constexpr float kGameTick = 1.f / kTicksPerSecond;

class TickAccumulator
{
  public:
    void restart()
    {
        pending_ = 0.f;
    }
    void add(float seconds)
    {
        pending_ += seconds;
    }
    bool tickDue() const
    {
        return pending_ >= kGameTick;
    }
    void takeTick()
    {
        pending_ -= kGameTick;
    }
    float tickShare() const   // 0..1
    {
        return std::clamp(pending_ * kTicksPerSecond, 0.f, 1.f);
    }

  private:
    float pending_ = 0.f;
};

} // namespace argon
