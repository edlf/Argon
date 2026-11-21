// The connected gamepads' two rumble motors, 0..65535 each (Xbox style, as the game's XInputSetState in 0xb4530: the
// first word the left, low-frequency motor, the second the right, high-frequency one). A pad is picked by its place
// among the connected pads with rumble (0 the first). One implementation per system: pad_rumble_win32.cpp (XInput),
// pad_rumble_linux.cpp (the kernel's force feedback, FF_RUMBLE); CMakeLists.txt picks one.
#pragma once
#include <cstdint>
#include <memory>

class PadRumble
{
  public:
    static constexpr int kPads = 4;

    PadRumble();
    ~PadRumble();   // the motors stop
    PadRumble(const PadRumble &) = delete;
    PadRumble &operator=(const PadRumble &) = delete;

    // to the `pad`-th connected pad (a pad that changes or goes away is stopped); unchanged speeds are not sent again
    void set(int pad, uint16_t lowFrequency, uint16_t highFrequency);

  private:
    struct Device;
    std::unique_ptr<Device> devices_[kPads];
};
