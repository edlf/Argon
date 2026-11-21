// Argon: lock-on (0x6c420) and the HUD's crosshairs (0x4e7f0 states, drawn by 0x4d9d0), per player: the targets in front
// of a craft, and per colour list 0 red (crosshair.dds), 1 blue (crosshair_blue), 2 yellow (crosshair_yellow) the
// crosshairs following them.
#pragma once
#include "logic/race_crafts.hpp"

#include <array>
#include <vector>

namespace argon::weapons
{

// 0x6c420: the crafts within 500 m and a 0.85 cone of the craft in `slot`'s forward axis, nearest first, at most 6; none
// when no craft is in that slot
std::vector<int> lockCandidates(int slot, const RaceCrafts &crafts);

// one crosshair: 0 none, 1 appearing (0.5 s), 2 held, 3 released (gone 0.25 s later); stateSince: when its state began
struct Crosshair
{
    int state = 0, target = -1;
    float stateSince = 0;
};

class PlayerLocks
{
  public:
    static constexpr int kLists = 3;
    static constexpr int kCrosshairs = 6;
    using Lists = std::array<std::vector<int>, kLists>;
    using Crosshairs = std::array<std::array<Crosshair, kCrosshairs>, kLists>;

    // 0x4e7f0, each step with this step's lists: a listed target keeps or gets a crosshair (at most one new one per
    // 0.25 s and list), the others are released
    void update(const Lists &lists, float now);
    const Lists &lists() const;
    const Crosshairs &crosshairs() const;
    // a list holds the craft in `slot` (the AI's shields react to it, 0x6e1e0)
    bool holds(int slot) const;

  private:
    std::array<bool, kCrosshairs> follow(int list, float now);
    void age(int list, const std::array<bool, kCrosshairs> &seen, float now);

    Lists lists_;
    Crosshairs crosshairs_{};
    std::array<float, kLists> lastNew_{};
};

}  // namespace argon::weapons
