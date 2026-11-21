#include "logic/route_fork_log.hpp"

#include <algorithm>

namespace
{
const int kStepsBeforeMeasuring = 30;   // half a second at 60 Hz
const size_t kLongestArm = 32;
}  // namespace

void RouteForkLog::reset(const argon::AiGraph &graph)
{
    graph_ = &graph;
    incoming_.assign(graph.nodes.size(), 0);
    for (const argon::AiGraph::Node &node : graph.nodes)
    {
        for (int to : node.fwd)
        {
            ++incoming_[size_t(to)];
        }
    }
    std::fill(std::begin(forksSeen_), std::end(forksSeen_), 0);
    std::fill(std::begin(pending_), std::end(pending_), Pending{});
    entries_.clear();
}

void RouteForkLog::step(int racer, const argon::GameCraft &craft)
{
    Pending &pending = pending_[racer];
    const argon::GameCraft::ForkPassed &fork = craft.lastFork();
    if (fork.count != forksSeen_[racer])
    {
        finish(pending, craft.position());
        forksSeen_[racer] = fork.count;
        pending.entry = {racer, craft.laps() + 1, fork.node, fork.branch, 0, craft.route()};
        pending.stepsLeft = kStepsBeforeMeasuring;
        return;
    }
    if (pending.stepsLeft > 0 && --pending.stepsLeft == 0)
    {
        finish(pending, craft.position());
    }
}

void RouteForkLog::finish(Pending &pending, const glm::vec3 &position)
{
    if (!graph_ || pending.stepsLeft < 0)
    {
        return;
    }
    pending.entry.nearestBranch = nearestBranch(position, pending.entry.node);
    entries_.push_back(pending.entry);
    pending.stepsLeft = -1;
}

// the branch's path: the fork, its nodes while each has one backward link, then the merge
std::vector<int> RouteForkLog::pathOf(int fork, int branch) const
{
    std::vector<int> path{fork};
    int node = graph_->nodes[size_t(fork)].fwd[size_t(branch)];
    while (path.size() < kLongestArm)
    {
        path.push_back(node);
        if (incoming_[size_t(node)] != 1)
        {
            break;
        }
        node = graph_->next(node);
    }
    return path;
}

float RouteForkLog::distanceToPath(const glm::vec3 &position, const std::vector<int> &path) const
{
    float nearest = 3.4028235e38f;
    for (size_t i = 0; i + 1 < path.size(); ++i)
    {
        const glm::vec3 from = graph_->nodes[size_t(path[i])].pos, ab = graph_->nodes[size_t(path[i + 1])].pos - from;
        const float share = glm::dot(ab, ab) > 0.f ? std::clamp(glm::dot(position - from, ab) / glm::dot(ab, ab), 0.f, 1.f) : 0.f;
        nearest = std::min(nearest, glm::length(position - (from + ab * share)));
    }
    return nearest;
}

int RouteForkLog::nearestBranch(const glm::vec3 &position, int fork) const
{
    int best = 0;
    float bestDistance = 3.4028235e38f;
    for (size_t k = 0; k < graph_->nodes[size_t(fork)].fwd.size(); ++k)
    {
        const float distance = distanceToPath(position, pathOf(fork, int(k)));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = int(k);
        }
    }
    return best;
}
