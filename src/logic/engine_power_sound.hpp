// Argon: when a craft's engine power down / up sounds play (docs\audio\ships.md "Ships"). A weapon hit that
// lengthens the engine-off time calls 0xc2ef0 (SF_SHIP_ENGINEPOWERDOWN), the time running out calls 0xc2f40
// (SF_SHIP_ENGINEPOWERUP). The sound manager's byte +0x201 + craft lets each play only after the other; it lives in
// BSS (0 at boot) and nothing else writes it, so a craft slot's first engine-off hit since boot is silent.
#pragma once

namespace argon
{

class EnginePowerSound
{
  public:
    // 0xc2ef0: true when the power down sound plays
    bool powerDown()
    {
        const bool plays = running_;
        running_ = false;
        return plays;
    }

    // 0xc2f40: true when the power up sound plays
    bool powerUp()
    {
        const bool plays = !running_;
        running_ = true;
        return plays;
    }

  private:
    bool running_ = false;   // +0x201 + craft
};

}  // namespace argon
