// Argon: a log of the AI nodes' forks each racer passes (the headless drive's report, docs\physics\opponents.md "Opponents"):
// the branch its route took there and, half a second later, the branch whose path (fork, its nodes, the merge) lies
// nearest the craft, i.e. the one it really flies.
#pragma once
#include <optional>
#include <vector>

#include "craft/craftsim.hpp"

class RouteForkLog
{
  public:
    struct Entry
    {
        int racer = 0, lap = 0, node = 0, branch = 0, nearestBranch = 0;
        std::optional<argon::airoutes::RouteList> list;
    };

    void reset(const argon::AiGraph &graph);
    void step(int racer, const argon::GameCraft &craft);   // each 60 Hz step, racer < kRacers
    const std::vector<Entry> &entries() const
    {
        return entries_;
    }

    static constexpr int kRacers = 16;

  private:
    struct Pending
    {
        Entry entry;
        int stepsLeft = -1;   // -1: nothing to measure
    };
    std::vector<int> pathOf(int fork, int branch) const;
    float distanceToPath(const glm::vec3 &position, const std::vector<int> &path) const;
    int nearestBranch(const glm::vec3 &position, int fork) const;
    void finish(Pending &pending, const glm::vec3 &position);

    const argon::AiGraph *graph_ = nullptr;
    std::vector<int> incoming_;
    int forksSeen_[kRacers]{};
    Pending pending_[kRacers]{};
    std::vector<Entry> entries_;
};
