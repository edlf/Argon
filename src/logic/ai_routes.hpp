// Argon: the AI's route choice at the AI nodes' branches (airuntime: the route table CAIRouteTable, the strategy at
// drone +0x20f8 and the "ai dpa" at drone +0x1c90; docs\physics\opponents.md "Opponents").
//   table     0xe3830 / 0xe38c0, once per track load: for each branch of a node with more than one forward link, the
//             share of its nodes (up to the merge, the first node with another backward link) on each surface and
//             turbo_good for each speed class, in %, and its pickups (0x1b9be4..0x1b9bf0, 2 per pickup, 0xe2520);
//             0xe16e0: each node's shortest branch to the first node (0x1b9bf4)
//   lists     the strategy (0xe4730) keeps 8 lists of the 16 nodes ahead, rebuilt when the craft's node changes, each
//             taking its own branch at every fork (RouteList)
//   choice    0xe1370 every frame copies one list to the tactics (0xe4be0 / 0xe51a0 steer along it)
#pragma once
#include <array>
#include <vector>

#include "logic/weapon_rules.hpp"

namespace argon::airoutes
{

const int kSurfaces = 5;        // TriangleSurface::type: 0 track, 1 rough, 2 loose, 3 water, 4 slippery
const int kSpeedClasses = 5;
const int kLookAhead = 16;      // the strategy's lists: 16 shorts each (+0x448 + 0x20 k)
const int kTrackSurface = 0, kRoughSurface = 1, kLooseSurface = 2, kWaterSurface = 3, kSlipperySurface = 4;
const int kNoSurface = -1;      // 0xe38c0: no ground and no water under the node

// one branch's row in the route table (0x34 bytes: node, branch, then these)
struct BranchFacts
{
    std::array<float, kSurfaces> surfacePercent{};          // +8..+0x18 (stored track, rough, loose, water, slippery)
    std::array<float, kSpeedClasses> turboGoodPercent{};    // +0x20..+0x30: turbo_good_slow .. ultrafast
    int pickups = 0;                                        // +0x1c
};

// an AI node as the strategy sees it: its forward links and, at a fork, one row per link
struct RouteNode
{
    std::vector<int> next;
    std::vector<BranchFacts> branches;
    int shortest = 0;   // 0x1b9bf4[node]
};

// the strategy's lists, in the order of +0x448 + 0x20 k
enum class RouteList
{
    Track,        // 0xe47f0: 0xe3460 priority 2 (surface_track)
    Water,        // 0xe4850: priority 3 (surface_water)
    Loose,        // 0xe4970: priority 5 (surface_loose)
    Slippery,     // 0xe4910: priority 6 (surface_slippy)
    Rough,        // 0xe48b0: priority 4 (surface_rough)
    Shortest,     // 0xe4ae0: 0x1b9bf4
    TurboGood,    // 0xe49d0: 0xe3740, turbo_good for the race's speed class
    Pickups       // 0xe4a50: 0xe35d0, a pick weighted by the pickups, from the strategy's rand() (+0x54c)
};

// what a craft takes its list from (0xe1370)
struct RouteRacer
{
    int pilot = 0;          // 0x52cdc0[slot]
    int lapCounter = 1;     // race slot +0x14: 1 on the first lap
    int slot = 0;
    bool nemesisRace = false;                              // 0x791f0
    weapons::DroneKind kind = weapons::DroneKind::General; // slot +0x19 = craft +0x9f9, +0x1a = +0x9f8 (0x5949a)
};
RouteList chooseRouteList(const RouteRacer &racer);

// how one list walks: its kind, the race's speed class and the strategy's random number
struct RoutePreference
{
    RouteList list = RouteList::Track;
    int speedClass = 0;
    int seed = 0;
};
int branchOf(const RouteNode &node, const RoutePreference &preference);
int nextOnRoute(const std::vector<RouteNode> &nodes, int node, const RoutePreference &preference);
std::array<int, kLookAhead> lookAhead(const std::vector<RouteNode> &nodes, int start, const RoutePreference &preference);

// 0xe16e0: the branch with the least distance to the first node; with two branches it stores 0 either way (as shipped)
int shortestBranch(const std::vector<float> &distanceToFirst);

// 0xe38c0: one branch's row from what each of its nodes holds
struct ArmNode
{
    int surface = kTrackSurface;     // or kNoSurface
    unsigned turboGoodClasses = 0;   // bit s: turbo_good for speed class s (node flag 0x2000 << s)
    int pickups = 0;
};
BranchFacts branchFacts(const std::vector<ArmNode> &arm);

}  // namespace argon::airoutes
