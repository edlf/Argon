// Argon: the catch-up's rows that depend on who races (catch-up 0x6d650, tables from config\ai_values_<track>.txt,
// parser 0xe5990; docs\physics\opponents.md "Opponents"). The drone kind is the craft setup's (0x58e40: +0x9f9 nemesis, +0x9f8
// home circuit, logic\weapon_rules.hpp).
#pragma once
#include <array>

#include "logic/weapon_rules.hpp"

namespace argon::catchup
{

const int kSpeedClasses = 5;
using SpeedRow = std::array<float, kSpeedClasses>;

// <Kind>_Zone<n>Modifier, one zone's gain per speed class: 0x6d650 takes Nemesis_ for the nemesis (+0x9f9,
// 0x43a014 + 0x1ca0), else HomeCircuit_ on its home track (+0x9f8, + 0x1660), else General_ (+ 0x1020)
struct ZoneGainRows
{
    SpeedRow general{}, homeCircuit{}, nemesis{};
};
float zoneGain(const ZoneGainRows &rows, weapons::DroneKind kind, int speedClass);

// "Player Is": the player's pilot's <pilot>_Nemesis_AI_modifier for the nemesis (+0x9f9, 0x43a014 + 0x4c20), its
// <pilot>_General_AI_modifier for every other drone (+ 0x3820; a home circuit drone too)
struct PlayerIsRows
{
    SpeedRow general{}, nemesis{};
};
float playerIsGain(const PlayerIsRows &rows, weapons::DroneKind kind, int speedClass);

}  // namespace argon::catchup
