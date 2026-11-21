// Argon: the AI's route table for a race (airuntime\airoutetable.cpp and aimap.cpp in the game; docs\physics\README.md
// "Opponents"): what lies on each branch of the AI nodes' forks, for the strategy's lists (logic\ai_routes.hpp).
//   surfaces  0xe38c0: a segment from 10 m above each branch node to 10 m below it (0xd4770) gives the ground's
//             material, 0xce6b0 at its lower end the water; water counts when the ground lies under its surface; a
//             node with neither counts in no surface
//   pickups   0xe2520 (the AI file's load): each blue / red / yellow / mega-up pickup counts at its nearest node when it
//             lies within 3 / 6 / 9 / 15 / 20 m (speed class) of the line to that node's first link, twice (its loop
//             runs two rounds); power chips do not count
//   shortest  0xe16e0 / 0xe1800: from each branch's first node along the first links to the first node (flag 2)
#include "craft/craftsim.hpp"

namespace argon
{
namespace
{
const float kProbeReach = 10.f;   // 0x1a35a4
const float kPickupReach[5] = {3.f, 6.f, 9.f, 15.f, 20.f};
const int kPickupCountsPerPickup = 2;

struct RouteTableBuilder
{
    const AiGraph &graph;
    const SimWorld &world;

    int surfaceUnder(const AiGraph::Node &node) const
    {
        const glm::vec3 above = node.pos + node.down * kProbeReach, below = node.pos - node.down * kProbeReach;
        glm::vec3 hit, normal;
        TriangleSurface surface;
        const bool ground = world.segment(above, below, hit, normal, surface);
        float depth = 0.f;
        const bool underWater = world.waterDepth(below, depth) && depth < 0.f;
        if (underWater && (!ground || hit.y < below.y - depth))
        {
            return airoutes::kWaterSurface;
        }
        return ground ? int(surface.type) : airoutes::kNoSurface;
    }

    std::vector<int> incomingLinks() const
    {
        std::vector<int> incoming(graph.nodes.size(), 0);
        for (const AiGraph::Node &node : graph.nodes)
        {
            for (int to : node.fwd)
            {
                ++incoming[size_t(to)];
            }
        }
        return incoming;
    }

    // 0xe38c0: the branch's nodes while each has one backward link (up to the merge), along the first links
    std::vector<airoutes::ArmNode> armOf(int start, const std::vector<int> &incoming, const std::vector<int> &pickups) const
    {
        std::vector<airoutes::ArmNode> arm;
        int routeNode = start;
        while (incoming[size_t(routeNode)] == 1 && arm.size() < graph.nodes.size())
        {
            const AiGraph::Node &node = graph.nodes[size_t(routeNode)];
            arm.push_back({surfaceUnder(node), (node.flags / AiGraph::TurboGoodSlow) & 0x1fu, pickups[size_t(routeNode)]});
            if (node.fwd.empty())
            {
                break;
            }
            routeNode = node.fwd[0];
        }
        return arm;
    }

    // 0xe1800: the length from a node along the first links to the first node
    float lengthToFirst(int start) const
    {
        float length = 0.f;
        int node = start;
        for (size_t steps = 0; !(graph.nodes[size_t(node)].flags & AiGraph::First) && steps < graph.nodes.size(); ++steps)
        {
            const int next = graph.next(node);
            length += glm::length(graph.nodes[size_t(next)].pos - graph.nodes[size_t(node)].pos);
            node = next;
        }
        return length;
    }

    int nearestNode(const glm::vec3 &point) const
    {
        int best = -1;
        float bestDistance = 3.4028235e38f;
        for (size_t i = 0; i < graph.nodes.size(); ++i)
        {
            const float distance = glm::length(point - graph.nodes[i].pos);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                best = int(i);
            }
        }
        return best;
    }

    // the sideways distance from the line through the node to its first link (horizontal normal (-dz, 0, dx))
    float sidewaysOffset(int node, const glm::vec3 &point) const
    {
        const glm::vec3 from = graph.nodes[size_t(node)].pos, to = graph.nodes[size_t(graph.next(node))].pos;
        const glm::vec3 side = glm::normalize(glm::vec3(from.z - to.z, 0.f, to.x - from.x));
        return glm::dot(point - from, side);
    }

    std::vector<int> pickupsPerNode(const std::vector<Pickup> &pickups, int speedClass) const
    {
        std::vector<int> counts(graph.nodes.size(), 0);
        const float reach = kPickupReach[std::clamp(speedClass, 0, 4)];
        for (const Pickup &pickup : pickups)
        {
            const glm::vec3 p(pickup.pos[0], pickup.pos[1], pickup.pos[2]);
            const int node = nearestNode(p);
            if (pickup.type == "POWERCHIP" || node < 0 || graph.nodes[size_t(node)].fwd.empty())
            {
                continue;
            }
            const float offset = sidewaysOffset(node, p);
            if (-reach < offset && offset < reach)
            {
                counts[size_t(node)] += kPickupCountsPerPickup;
            }
        }
        return counts;
    }

    airoutes::RouteNode fork(int node, const std::vector<int> &incoming, const std::vector<int> &pickups) const
    {
        airoutes::RouteNode route;
        route.next = graph.nodes[size_t(node)].fwd;
        if (route.next.size() < 2)
        {
            return route;
        }
        std::vector<float> toFirst;
        for (int start : route.next)
        {
            route.branches.push_back(airoutes::branchFacts(armOf(start, incoming, pickups)));
            toFirst.push_back(lengthToFirst(start));
        }
        route.shortest = airoutes::shortestBranch(toFirst);
        return route;
    }
};
}  // namespace

void AiGraph::buildRoutes(const SimWorld &world, const std::vector<Pickup> &pickups, int speedClass)
{
    const RouteTableBuilder builder{*this, world};
    const std::vector<int> incoming = builder.incomingLinks();
    const std::vector<int> pickupCounts = builder.pickupsPerNode(pickups, speedClass);
    routes.clear();
    for (size_t n = 0; n < nodes.size(); ++n)
    {
        routes.push_back(builder.fork(int(n), incoming, pickupCounts));
    }
}

}  // namespace argon
