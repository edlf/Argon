// Argon: the AI pilot (drone_update), the pad pilot, the route and lap progress (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
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
#include "craft/row_major.hpp"

namespace argon
{
namespace
{
float percentOf(float control)   // a 0..1 control as the control slots' 0..100
{
    return std::clamp(control, 0.f, 1.f) * 100.f;
}
}  // namespace

// how far the craft is from the route segment that starts at `node`
float GameCraft::distanceToSegment(const AiGraph &graph, int node) const
{
    const glm::vec3 from = graph.nodes[size_t(node)].pos, to = graph.nodes[size_t(routeNext(graph, node))].pos, along = to - from;
    const float lengthSquared = glm::dot(along, along);
    const float share = lengthSquared > 1e-6f ? std::clamp(glm::dot(pos_ - from, along) / lengthSquared, 0.f, 1.f) : 0.f;
    return glm::length(pos_ - (from + along * share));
}

// point on the look-ahead polyline at distance r from the craft (0xe51a0)
glm::vec3 GameCraft::pointAtRadius(const AiGraph &graph, float radius, uint32_t &flags) const
{
    glm::vec3 p0 = graph.nodes[look_[0]].pos, p1 = graph.nodes[look_[1]].pos;
    float d1 = glm::length(p1 - pos_);
    if (d1 > radius)
    {
        flags = graph.nodes[look_[1]].flags;
        return p1;
    }
    float d0;
    for (int i = 1; i < 15; ++i)
    {
        p0 = p1;
        d0 = d1;
        p1 = graph.nodes[look_[i + 1]].pos;
        d1 = glm::length(p1 - pos_);
        if (d0 < radius && radius < d1)
        {
            float share = d1 - d0 == 0.f ? 1.f : (radius - d0) / (d1 - d0);
            flags = graph.nodes[look_[i]].flags;
            return p0 + (p1 - p0) * share;
        }
    }
    flags = graph.nodes[look_[15]].flags;
    return p1;
}

// 0xe46a0: the look-ahead, the three targets (moved aside to overtake), then the control slots from how far the
// aim is off them
void GameCraft::aiPilot(const AiGraph &graph)
{
    if (graph.nodes.size() < 3)
    {
        return;
    }
    fillLookAhead(graph);
    placeTargets(graph);
    moveTargetsToOvertake();
    // 0xe0f40: corner flags of the tactics points
    const uint32_t flags = flags1_ | flags2_;
    const AiCorner corner{(flags & AiGraph::CornerSlow) != 0, (flags & AiGraph::Straight) != 0, (flags & AiGraph::Hairpin) != 0};
    // 0xe0810
    const float blend = targetBlend();
    for (float &v : ctrl_)
    {
        v = 0;
    }
    target_ = P1_ * (1.f - blend) + P2_ * blend;
    const AiAimError error = aimError(blend, corner);
    ctrl_[12] = error.raw;   // +0x924 keeps the raw error (the steering assist uses it)
    steerAtTarget(error, blend, corner);
    throttleForAim(error.turned, blend, corner);
    climbOutOfDive();
}

// 0xe4730: the 16 nodes ahead of the current one on the list 0xe1370 picked
void GameCraft::fillLookAhead(const AiGraph &graph)
{
    look_[0] = node_;
    for (int i = 1; i < 16; ++i)
    {
        look_[i] = routeNext(graph, look_[i - 1]);
    }
}

// 0xe4be0: three targets at growing distances
void GameCraft::placeTargets(const AiGraph &graph)
{
    float classReach = speedClass_ < 2 ? 0.f : speedClass_ == 2 ? 20.f : 40.f;
    const uint32_t nextFlags = graph.nodes[look_[1]].flags;
    float extra = 90.f;   // brain +0x47c (= craft +0x2ad0), set every frame from the per-AI table 0x1b9648
    if (nextFlags & AiGraph::Hairpin)
    {
        classReach = 0;
        extra *= 0.5f;
    }
    if (nextFlags & AiGraph::CornerSlow)
    {
        classReach *= 0.5f;
    }
    P1_ = pointAtRadius(graph, 1.f, flags1_);
    P2_ = pointAtRadius(graph, classReach + extra + 1.f, flags2_);
    uint32_t farFlags;
    P3_ = pointAtRadius(graph, classReach + extra + 21.f, farFlags);
}

// overtaking (0xe4be0): less than 5 % of a lap behind the craft one place ahead -> all three points move
// sideways by up x (P1 - P3), normalised, x (10 - 3 gap), each component clamped to +-5; side from the viewer
void GameCraft::moveTargetsToOvertake()
{
    if (!overtake_ || overGap_ >= 5.f)
    {
        return;
    }
    const glm::vec3 along = P1_ - P3_;
    if (along.x == 0.f && along.z == 0.f)
    {
        return;
    }
    glm::vec3 offset = glm::cross(glm::vec3(0, 1, 0), along);
    const float length = glm::length(offset);
    if (length > 1e-6f)
    {
        offset *= (10.f - overGap_ * 3.f) / length;
    }
    offset = glm::clamp(offset, glm::vec3(-5.f), glm::vec3(5.f));
    if (overSide_ == 1)
    {
        offset = -offset;
    }
    P1_ += offset;
    P2_ += offset;
    P3_ += offset;
}

// how far between the near and middle targets the craft aims: its speed over the class's top (250 / 300 / 350)
float GameCraft::targetBlend() const
{
    const float speed = glm::length(currentState().velocity);
    if (speedClass_ <= 2)
    {
        return std::clamp(speed, 0.f, 250.f) * 0.004f;
    }
    if (speedClass_ == 3)
    {
        return std::clamp(speed, 0.f, 300.f) * 0.00333333341f;
    }
    return std::clamp(speed, 0.f, 350.f) * 0.00285714283f;
}

// the aim (nose plus some velocity) against the target and the far point; velinertia +0x14 is the angular momentum
// (state +0x54), not omega, and turns the error by the yaw
GameCraft::AiAimError GameCraft::aimError(float blend, const AiCorner &corner) const
{
    const State &state = currentState();
    const float velGain = 0.6f, yawGain = 0.018f;   // per-AI tables 0x1b9608 / 0x1b9688
    glm::vec3 aim = corner.straight ? fwd_ + state.velocity * velGain * blend * 0.005f : fwd_ + state.velocity * velGain * blend * blend * 0.01f;
    aim = glm::normalize(aim);
    const glm::vec3 side = glm::cross(up_, aim);
    const glm::vec3 toTarget = pos_ - target_, toFar = pos_ - P3_;
    AiAimError error;
    error.raw = glm::length(toTarget) > 1e-4f ? -glm::dot(side, glm::normalize(toTarget)) : 0.f;
    error.farPoint = glm::length(toFar) > 1e-4f ? -glm::dot(side, glm::normalize(toFar)) : 0.f;
    const float yaw = std::clamp(glm::dot(state.angularMomentum, up_) * yawGain, -0.1f, 0.1f);
    error.turned = error.raw - yaw;
    return error;
}

// steer (slots 5 / 6) toward the target when the far point agrees, the airbrake (7 / 8) past the brake error or in a
// hairpin
void GameCraft::steerAtTarget(const AiAimError &error, float blend, const AiCorner &corner)
{
    const float brakeErr = 0.4f;   // per-AI table 0x1b96c8
    const float threshold = corner.straight ? 0.03f : 0.02f;
    const bool hard = blend <= 0.5f || corner.slow || corner.hairpin;
    const float err = error.turned;
    if (err < -threshold && error.farPoint < 0.f)
    {
        ctrl_[5] = 0;
        ctrl_[6] = std::clamp((err + 0.02f) * (hard ? -500.f : -200.f), 0.f, 100.f);
        if (err < -brakeErr || corner.hairpin)
        {
            ctrl_[8] = 100;
        }
    }
    if (err > threshold && error.farPoint > 0.f)
    {
        ctrl_[6] = 0;
        ctrl_[5] = std::clamp((err - 0.02f) * (hard ? 500.f : 200.f), 0.f, 100.f);
        if (err > brakeErr || corner.hairpin)
        {
            ctrl_[7] = 100;
        }
    }
}

// full throttle while nearly on line (or slow), else the brake; a hairpin above the limit lets both go
void GameCraft::throttleForAim(float err, float blend, const AiCorner &corner)
{
    if ((err > -0.45f && err < 0.45f) || blend < 0.25f)
    {
        ctrl_[0] = 100;
    }
    else
    {
        ctrl_[9] = 100;
    }
    if (corner.hairpin)
    {
        const float limit = std::clamp(0.f * 0.5f, 0.3f, 1.f);   // control +0x470: nothing in the XBE writes it
        if (limit < blend)
        {
            ctrl_[9] = 0;
            ctrl_[0] = 0;
        }
    }
}

// 0xe1080: nose pointing down hard while airborne -> climb
void GameCraft::climbOutOfDive()
{
    if (!onGround_ && fwd_.y < -0.6f)
    {
        ctrl_[3] = std::clamp(fwd_.y * -50.f, 0.f, 100.f);
        ctrl_[2] = 0;
    }
}

// control slots as the AI fills them: [0] throttle, [9] brake, [5] / [6] steer, [7] / [8] airbrakes,
// [2] dive, [3] climb (0..100); [12] = 0 (no steering assist)
void GameCraft::padPilot()
{
    for (float &v : ctrl_)
    {
        v = 0;
    }
    ctrl_[0] = percentOf(pad_.throttle);
    ctrl_[9] = percentOf(pad_.brake);
    // the data is mirrored (right-handed in effect), so the craft's +right axis is screen left:
    // slot 6 steers screen right, slot 5 screen left; the airbrakes pair the same way
    ctrl_[6] = percentOf(pad_.steer);
    ctrl_[5] = percentOf(-pad_.steer);
    ctrl_[8] = percentOf(pad_.airRight);
    ctrl_[7] = percentOf(pad_.airLeft);
    ctrl_[2] = percentOf(pad_.dive);
    ctrl_[3] = percentOf(pad_.climb);
}

int GameCraft::routeNext(const AiGraph &graph, int node) const
{
    if (!routeList_ || graph.routes.size() != graph.nodes.size())
    {
        return graph.next(node);
    }
    return airoutes::nextOnRoute(graph.routes, node, {*routeList_, speedClass_, routeSeed_});
}

void GameCraft::notePassed(const AiGraph &graph, int from, int to)
{
    const std::vector<int> &links = graph.nodes[size_t(from)].fwd;
    if (links.size() < 2)
    {
        return;
    }
    lastFork_.node = from;
    lastFork_.branch = int(std::find(links.begin(), links.end(), to) - links.begin());
    ++lastFork_.count;
}

void GameCraft::trackProgress(const AiGraph &graph, float dt)
{
    if (graph.nodes.size() < 3)
    {
        return;
    }
    // progress along the route only: of the next few segments, take the one closest to the craft
    {
        int node = node_, best = node_;
        float bd = distanceToSegment(graph, node_);
        for (int k = 1; k <= 6; ++k)
        {
            node = routeNext(graph, node);
            float distance = distanceToSegment(graph, node);
            if (distance < bd)
            {
                bd = distance;
                best = node;
            }
        }
        for (int m = node_; m != best; m = routeNext(graph, m))
        {
            int nx = routeNext(graph, m);
            notePassed(graph, m, nx);
            if (graph.nodes[nx].flags & AiGraph::Restart)
            {
                lastRestart_ = nx;
            }
            if (nx == m)
            {
                break;
            }
        }
        node_ = best;
    }
    // the lap line (race_manager_update 0x62ba0): a racer's lap percent is the one of its current AI node (the route
    // table 0xe3c90: the start node 0x1b9bd0 0.1 %, the node before it 99 %, the others their distance along the
    // routes); a drop of 90 or more is a crossing. The current node changes when the craft passes a transition plane
    // of its node's hull (config\ai_<t>.txt <transplane> = n: the plane through node + n, normal n; table 0x1b9bd4),
    // and the files' planes lie halfway to the next node (bracken: node 51 -> 0, |n| 55.87 of 111.7 m): the line is
    // halfway between the last node and the start node, crossed forwards
    {
        const glm::vec3 &fp = graph.nodes[graph.first].pos;
        const glm::vec3 &lp = graph.nodes[graph.last].pos;
        glm::vec3 mid = (fp + lp) * 0.5f, dir = fp - lp;
        float side = glm::dot(pos_ - mid, dir);
        if (lapSide_ < 0.f && side >= 0.f && glm::length(pos_ - mid) < 80.f)
        {
            ++crossings_;
            crossLapTime_ = lapTime_;
            if (started_)
            {
                lastLap_ = lapTime_;
                bestLap_ = bestLap_ > 0 ? std::min(bestLap_, lapTime_) : lapTime_;
                ++laps_;
            }
            started_ = true;
            lapTime_ = 0;
        }
        lapSide_ = side;
    }
    lapTime_ += dt;
}

float GameCraft::lapProgress(const AiGraph &graph) const
{
    if (graph.length <= 0 || node_ >= int(graph.dist.size()) || graph.dist[node_] < 0)
    {
        return 0;
    }
    int nx = routeNext(graph, node_);
    glm::vec3 from = graph.nodes[node_].pos, ab = graph.nodes[nx].pos - from;
    float share = glm::dot(ab, ab) > 1e-6f ? std::clamp(glm::dot(pos_ - from, ab) / glm::dot(ab, ab), 0.f, 1.f) : 0.f;
    float distance = graph.dist[node_] + share * glm::length(ab);
    return std::fmod(distance / graph.length + 1.f, 1.f);
}

} // namespace argon
