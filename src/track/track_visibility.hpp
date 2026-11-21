// A track's visibility sectors, data\tracks\<t>\visdata.vis (docs/tracks/README.md "visdata.vis").
// In a race the track draw (0xafcf0) finds the sector nearest the eye and draws only the meshes listed by it and by its
// neighbours (the bone-attached ones too, 0xb8500); the rest of the track is skipped.
#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace argon
{

struct VisSector
{
    glm::vec3 position{0.f};
    std::vector<uint32_t> meshes;       // track mesh indices
    std::vector<uint32_t> neighbours;   // sector indices
};

class TrackVisibility
{
  public:
    // the file's bytes; false (and no sectors) when they do not hold a whole file
    bool read(const std::vector<uint8_t> &bytes);
    bool empty() const
    {
        return sectors_.empty();
    }
    const std::vector<VisSector> &sectors() const
    {
        return sectors_;
    }
    // the sector nearest to the eye (3D distance; the first of equals)
    size_t nearestSector(const glm::vec3 &eye) const;
    // per track mesh: drawn from this eye (meshCount entries; all true without sectors)
    std::vector<bool> visibleMeshes(const glm::vec3 &eye, size_t meshCount) const;
    // sets the meshes the sector and its neighbours list (those inside `visible`); the rest are left as they are
    void markVisibleMeshes(size_t sector, std::vector<bool> &visible) const;

  private:
    static void markSectorMeshes(const VisSector &sector, std::vector<bool> &visible);

    std::vector<VisSector> sectors_;
};

} // namespace argon
