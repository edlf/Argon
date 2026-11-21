// Argon: the lap records' rules (logic\lap_record_rules.hpp)
#include "logic/lap_record_rules.hpp"

namespace argon::laps
{
namespace
{
bool anyIn(const std::array<bool, kTracks> &flags, int from, int to)
{
    for (int t = from; t < to; ++t)
    {
        if (flags[size_t(t)])
        {
            return true;
        }
    }
    return false;
}
}  // namespace

LapVerdict judgeLap(float lap, const LapTimes &before, bool timeAttack)
{
    LapVerdict verdict;
    if (lap >= before.raceBest)
    {
        return verdict;
    }
    verdict.raceBest = true;
    if (timeAttack)
    {
        verdict.profileBest = lap < before.profileBest;
        verdict.record = verdict.profileBest && lap < before.record;
        return verdict;
    }
    verdict.record = lap < before.record;
    return verdict;
}

bool opensOnSecondRoutes(const std::array<bool, kTracks> &newRecords)
{
    constexpr int kMars = 0x10, kTerraformedMars = 0x11;
    if (anyIn(newRecords, 0, 8) || newRecords[kMars])
    {
        return false;
    }
    return anyIn(newRecords, 8, 16) || newRecords[kTerraformedMars];
}
}  // namespace argon::laps
