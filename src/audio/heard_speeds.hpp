// Argon: each craft's speed as the engine sounds hear it (0xc5200): it follows the craft's |forward speed| by at most
// 1 m/s a frame. By grid slot; slot 0 is the player's.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>

class HeardSpeeds
{
  public:
    static constexpr int kSlots = 16;

    void follow(int slot, float forwardSpeed)
    {
        float &heard = speeds_[size_t(slot)];
        heard += std::clamp(std::abs(forwardSpeed) - heard, -1.f, 1.f);
    }
    float of(int slot) const
    {
        return speeds_[size_t(slot)];
    }
    // the craft's race starts again: its sound from standing
    void restart(int slot)
    {
        speeds_[size_t(slot)] = 0.f;
    }

  private:
    std::array<float, kSlots> speeds_{};
};
