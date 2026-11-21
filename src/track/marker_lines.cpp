// Argon: the editor's markings as line segments, see marker_lines.hpp.
#include "track/marker_lines.hpp"

#include <array>
#include <cmath>

namespace
{
constexpr int kRingSegments = 48;
const glm::vec3 kRouteColour(1, 0.85f, 0.1f), kRestartColour(1, 0.5f, 0), kEmitterColour(0.85f, 0.35f, 1.f);

glm::vec3 vec3Of(const std::array<float, 3> &xyz)
{
    return glm::vec3(xyz[0], xyz[1], xyz[2]);
}

glm::vec3 pickupColour(const std::string &type)
{
    if (type == "POWERUPRED")
    {
        return {1, 0.15f, 0.15f};
    }
    if (type == "POWERUPBLUE")
    {
        return {0.25f, 0.4f, 1};
    }
    if (type == "POWERUPYELLOW")
    {
        return {1, 1, 0.2f};
    }
    if (type == "POWERCHIP")
    {
        return {0.2f, 1, 0.6f};
    }
    return {1, 0.3f, 1};
}

glm::vec3 issueColour(argon::RouteIssue::Kind kind)
{
    switch (kind)
    {
    case argon::RouteIssue::NoGround:
        return {1, 0.1f, 0.1f};
    case argon::RouteIssue::Blocked:
        return {1, 0.4f, 0.f};
    case argon::RouteIssue::Airborne:
        return {0.3f, 0.8f, 1.f};
    default:
        return {1, 0.8f, 0.2f};
    }
}
}  // namespace

// each start slot: an arrow along its forward, a bar across and a post (slot 0 green, the rest blue)
void MarkerLines::addStarts(const argon::TrackData &track)
{
    for (size_t i = 0; i < track.starts.size(); ++i)
    {
        const argon::StartSlot &slot = track.starts[i];
        const glm::vec3 at = vec3Of(slot.pos), ahead = glm::normalize(vec3Of(slot.forward));
        const glm::vec3 right = glm::normalize(glm::cross(ahead, glm::vec3(0, 1, 0)));
        const glm::vec3 colour = i == 0 ? glm::vec3(0.2f, 1, 0.2f) : glm::vec3(0.1f, 0.8f, 1);
        line(at, at + ahead * 8.f, colour);
        line(at + ahead * 8.f, at + ahead * 5.f + right * 2.f, colour);
        line(at + ahead * 8.f, at + ahead * 5.f - right * 2.f, colour);
        line(at - right * 2.f, at + right * 2.f, colour);
        line(at, at + glm::vec3(0, 3, 0), colour);
    }
}

// the AI nodes' links half a metre up, and a post at each (taller at the restart nodes)
void MarkerLines::addAiRoute(const argon::TrackData &track)
{
    std::vector<int> indexById;
    for (size_t i = 0; i < track.ai.size(); ++i)
    {
        const int id = track.ai[i].id;
        if (id >= 0)
        {
            if (id >= int(indexById.size()))
            {
                indexById.resize(size_t(id) + 1, -1);
            }
            indexById[size_t(id)] = int(i);
        }
    }
    const glm::vec3 lift(0, 0.5f, 0);
    for (const argon::AiNode &node : track.ai)
    {
        const glm::vec3 at = vec3Of(node.pos) + lift;
        for (int next : node.forward)
        {
            if (next >= 0 && next < int(indexById.size()) && indexById[size_t(next)] >= 0)
            {
                line(at, vec3Of(track.ai[size_t(indexById[size_t(next)])].pos) + lift, kRouteColour);
            }
        }
        line(at, at + glm::vec3(0, node.restart ? 3.f : 1.5f, 0), kRestartColour);
    }
}

void MarkerLines::addPickups(const argon::TrackData &track)
{
    for (const argon::Pickup &pickup : track.pickups)
    {
        cross(vec3Of(pickup.pos), 1.5f, pickupColour(pickup.type));
    }
}

// sound emitters (audio\tracks\<t>\<t>.txt TRIGGER points): a cross, the MINDIST and the MAXDIST rings
void MarkerLines::addEmitters(const std::vector<argon::SoundEmitter> &emitters)
{
    for (const argon::SoundEmitter &emitter : emitters)
    {
        cross(emitter.pos, 4.f, kEmitterColour);
        ring(emitter.pos, emitter.minDist, kEmitterColour * 0.8f);
        ring(emitter.pos, emitter.maxDist, kEmitterColour * 0.45f);
    }
}

// the route check's problems: a pylon and an X
void MarkerLines::addIssues(const std::vector<argon::RouteIssue> &issues)
{
    for (const argon::RouteIssue &issue : issues)
    {
        const glm::vec3 colour = issueColour(issue.kind);
        line(issue.pos, issue.pos + glm::vec3(0, 25, 0), colour);
        line(issue.pos + glm::vec3(-6, 0, -6), issue.pos + glm::vec3(6, 0, 6), colour);
        line(issue.pos + glm::vec3(-6, 0, 6), issue.pos + glm::vec3(6, 0, -6), colour);
    }
}

void MarkerLines::line(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &colour)
{
    vertices_.push_back({from, colour});
    vertices_.push_back({to, colour});
}

void MarkerLines::cross(const glm::vec3 &at, float size, const glm::vec3 &colour)
{
    line(at - glm::vec3(size, 0, 0), at + glm::vec3(size, 0, 0), colour);
    line(at - glm::vec3(0, 0, size), at + glm::vec3(0, 0, size), colour);
    line(at, at + glm::vec3(0, 2 * size, 0), colour);
}

void MarkerLines::ring(const glm::vec3 &centre, float radius, const glm::vec3 &colour)
{
    constexpr float kTurn = 6.2831853f;
    for (int k = 0; k < kRingSegments; ++k)
    {
        const float from = float(k) / kRingSegments * kTurn, to = float(k + 1) / kRingSegments * kTurn;
        line(centre + radius * glm::vec3(std::cos(from), 0, std::sin(from)), centre + radius * glm::vec3(std::cos(to), 0, std::sin(to)), colour);
    }
}
