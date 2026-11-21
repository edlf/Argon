// Argon: the speedometer's read-out: the player's last four forward speeds (gamehud_update 0x496f0), shown as their
// sum x 0.9 (huddata +0x14) in km/h.
#pragma once
#include <algorithm>
#include <array>

class SpeedReadout
{
  public:
    // one 60 Hz step's forward speed (m/s)
    void push(float forwardSpeed)
    {
        std::copy_backward(speeds_.begin(), speeds_.end() - 1, speeds_.end());
        speeds_[0] = forwardSpeed;
    }
    float kmh() const
    {
        return std::max(0.f, (speeds_[0] + speeds_[1] + speeds_[2] + speeds_[3]) * 0.9f);
    }

  private:
    std::array<float, 4> speeds_{};
};
