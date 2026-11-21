// Argon: the crafts' engine trails (points added by 0x50960, faded by 0x9db10, drawn by 0x9dca0): two trails per craft
// (left / right, at the craft -+ 2 x right - 4 x forward), 96-point rings; every point carries a vertical (+-0.5 x up)
// and a horizontal (+-right) cross section. Colour from |forward speed| x 1.1 (blue -> red), written 3 updates late,
// all colours fade by 1 per update; drawn with newtrail.dds twice, additive.
#pragma once
#include "effects/effect_slots.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>

namespace argon
{

struct Vertex;

// what colours a trail: the craft's forward speed and whether its turbo is on (+0x9f1)
struct TrailDrive
{
    float forwardSpeed;
    bool turbo;
};

class EngineTrails
{
  public:
    static constexpr int kPoints = 96;   // 0x50960 / 0x9db10: the points of a ring
    static constexpr int kTrailsPerCraft = 2;
    // two ribbons of two triangles between each pair of points
    static constexpr uint32_t kMaxVertices = uint32_t(kEffectSlots) * kTrailsPerCraft * (kPoints - 1) * 12;

    void reset();
    // one 60 Hz update of the craft in `slot` (matrix columns right, up, forward, position)
    void follow(int slot, const glm::mat4 &matrix, const TrailDrive &drive);
    // the ribbons of every trail point written and still lit; returns the vertices written
    uint32_t writeRibbons(Vertex *out) const;

  private:
    struct Point
    {
        glm::vec3 pos{0.f}, vertical{0.f}, horizontal{0.f};
        uint8_t rgb[3]{};
    };
    using Ring = std::array<Point, kPoints>;

    void fade(int slot);
    uint32_t writeSegment(const Ring &ring, int first, Vertex *out) const;

    std::array<std::array<Ring, kTrailsPerCraft>, kEffectSlots> rings_{};
    int heads_[kEffectSlots][kTrailsPerCraft]{};
};

}  // namespace argon
