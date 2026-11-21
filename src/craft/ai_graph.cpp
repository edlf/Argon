// Argon: the AI node graph a track's route builds (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
#include "craft/craftsim.hpp"
#include "files/game_path.hpp"
#include "logic/blast_push.hpp"
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

namespace argon
{

// ------------------------------------------------------------------ AI graph
void AiGraph::build(const TrackData &td)
{
    nodes.clear();
    std::map<int, int> byId;
    for (size_t i = 0; i < td.ai.size(); ++i)
    {
        byId[td.ai[i].id] = int(i);
    }
    first = 0;
    for (size_t i = 0; i < td.ai.size(); ++i)
    {
        const AiNode &aiNode = td.ai[i];
        Node node;
        node.pos = glm::vec3(aiNode.pos[0], aiNode.pos[1], aiNode.pos[2]);
        node.down = glm::vec3(aiNode.down[0], aiNode.down[1], aiNode.down[2]);
        node.flags |= aiNode.turboGood * TurboGoodSlow;
        for (int forward : aiNode.forward)
        {
            auto it = byId.find(forward);
            if (it != byId.end())
            {
                node.fwd.push_back(it->second);
            }
        }
        if (aiNode.first)
        {
            node.flags |= First;
            first = int(i);
        }
        if (aiNode.restart)
        {
            node.flags |= Restart;
        }
        if (aiNode.cornerSlow)
        {
            node.flags |= CornerSlow;
        }
        if (aiNode.cornerHairpin)
        {
            node.flags |= Hairpin;
        }
        if (aiNode.cornerStraight)
        {
            node.flags |= Straight;
        }
        if (aiNode.envInside)
        {
            node.flags |= EnvInside;
        }
        nodes.push_back(node);
    }
    routes.clear();
    dist.assign(nodes.size(), -1.f);
    length = 0;
    if (nodes.empty())
    {
        return;
    }
    measureMainLoop();
    spreadSideBranches();
}

void AiGraph::measureMainLoop()
{
    int node = first;
    last = first;
    for (size_t k = 0; k <= nodes.size(); ++k)
    {
        dist[node] = length;
        int nx = next(node);
        if (nx == first)
        {
            last = node;
        }
        length += glm::length(nodes[nx].pos - nodes[node].pos);
        node = nx;
        if (node == first || dist[node] >= 0)
        {
            break;
        }
    }
}

void AiGraph::spreadSideBranches()
{
    for (size_t fork = 0; fork < nodes.size(); ++fork)
    {
        if (dist[fork] < 0.f)
        {
            continue;
        }
        for (size_t k = 1; k < nodes[fork].fwd.size(); ++k)
        {
            spreadSideBranch(int(fork), int(k));
        }
    }
}

// 0xe4460: the branch's nodes up to the merge, each 1 / (count + 1) of the way further from the fork's distance to
// the merge's
void AiGraph::spreadSideBranch(int fork, int branch)
{
    std::vector<int> arm;
    int node = nodes[size_t(fork)].fwd[size_t(branch)];
    while (dist[size_t(node)] < 0.f && arm.size() < nodes.size())
    {
        arm.push_back(node);
        node = next(node);
    }
    if (dist[size_t(node)] < 0.f)
    {
        return;
    }
    const float from = dist[size_t(fork)];
    const float to = dist[size_t(node)] < from ? dist[size_t(node)] + length : dist[size_t(node)];
    const float step = (to - from) / float(arm.size() + 1);
    for (size_t i = 0; i < arm.size(); ++i)
    {
        dist[size_t(arm[i])] = from + step * float(i + 1);
    }
}

} // namespace argon
