#include "logic/ai_routes.hpp"

#include <cstdlib>

namespace argon::airoutes
{
namespace
{
// 0xe1370: (pilot + laps + slot) & 7 -> the list (+0x464 + 0xf0 / 0xd0 / 0xb0 / 0x10 / 0x30 / 0x90 / 0x50 / 0x70)
const RouteList kListByTurn[8] = {
    RouteList::Pickups, RouteList::TurboGood, RouteList::Shortest, RouteList::Track, RouteList::Water, RouteList::Rough, RouteList::Loose, RouteList::Slippery};
const float kPercent = 100.f;   // 0x1a3450

int surfaceOf(RouteList list)
{
    switch (list)
    {
    case RouteList::Water:
        return kWaterSurface;
    case RouteList::Loose:
        return kLooseSurface;
    case RouteList::Slippery:
        return kSlipperySurface;
    case RouteList::Rough:
        return kRoughSurface;
    default:
        return kTrackSurface;
    }
}

// 0xe3460 / 0xe3740: the first branch with the largest share above 0, else branch 0
int firstLargest(const std::vector<float> &shares)
{
    int best = 0;
    float bestShare = 0.f;
    for (size_t k = 0; k < shares.size(); ++k)
    {
        if (bestShare < shares[k])
        {
            bestShare = shares[k];
            best = int(k);
        }
    }
    return best;
}

std::vector<float> surfaceShares(const RouteNode &node, int surface)
{
    std::vector<float> shares;
    for (const BranchFacts &facts : node.branches)
    {
        shares.push_back(facts.surfacePercent[size_t(surface)]);
    }
    return shares;
}

std::vector<float> turboGoodShares(const RouteNode &node, int speedClass)
{
    std::vector<float> shares;
    for (const BranchFacts &facts : node.branches)
    {
        shares.push_back(facts.turboGoodPercent[size_t(speedClass)]);
    }
    return shares;
}

// 0xe35d0: rand() % the pickups on the fork's branches, picked by their running sum; without pickups rand() % count
int pickupWeighted(const RouteNode &node, int seed)
{
    int total = 0;
    for (const BranchFacts &facts : node.branches)
    {
        total += facts.pickups;
    }
    if (total <= 0)
    {
        return std::abs(seed % int(node.next.size()));
    }
    const int roll = seed % total;
    int sum = 0;
    for (size_t k = 0; k < node.branches.size(); ++k)
    {
        sum += node.branches[k].pickups;
        if (roll < sum)
        {
            return int(k);
        }
    }
    return 0;
}

int clampedSpeedClass(int speedClass)
{
    return speedClass < 0 ? 0 : speedClass >= kSpeedClasses ? kSpeedClasses - 1 : speedClass;
}
}  // namespace

// a nemesis race takes Shortest; so do the nemesis (+0x19: Pickups only while the strategy's +0x549 is set, and 0xe4ae0,
// rebuilt last in 0xe4730, clears the +0x549 that 0xe4a50 sets) and a pilot on its home track (+0x1a)
RouteList chooseRouteList(const RouteRacer &racer)
{
    if (racer.nemesisRace || racer.kind != weapons::DroneKind::General)
    {
        return RouteList::Shortest;
    }
    return kListByTurn[(racer.pilot + racer.lapCounter + racer.slot) & 7];
}

int branchOf(const RouteNode &node, const RoutePreference &preference)
{
    if (node.next.size() < 2 || node.branches.size() != node.next.size())
    {
        return 0;
    }
    switch (preference.list)
    {
    case RouteList::Shortest:
        return node.shortest;
    case RouteList::TurboGood:
        return firstLargest(turboGoodShares(node, clampedSpeedClass(preference.speedClass)));
    case RouteList::Pickups:
        return pickupWeighted(node, preference.seed);
    default:
        return firstLargest(surfaceShares(node, surfaceOf(preference.list)));
    }
}

int nextOnRoute(const std::vector<RouteNode> &nodes, int node, const RoutePreference &preference)
{
    const RouteNode &here = nodes[size_t(node)];
    if (here.next.empty())
    {
        return node;
    }
    return here.next[size_t(branchOf(here, preference))];
}

std::array<int, kLookAhead> lookAhead(const std::vector<RouteNode> &nodes, int start, const RoutePreference &preference)
{
    std::array<int, kLookAhead> list{};
    list[0] = start;
    for (int i = 1; i < kLookAhead; ++i)
    {
        list[size_t(i)] = nextOnRoute(nodes, list[size_t(i - 1)], preference);
    }
    return list;
}

int shortestBranch(const std::vector<float> &distanceToFirst)
{
    if (distanceToFirst.size() != 3)
    {
        return 0;
    }
    const float d0 = distanceToFirst[0], d1 = distanceToFirst[1], d2 = distanceToFirst[2];
    if (d0 < d1 && d0 < d2)
    {
        return 0;
    }
    return d1 < d2 ? 1 : 2;
}

BranchFacts branchFacts(const std::vector<ArmNode> &arm)
{
    BranchFacts facts;
    if (arm.empty())
    {
        return facts;
    }
    for (const ArmNode &node : arm)
    {
        if (node.surface >= 0 && node.surface < kSurfaces)
        {
            facts.surfacePercent[size_t(node.surface)] += kPercent;
        }
        for (int s = 0; s < kSpeedClasses; ++s)
        {
            if (node.turboGoodClasses & (1u << s))
            {
                facts.turboGoodPercent[size_t(s)] += kPercent;
            }
        }
        facts.pickups += node.pickups;
    }
    const float perNode = 1.f / float(arm.size());
    for (float &share : facts.surfacePercent)
    {
        share *= perNode;
    }
    for (float &share : facts.turboGoodPercent)
    {
        share *= perNode;
    }
    return facts;
}

}  // namespace argon::airoutes
