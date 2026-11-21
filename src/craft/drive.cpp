#include "craft/drive.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstdio>
#include <map>

namespace argon
{

// ------------------------------------------------------------------ collision grid
void CollisionGrid::build(const std::vector<glm::vec3> &tris, float cell)
{
    tris_ = tris;
    cells_.clear();
    cell_ = cell;
    if (tris_.empty())
    {
        return;
    }
    min_ = glm::vec2(1e30f);
    max_ = glm::vec2(-1e30f);
    for (auto &triangle : tris_)
    {
        min_ = glm::min(min_, glm::vec2(triangle.x, triangle.z));
        max_ = glm::max(max_, glm::vec2(triangle.x, triangle.z));
    }
    nx_ = std::max(1, int(std::ceil((max_.x - min_.x) / cell_)) + 1);
    nz_ = std::max(1, int(std::ceil((max_.y - min_.y) / cell_)) + 1);
    cells_.assign(size_t(nx_) * nz_, {});
    for (uint32_t t = 0; t < tris_.size() / 3; ++t)
    {
        forCellsInBox(triangleCells(t), [&](size_t ci) { cells_[ci].push_back(t); });
    }
}

// a triangle listed by several cells of the box is kept in the first of them the walk reaches (z, then x): the box's
// cell at the larger of the two ranges' first x and first z. No list of the ones seen, no allocation
void CollisionGrid::trianglesInBox(glm::vec3 lo, glm::vec3 hi, std::vector<uint32_t> &out) const
{
    out.clear();
    if (tris_.empty())
    {
        return;
    }
    const CellRange box = cellRange(glm::vec2(lo.x, lo.z), glm::vec2(hi.x, hi.z));
    for (int z = box.z0; z <= box.z1; ++z)
    {
        for (int x = box.x0; x <= box.x1; ++x)
        {
            for (uint32_t triangle : cells_[size_t(z) * nx_ + x])
            {
                const CellRange own = triangleCells(triangle);
                const bool firstListing = std::max(own.z0, box.z0) == z && std::max(own.x0, box.x0) == x;
                if (firstListing && triangleOverlaps(triangle, lo, hi))
                {
                    out.push_back(triangle);
                }
            }
        }
    }
}

bool CollisionGrid::triangleOverlaps(uint32_t triangle, const glm::vec3 &lo, const glm::vec3 &hi) const
{
    const glm::vec3 &a = tris_[triangle * 3], &b = tris_[triangle * 3 + 1], &c = tris_[triangle * 3 + 2];
    const glm::vec3 mn = glm::min(glm::min(a, b), c), mx = glm::max(glm::max(a, b), c);
    return !(mx.x < lo.x || mn.x > hi.x || mx.y < lo.y || mn.y > hi.y || mx.z < lo.z || mn.z > hi.z);
}

// the cells the triangle's xz bounds cover: the ones build() lists it in
CollisionGrid::CellRange CollisionGrid::triangleCells(uint32_t triangle) const
{
    const glm::vec3 &a = tris_[triangle * 3], &b = tris_[triangle * 3 + 1], &c = tris_[triangle * 3 + 2];
    const glm::vec2 lo = glm::min(glm::min(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z)), glm::vec2(c.x, c.z));
    const glm::vec2 hi = glm::max(glm::max(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z)), glm::vec2(c.x, c.z));
    return cellRange(lo, hi);
}

CollisionGrid::CellRange CollisionGrid::cellRange(glm::vec2 lo, glm::vec2 hi) const
{
    return CellRange{std::clamp(int((lo.x - min_.x) / cell_), 0, nx_ - 1), std::clamp(int((hi.x - min_.x) / cell_), 0, nx_ - 1),
                     std::clamp(int((lo.y - min_.y) / cell_), 0, nz_ - 1), std::clamp(int((hi.y - min_.y) / cell_), 0, nz_ - 1)};
}

template <class F> void CollisionGrid::forCellsInBox(glm::vec2 lo, glm::vec2 hi, F &&visit) const
{
    forCellsInBox(cellRange(lo, hi), std::forward<F>(visit));
}

template <class F> void CollisionGrid::forCellsInBox(const CellRange &range, F &&visit) const
{
    for (int z = range.z0; z <= range.z1; ++z)
    {
        for (int x = range.x0; x <= range.x1; ++x)
        {
            visit(size_t(z) * nx_ + x);
        }
    }
}

bool rayHitsTriangle(const Ray &ray, const glm::vec3 *corners, float &distance)
{
    const glm::vec3 &origin = ray.origin, &d = ray.direction;
    const glm::vec3 e1 = corners[1] - corners[0], e2 = corners[2] - corners[0], p = glm::cross(d, e2);
    const float det = glm::dot(e1, p);
    if (std::abs(det) < 1e-9f)
    {
        return false;
    }
    const float inv = 1.f / det;
    const glm::vec3 s = origin - corners[0];
    const float u = glm::dot(s, p) * inv;
    if (u < 0 || u > 1)
    {
        return false;
    }
    const glm::vec3 q = glm::cross(s, e1);
    const float v = glm::dot(d, q) * inv;
    if (v < 0 || u + v > 1)
    {
        return false;
    }
    distance = glm::dot(e2, q) * inv;
    return distance >= 0;
}

// a triangle's normal from its winding (not turned)
static glm::vec3 faceNormal(const glm::vec3 *corners)
{
    return glm::normalize(glm::cross(corners[1] - corners[0], corners[2] - corners[0]));
}

bool CollisionGrid::rayDown(const glm::vec3 &from, float maxDrop, GroundHit &hit) const
{
    if (tris_.empty() || from.x < min_.x || from.z < min_.y || from.x > max_.x || from.z > max_.y)
    {
        return false;
    }
    const size_t ci = size_t(std::clamp(int((from.z - min_.y) / cell_), 0, nz_ - 1)) * nx_ + size_t(std::clamp(int((from.x - min_.x) / cell_), 0, nx_ - 1));
    const Ray down{from, glm::vec3(0, -1, 0)};
    float best = maxDrop;
    bool found = false;
    for (uint32_t triangle : cells_[ci])
    {
        float distance;
        if (rayHitsTriangle(down, &tris_[size_t(triangle) * 3], distance) && distance < best)
        {
            best = distance;
            found = true;
            hit = GroundHit{from.y - best, faceNormal(&tris_[size_t(triangle) * 3])};
        }
    }
    if (found && hit.normal.y < 0)
    {
        hit.normal = -hit.normal;
    }
    return found;
}

bool CollisionGrid::segment(const glm::vec3 &from, const glm::vec3 &to, SegmentHit &hit) const
{
    if (tris_.empty())
    {
        return false;
    }
    const Ray along{from, to - from};
    float best = 1.f;
    bool found = false;
    forCellsInBox(glm::min(glm::vec2(from.x, from.z), glm::vec2(to.x, to.z)), glm::max(glm::vec2(from.x, from.z), glm::vec2(to.x, to.z)), [&](size_t ci) {
        for (uint32_t tri : cells_[ci])
        {
            float distance;
            if (rayHitsTriangle(along, &tris_[size_t(tri) * 3], distance) && distance < best)
            {
                best = distance;
                found = true;
                hit = SegmentHit{best, faceNormal(&tris_[size_t(tri) * 3]), int(tri)};
            }
        }
    });
    return found;
}

// ground for a point on the AI line: first surface below (from 4 m above, so tunnel roofs
// and bridges are skipped); if there is none, the line may run a little under the road or
// the water, so look up to 20 m above it
static bool groundAt(const CollisionGrid &grid, const glm::vec3 &point, GroundHit &ground)
{
    return grid.rayDown(glm::vec3(point.x, point.y + 4.f, point.z), 124.f, ground) || grid.rayDown(glm::vec3(point.x, point.y + 20.f, point.z), 16.f, ground);
}

// ------------------------------------------------------------------ route
static glm::vec3 v3(const std::array<float, 3> &values)
{
    return {values[0], values[1], values[2]};
}

static glm::vec3 catmull(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, float fraction)
{
    float t2 = fraction * fraction, t3 = t2 * fraction;
    return 0.5f * ((2.f * p1) + (-p0 + p2) * fraction + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t2 + (-p0 + 3.f * p1 - 3.f * p2 + p3) * t3);
}

// the route's i-th node, wrapping round a closed loop, else held at the ends
glm::vec3 Route::nodePos(const TrackData &td, long index) const
{
    const long count = long(nodeOrder_.size());
    index = closed_ ? ((index % count) + count) % count : std::clamp<long>(index, 0, count - 1);
    return v3(td.ai[nodeOrder_[size_t(index)]].pos);
}

void Route::build(const TrackData &td)
{
    samples_.clear();
    nodeOrder_.clear();
    closed_ = false;
    if (td.ai.size() < 3)
    {
        throw std::runtime_error("no AI nodes");
    }
    std::map<int, int> byId;
    for (size_t i = 0; i < td.ai.size(); ++i)
    {
        byId[td.ai[i].id] = int(i);
    }
    int start = 0;
    for (size_t i = 0; i < td.ai.size(); ++i)
    {
        if (td.ai[i].first)
        {
            start = int(i);
            break;
        }
    }
    std::vector<bool> seen(td.ai.size(), false);
    int node = start;
    while (node >= 0 && !seen[node])
    {
        seen[node] = true;
        nodeOrder_.push_back(node);
        const auto &forward = td.ai[node].forward;
        int nextNode = -1;
        if (!forward.empty())
        {
            auto it = byId.find(forward[0]);
            if (it != byId.end())
            {
                nextNode = it->second;
            }
        }
        if (nextNode == start)
        {
            closed_ = true;
        }
        node = nextNode;
    }
    if (nodeOrder_.size() < 3)
    {
        throw std::runtime_error("AI route too short");
    }

    size_t count = nodeOrder_.size();
    float distance = 0;
    glm::vec3 prev(0);
    size_t segs = closed_ ? count : count - 1;
    for (size_t i = 0; i < segs; ++i)
    {
        glm::vec3 p0 = nodePos(td, long(i) - 1), p1 = nodePos(td, long(i)), p2 = nodePos(td, long(i) + 1), p3 = nodePos(td, long(i) + 2);
        int steps = std::max(2, int(glm::length(p2 - p1) / 2.f));    // ~2 m samples
        const AiNode &nd = td.ai[nodeOrder_[i]];
        float cap = nd.cornerHairpin ? 0.45f : nd.cornerSlow ? 0.7f : 1.f;   // fraction of vmax
        for (int k = 0; k < steps; ++k)
        {
            glm::vec3 point = catmull(p0, p1, p2, p3, float(k) / steps);
            if (!samples_.empty())
            {
                distance += glm::length(point - prev);
            }
            samples_.push_back({point, glm::vec3(0, 0, 1), distance, cap, 0.f, int(nodeOrder_[i])});
            prev = point;
        }
    }
    if (closed_)
    {   // close the loop distance
        distance += glm::length(samples_.front().pos - prev);
        samples_.push_back(samples_.front());
        samples_.back().dist = distance;
    }
    for (size_t i = 0; i < samples_.size(); ++i)
    {
        size_t before = i > 0 ? i - 1 : (closed_ ? samples_.size() - 2 : 0);
        size_t after = i + 1 < samples_.size() ? i + 1 : (closed_ ? 1 : i);
        glm::vec3 tangent = samples_[after].pos - samples_[before].pos;
        samples_[i].tangent = glm::length(tangent) > 1e-4f ? glm::normalize(tangent) : glm::vec3(0, 0, 1);
    }
}

void Route::planSpeeds(float vmax, float latAccel, float accel, float brake)
{
    size_t size = samples_.size();
    if (size < 3)
    {
        return;
    }
    // curvature limit from the horizontal turn rate between samples ~10 m apart
    for (size_t i = 0; i < size; ++i)
    {
        size_t before = i >= 5 ? i - 5 : (closed_ ? i + size - 1 - 5 : 0);
        size_t after = i + 5 < size ? i + 5 : (closed_ ? i + 5 - (size - 1) : size - 1);
        glm::vec3 ta = samples_[before].tangent, tb = samples_[after].tangent;
        glm::vec2 ha(ta.x, ta.z), hb(tb.x, tb.z);
        float len = std::max(1.f, std::abs(samples_[after].dist - samples_[before].dist));
        float ang =
            (glm::length(ha) > 1e-3f && glm::length(hb) > 1e-3f) ? std::acos(std::clamp(glm::dot(glm::normalize(ha), glm::normalize(hb)), -1.f, 1.f)) : 0.f;
        float radius = ang > 1e-4f ? len / ang : 1e9f;
        samples_[i].speed = std::min(vmax * samples_[i].speedCap, std::sqrt(latAccel * radius));
    }
    // braking (backward) and acceleration (forward) limits; two rounds so a closed loop settles
    for (int round = 0; round < 2; ++round)
    {
        for (size_t k = size - 1; k-- > 0;)
        {
            float ds = samples_[k + 1].dist - samples_[k].dist;
            samples_[k].speed = std::min(samples_[k].speed, std::sqrt(samples_[k + 1].speed * samples_[k + 1].speed + 2 * brake * ds));
        }
        if (closed_)
        {
            samples_.back().speed = samples_.front().speed = std::min(samples_.front().speed, samples_.back().speed);
        }
        for (size_t k = 1; k < size; ++k)
        {
            float ds = samples_[k].dist - samples_[k - 1].dist;
            samples_[k].speed = std::min(samples_[k].speed, std::sqrt(samples_[k - 1].speed * samples_[k - 1].speed + 2 * accel * ds));
        }
        if (closed_)
        {
            samples_.front().speed = samples_.back().speed;
        }
    }
}

namespace
{
bool sampleBefore(const RouteSample &sample, float distance)
{
    return sample.dist < distance;
}
}  // namespace

RouteSample Route::at(float distance) const
{
    if (samples_.empty())
    {
        return {};
    }
    float routeLength = length();
    if (closed_ && routeLength > 0)
    {
        distance = std::fmod(std::fmod(distance, routeLength) + routeLength, routeLength);
    }
    distance = std::clamp(distance, 0.f, routeLength);
    auto it = std::lower_bound(samples_.begin(), samples_.end(), distance, sampleBefore);
    if (it == samples_.begin())
    {
        return *it;
    }
    if (it == samples_.end())
    {
        return samples_.back();
    }
    const RouteSample &after = *it, &before = *(it - 1);
    float share = (distance - before.dist) / std::max(1e-4f, after.dist - before.dist);
    RouteSample sample = before;
    sample.pos = glm::mix(before.pos, after.pos, share);
    sample.tangent = glm::normalize(glm::mix(before.tangent, after.tangent, share));
    sample.speed = before.speed + (after.speed - before.speed) * share;
    sample.dist = distance;
    return sample;
}

std::string RouteIssue::text() const
{
    char text[160];
    switch (kind)
    {
    case NoGround:
        std::snprintf(text, sizeof text, "no road under the AI line for %.0f m", to - from);
        break;
    case Blocked:
        std::snprintf(text, sizeof text, "solid wall across the AI line (%.0f deg)", value);
        break;
    case Steep:
        std::snprintf(text, sizeof text, "ledge: road rises %.1f m within 2 m", value);
        break;
    case Airborne:
        std::snprintf(text, sizeof text, "jump/drop: line up to %.0f m above the ground for %.0f m", value, to - from);
        break;
    }
    return text;
}

namespace
{
// an issue merged into the last one of its kind if that one ended within 6 m (to = its end, value = the larger)
void addIssue(std::vector<RouteIssue> &issues, const RouteIssue &issue)
{
    for (auto it = issues.rbegin(); it != issues.rend() && issue.from - it->to < 6.f; ++it)
    {
        if (it->kind == issue.kind)
        {
            it->to = issue.from;
            it->value = std::max(it->value, issue.value);
            return;
        }
    }
    issues.push_back(issue);
}
}  // namespace

std::vector<RouteIssue> Route::check(const CollisionGrid &grid, float hover) const
{
    std::vector<RouteIssue> out;
    bool havePrev = false, prevAir = false;
    float prevY = 0;
    glm::vec3 prevShip(0);
    for (const auto &sample : samples_)
    {
        GroundHit ground;
        if (!groundAt(grid, sample.pos, ground))
        {
            addIssue(out, RouteIssue{RouteIssue::NoGround, sample.dist, sample.dist, sample.pos, 0});
            havePrev = false;
            continue;
        }
        // the ray starts 4 m above the line, so the ground is never far above it; a big gap
        // below the line means the road falls away (a jump or drop the ship flies over)
        const float gy = ground.y;
        float heightAboveRoad = sample.pos.y - (gy + hover);
        bool air = heightAboveRoad > 8.f;
        if (air)
        {
            addIssue(out, RouteIssue{RouteIssue::Airborne, sample.dist, sample.dist, sample.pos, heightAboveRoad});
        }
        glm::vec3 ship(sample.pos.x, gy + hover + 1.2f, sample.pos.z);
        if (havePrev && !air)
        {
            // ledge going up; not when landing after a jump (the previous ground was far below)
            if (!prevAir && gy - prevY > 2.5f)
            {
                addIssue(out, RouteIssue{RouteIssue::Steep, sample.dist, sample.dist, ship, gy - prevY});
            }
            SegmentHit wall;
            if (grid.segment(prevShip, ship, wall) && std::abs(wall.normal.y) < 0.5f)   // near-vertical surface in the way
            {
                const float tilt = glm::degrees(std::acos(std::clamp(std::abs(wall.normal.y), 0.f, 1.f)));
                addIssue(out, RouteIssue{RouteIssue::Blocked, sample.dist, sample.dist, glm::mix(prevShip, ship, wall.along), tilt});
            }
        }
        prevY = gy;
        prevShip = ship;
        prevAir = air;
        havePrev = true;
    }
    for (auto &issue : out)   // a single-sample NoGround is a 0 m entry; give it the sample spacing
    {
        if (issue.kind == RouteIssue::NoGround)
        {
            issue.to = std::max(issue.to, issue.from + 2.f);
        }
    }
    return out;
}

// ------------------------------------------------------------------ driver
void Driver::reset(const Route &route, float startDist)
{
    dist = startDist;
    auto sample = route.at(dist);
    speed = 0;
    travel = 0;
    lapTime = 0;
    laps = 0;
    pos = sample.pos;
    forward = sample.tangent;
    up = glm::vec3(0, 1, 0);
}

void Driver::step(const Route &route, const CollisionGrid &grid, float dt, float speedScale, float hover)
{
    if (!route.valid())
    {
        return;
    }
    auto sample = route.at(dist);
    float target = sample.speed * speedScale;
    float acceleration = target > speed ? 35.f : -60.f;                      // m/s^2 throttle / brake
    speed = target > speed ? std::min(target, speed + acceleration * dt) : std::max(target, speed + acceleration * dt);
    dist += speed * dt;
    travel += speed * dt;
    lapTime += dt;
    if (route.closed() && dist >= route.length())
    {
        dist -= route.length();
    }
    if (travel >= route.length())
    {        // a lap = one route length driven from the grid
        travel -= route.length();
        if (!route.closed())
        {
            dist = 0;
        }
        lastLap = lapTime;
        bestLap = bestLap > 0 ? std::min(bestLap, lapTime) : lapTime;
        lapTime = 0;
        ++laps;
    }
    sample = route.at(dist);
    GroundHit ground{0.f, glm::vec3(0, 1, 0)};
    overGround = groundAt(grid, sample.pos, ground);
    const float gy = ground.y;
    const glm::vec3 normal = ground.normal;
    if (overGround && sample.pos.y - (gy + hover) > 8.f)
    {
        overGround = false;   // jump: fly the line
    }
    glm::vec3 target3 = overGround ? glm::vec3(sample.pos.x, gy + hover, sample.pos.z) : sample.pos;
    float followShare = 1.f - std::exp(-dt / 0.05f);
    pos = glm::mix(pos, target3, followShare);
    pos.x = target3.x;
    pos.z = target3.z;                         // follow the line exactly in XZ
    glm::vec3 upT = overGround ? normal : glm::vec3(0, 1, 0);
    up = glm::normalize(glm::mix(up, upT, 1.f - std::exp(-dt / 0.15f)));
    forward = glm::normalize(sample.tangent - up * glm::dot(sample.tangent, up));
}

} // namespace argon
