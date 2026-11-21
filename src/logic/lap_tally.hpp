// Argon: a race's finish order and race times counted from the racers' lap counters, as the race manager
// keeps them (Race::stepStandings: a finished lap adds its time, reaching the lap limit puts the racer in the order).
// A replay counts its own from the replayed crafts, leaving the race's results alone. Pure.
#pragma once
#include <algorithm>
#include <array>
#include <vector>

class LapTally
{
  public:
    static constexpr int kMaxRacers = 16;

    void reset()
    {
        order.clear();
        totals.fill(0.f);
        laps.fill(0);
    }
    // a racer's lap counter and its last lap's time this step; with lapLimit > 0 the racer finishes at that many laps
    void update(int racer, int lapsNow, float lastLap, int lapLimit)
    {
        if (racer < 0 || racer >= kMaxRacers || lapsNow <= laps[size_t(racer)])
        {
            return;
        }
        totals[size_t(racer)] += lastLap;
        laps[size_t(racer)] = lapsNow;
        if (lapLimit > 0 && lapsNow >= lapLimit && !finished(racer))
        {
            order.push_back(racer);
        }
    }
    bool finished(int racer) const
    {
        return std::find(order.begin(), order.end(), racer) != order.end();
    }
    const std::vector<int> &finishOrder() const
    {
        return order;
    }
    float raceTime(int racer) const
    {
        return racer >= 0 && racer < kMaxRacers ? totals[size_t(racer)] : 0.f;
    }

  private:
    std::vector<int> order;
    std::array<float, kMaxRacers> totals{};
    std::array<int, kMaxRacers> laps{};
};
