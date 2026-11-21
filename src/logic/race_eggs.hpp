// Argon: the easter eggs a race runs with (docs\frontend\cheats.md "Easter eggs and cheats in a race"). The race
// load (0x792f0) copies the eggs switched on in FE EASTEREGGS (0x1bbc10 + i) into the race flags 0x52c8b8..0x52c8be;
// main_menu (0x30af0) clears them on the way back to the front end, so they only hold in a race.
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>

// FE EASTEREGGS row order (fe 0x30c + i)
enum class Egg
{
    UltraFocalLength,
    HyperBlur,
    AtlantisMode,
    Slide,
    AlwaysRain,
    WeaponsOff,
    FruityFruit,
    MirrorTracks
};

class RaceEggs
{
  public:
    static constexpr int kCount = 8;

    void switchOn(const bool (&eggsOn)[kCount])
    {
        std::copy(std::begin(eggsOn), std::end(eggsOn), on_.begin());
    }
    void clear()
    {
        on_.fill(false);
    }
    bool on(Egg egg) const
    {
        return on_[size_t(egg)];
    }

  private:
    std::array<bool, kCount> on_{};
};
