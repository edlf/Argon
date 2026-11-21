// visdata.vis (track_visibility.hpp): read by the track loader (0xafa66, skipped when the model's header has flag
// 0x10), used by the race's track draw 0xafcf0.
#include "track/track_visibility.hpp"

#include <limits>

#include "original/record_read.hpp"
#include "original/track_records.hpp"

namespace argon
{

namespace
{

using original::readAt;

// the lists follow the sectors: each sector's mesh indices, then its neighbours
bool readList(const std::vector<uint8_t> &bytes, uint32_t count, size_t &cursor, std::vector<uint32_t> &list)
{
    if (count > (bytes.size() - cursor) / 4)
    {
        return false;
    }
    list.resize(count);
    for (uint32_t k = 0; k < count; ++k)
    {
        list[k] = readAt<uint32_t>(bytes, cursor + k * 4);
    }
    cursor += size_t(count) * 4;
    return true;
}

} // namespace

bool TrackVisibility::read(const std::vector<uint8_t> &bytes)
{
    sectors_.clear();
    if (bytes.size() < sizeof(original::VisHeader))
    {
        return false;
    }
    const uint32_t count = readAt<original::VisHeader>(bytes, 0).count;
    if (count > (bytes.size() - sizeof(original::VisHeader)) / sizeof(original::VisSector))
    {
        return false;
    }
    std::vector<VisSector> sectors(count);
    size_t cursor = sizeof(original::VisHeader) + size_t(count) * sizeof(original::VisSector);
    for (uint32_t s = 0; s < count; ++s)
    {
        const auto record = readAt<original::VisSector>(bytes, sizeof(original::VisHeader) + size_t(s) * sizeof(original::VisSector));
        sectors[s].position = {record.position[0], record.position[1], record.position[2]};
        if (!readList(bytes, record.meshCount, cursor, sectors[s].meshes) || !readList(bytes, record.neighbourCount, cursor, sectors[s].neighbours))
        {
            return false;
        }
    }
    sectors_ = std::move(sectors);
    return true;
}

size_t TrackVisibility::nearestSector(const glm::vec3 &eye) const
{
    size_t nearest = 0;
    float best = std::numeric_limits<float>::max();
    for (size_t s = 0; s < sectors_.size(); ++s)
    {
        const float distance = glm::length(eye - sectors_[s].position);
        if (distance < best)
        {
            best = distance;
            nearest = s;
        }
    }
    return nearest;
}

std::vector<bool> TrackVisibility::visibleMeshes(const glm::vec3 &eye, size_t meshCount) const
{
    if (sectors_.empty())
    {
        return std::vector<bool>(meshCount, true);
    }
    std::vector<bool> visible(meshCount, false);
    markVisibleMeshes(nearestSector(eye), visible);
    return visible;
}

void TrackVisibility::markVisibleMeshes(size_t sector, std::vector<bool> &visible) const
{
    const VisSector &home = sectors_[sector];
    markSectorMeshes(home, visible);
    for (uint32_t neighbour : home.neighbours)
    {
        if (neighbour < sectors_.size())
        {
            markSectorMeshes(sectors_[neighbour], visible);
        }
    }
}

void TrackVisibility::markSectorMeshes(const VisSector &sector, std::vector<bool> &visible)
{
    for (uint32_t mesh : sector.meshes)
    {
        if (mesh < visible.size())
        {
            visible[mesh] = true;
        }
    }
}

} // namespace argon
