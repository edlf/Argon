#include "logic/catch_up_rules.hpp"

#include <algorithm>

namespace argon::catchup
{
namespace
{
size_t speedIndex(int speedClass)
{
    return size_t(std::clamp(speedClass, 0, kSpeedClasses - 1));
}

const SpeedRow &zoneRowOf(const ZoneGainRows &rows, weapons::DroneKind kind)
{
    if (kind == weapons::DroneKind::Nemesis)
    {
        return rows.nemesis;
    }
    return kind == weapons::DroneKind::HomeCircuit ? rows.homeCircuit : rows.general;
}
}  // namespace

float zoneGain(const ZoneGainRows &rows, weapons::DroneKind kind, int speedClass)
{
    return zoneRowOf(rows, kind)[speedIndex(speedClass)];
}

float playerIsGain(const PlayerIsRows &rows, weapons::DroneKind kind, int speedClass)
{
    const SpeedRow &row = kind == weapons::DroneKind::Nemesis ? rows.nemesis : rows.general;
    return row[speedIndex(speedClass)];
}

}  // namespace argon::catchup
