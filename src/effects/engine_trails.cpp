// Argon: the crafts' engine trails (effects\engine_trails.hpp; 0x50960, 0x9db10, 0x9dca0).
#include "effects/engine_trails.hpp"
#include "model/cmm.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace argon
{
namespace
{
const int kLateUpdates = 3;   // a point's colour is written 3 updates after its position

// a ribbon corner: u across the ribbon (0 / 1), the trail point's slot picks the texture row
struct TrailCorner
{
    glm::vec3 at;
    float across;
    int slot;
    const uint8_t *rgb;
};

Vertex trailVertex(const TrailCorner &corner)
{
    Vertex vertex{};
    vertex.pos[0] = corner.at.x;
    vertex.pos[1] = corner.at.y;
    vertex.pos[2] = corner.at.z;
    vertex.uv[0][0] = corner.across;
    vertex.uv[0][1] = float(corner.slot % EngineTrails::kPoints) * 0.25f;
    vertex.uv[1][0] = float(corner.slot % EngineTrails::kPoints) * 0.25f;
    vertex.uv[1][1] = 0.f;
    vertex.rgba[0] = corner.rgb[0];
    vertex.rgba[1] = corner.rgb[1];
    vertex.rgba[2] = corner.rgb[2];
    vertex.rgba[3] = 0xfe;
    return vertex;
}

// 0x50960: u = speed x 1.1 (max 0xf0), during a turbo u = speed x 2 (max 0xff); blue up to u 0x80, then towards red
std::array<uint8_t, 3> trailColour(const TrailDrive &drive)
{
    const float speed = std::abs(drive.forwardSpeed);
    const int u = drive.turbo ? std::clamp(int(std::lround(speed * 2.f)), 0, 0xff) : std::clamp(int(std::lround(speed * 1.1f)), 0, 0xf0);
    int blue = std::min(255, 2 * u), red = 0;
    if (u > 0x80)
    {
        red = std::clamp(2 * (u - 0x80), 0, 255);
        blue = 255 - red;
    }
    return {uint8_t((red * 0x50) >> 8), uint8_t((u << 4) >> 8), uint8_t((blue * 0x50) >> 8)};
}
}  // namespace

void EngineTrails::reset()
{
    rings_ = {};
    for (auto &heads : heads_)
    {
        heads[0] = heads[1] = 0;
    }
}

void EngineTrails::follow(int slot, const glm::mat4 &matrix, const TrailDrive &drive)
{
    slot = std::clamp(slot, 0, kEffectSlots - 1);
    const glm::vec3 right(matrix[0]), up(matrix[1]), forward(matrix[2]), pos(matrix[3]);
    fade(slot);
    const std::array<uint8_t, 3> colour = trailColour(drive);
    for (int side = 0; side < kTrailsPerCraft; ++side)
    {
        int &head = heads_[slot][side];
        head = (head + kPoints - 1) % kPoints;
        Ring &ring = rings_[size_t(slot)][size_t(side)];
        Point &point = ring[size_t(head)];
        point.pos = pos + (side ? 2.f : -2.f) * right - 4.f * forward;
        point.vertical = 0.5f * up;
        point.horizontal = right;
        std::memcpy(ring[size_t((head + kLateUpdates) % kPoints)].rgb, colour.data(), 3);
    }
}

// 0x9db10: every colour fades by 1 per update
void EngineTrails::fade(int slot)
{
    for (Ring &ring : rings_[size_t(slot)])
    {
        for (Point &point : ring)
        {
            for (uint8_t &channel : point.rgb)
            {
                channel = channel ? uint8_t(channel - 1) : 0;
            }
        }
    }
}

uint32_t EngineTrails::writeRibbons(Vertex *out) const
{
    uint32_t written = 0;
    for (int slot = 0; slot < kEffectSlots; ++slot)
    {
        for (int side = 0; side < kTrailsPerCraft; ++side)
        {
            for (int i = 0; i < kPoints - 1; ++i)
            {
                const int first = (heads_[slot][side] + i) % kPoints;
                written += writeSegment(rings_[size_t(slot)][size_t(side)], first, out + written);
            }
        }
    }
    return written;
}

// the vertical and the horizontal ribbon between a point and the next; nothing for points not written yet or dark
uint32_t EngineTrails::writeSegment(const Ring &ring, int first, Vertex *out) const
{
    const int second = (first + 1) % kPoints;
    const Point &p0 = ring[size_t(first)], &p1 = ring[size_t(second)];
    if (glm::length(p0.horizontal) < 0.5f || glm::length(p1.horizontal) < 0.5f)
    {
        return 0;
    }
    if (!(p0.rgb[0] | p0.rgb[1] | p0.rgb[2] | p1.rgb[0] | p1.rgb[1] | p1.rgb[2]))
    {
        return 0;
    }
    uint32_t written = 0;
    for (int ribbon = 0; ribbon < 2; ++ribbon)
    {
        const glm::vec3 o0 = ribbon ? p0.horizontal : p0.vertical, o1 = ribbon ? p1.horizontal : p1.vertical;
        const glm::vec3 a0 = p0.pos + o0, b0 = p0.pos - o0, a1 = p1.pos + o1, b1 = p1.pos - o1;
        const TrailCorner corners[6] = {{a0, 0, first, p0.rgb}, {b0, 1, first, p0.rgb}, {a1, 0, second, p1.rgb},
                                        {b0, 1, first, p0.rgb}, {b1, 1, second, p1.rgb}, {a1, 0, second, p1.rgb}};
        for (const TrailCorner &corner : corners)
        {
            out[written++] = trailVertex(corner);
        }
    }
    return written;
}

}  // namespace argon
