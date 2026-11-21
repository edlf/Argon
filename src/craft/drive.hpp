// AI drive for the viewer: a ship follows the game's own AI node graph (config\ai_<t>.txt),
// hovering over the collision mesh, and a route viability check.
//
// This is NOT the game's AI code (that runs in the XBE and also uses the hull/boundary
// planes, opponents and physics). It follows the same racing line the game's AI uses:
// forward0 links from the flag_first node, a Catmull-Rom spline through the node
// positions, speed planned from curvature plus the corner_slow / corner_hairpin hints.
#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "track/trackdata.hpp"

namespace argon
{

// a ray from origin along direction: its points are origin + t direction, t >= 0 (direction is not normalised)
struct Ray
{
    glm::vec3 origin, direction;
};

// the surface found below a point: its height and its normal, turned to face up
struct GroundHit
{
    float y;
    glm::vec3 normal;
};

// the first triangle on a segment: how far along (0..1), its normal and its index
struct SegmentHit
{
    float along;
    glm::vec3 normal;
    int triangle;
};

// whether the ray meets the triangle (both faces; Moller-Trumbore), and how far along
bool rayHitsTriangle(const Ray &ray, const glm::vec3 *corners, float &distance);

// Uniform XZ grid over collision triangles (the track's solid meshes).
class CollisionGrid
{
  public:
    void build(const std::vector<glm::vec3> &tris, float cell = 24.f);
    bool empty() const
    {
        return tris_.empty();
    }
    // the first surface going straight down from `from`, within maxDrop
    bool rayDown(const glm::vec3 &from, float maxDrop, GroundHit &hit) const;
    // the first triangle on the segment a -> b
    bool segment(const glm::vec3 &from, const glm::vec3 &to, SegmentHit &hit) const;
    size_t triangleCount() const
    {
        return tris_.size() / 3;
    }
    // triangles whose xz bounds overlap the box (unique, unsorted) and their corners
    void trianglesInBox(glm::vec3 lo, glm::vec3 hi, std::vector<uint32_t> &out) const;
    const glm::vec3 *triangle(uint32_t index) const
    {
        return &tris_[size_t(index) * 3];
    }

  private:
    struct CellRange
    {
        int x0, x1, z0, z1;   // inclusive
    };
    CellRange cellRange(glm::vec2 lo, glm::vec2 hi) const;
    CellRange triangleCells(uint32_t t) const;
    bool triangleOverlaps(uint32_t triangle, const glm::vec3 &lo, const glm::vec3 &hi) const;

    std::vector<glm::vec3> tris_;
    std::vector<std::vector<uint32_t>> cells_;
    glm::vec2 min_{0}, max_{0};
    float cell_ = 24.f;
    int nx_ = 0, nz_ = 0;
    template <class F> void forCellsInBox(glm::vec2 lo, glm::vec2 hi, F &&visit) const;
    template <class F> void forCellsInBox(const CellRange &range, F &&visit) const;
};

struct RouteSample
{
    glm::vec3 pos;        // on the AI line (spline through node positions)
    glm::vec3 tangent;
    float dist;           // distance along the route
    float speedCap;       // from node corner hints
    float speed;          // planned speed (m/s)
    int node;             // index into TrackData::ai of the segment start
};

struct RouteIssue
{
    enum Kind
    {
        NoGround,
        Blocked,
        Steep,
        Airborne
    } kind;
    float from, to;       // route distance range
    glm::vec3 pos;        // where it starts
    float value;          // gap length / wall angle / step height / height above ground
    std::string text() const;
    bool problem() const
    {
        return kind != Airborne;
    }   // jumps are info, the rest need a look
};

class Route
{
  public:
    // Builds the main route: forward0 links from the flag_first node (or node 0) until it loops; throws
    // std::runtime_error when the track has no AI nodes or the route is too short
    void build(const TrackData &td);
    glm::vec3 nodePos(const TrackData &td, long index) const;
    void planSpeeds(float vmax, float latAccel, float accel, float brake);
    bool valid() const
    {
        return samples_.size() > 4;
    }
    bool closed() const
    {
        return closed_;
    }
    float length() const
    {
        return samples_.empty() ? 0.f : samples_.back().dist;
    }
    int nodeCount() const
    {
        return int(nodeOrder_.size());
    }
    // position / tangent / planned speed at a route distance (wraps if closed)
    RouteSample at(float distance) const;
    const std::vector<RouteSample> &samples() const
    {
        return samples_;
    }
    // walk the route on the collision mesh and report problems
    std::vector<RouteIssue> check(const CollisionGrid &grid, float hover) const;

  private:
    std::vector<RouteSample> samples_;
    std::vector<int> nodeOrder_;
    bool closed_ = false;
};

// Simple driver: advances along the route at the planned speed, hovering over the ground.
struct Driver
{
    float dist = 0, travel = 0, speed = 0, lapTime = 0, lastLap = 0, bestLap = 0;
    int laps = 0;
    glm::vec3 pos{0}, forward{0, 0, 1}, up{0, 1, 0};
    bool overGround = true;
    void reset(const Route &r, float startDist = 0);
    // dt seconds; speedScale scales the planned speed; hover = height over the road
    void step(const Route &route, const CollisionGrid &grid, float dt, float speedScale, float hover);
};

} // namespace argon
