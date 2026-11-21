// Argon: the crafts' water spray (the wake part of 0x50960 -> 0x9d620, updated by 0x9c540). While both rear box
// corners are under water, at 0, 2, 4 .. m along each update's displacement (logic\water_wake.hpp) a puff each side
// at the step - 5 x forward -+ 2 x right (+-2.55 m jitter in x / z; height wakeSpriteHeight). Life 0.5..1.53 s,
// alpha 16..79 fading to 0 over the last 15 %, half-width 1.2..1.8 (random mirror), height sin(age / life x pi) x
// 2.4..4.8; quadsprite.dds cell (0, 384). Droplet sheets (0x9d2c0, a second pool, updated inline in 0x9ca50): at
// each step, 50 % per side, a sheet at the step - 4 x forward -+ 1.2 x right (height wakeSpriteHeight), velocity =
// the displacement since the last update -+ 15 x right + (64..95) x 0.04 up, gravity -10. Corners base -+ 4 x
// forward and base + 8 x normalize(velocity) -+ 4 x forward; life 0.5 s, alpha 32..63 held 0.2 s then fading to 0;
// quadsprite.dds cell (128, 384).
#pragma once
#include "effects/effect_slots.hpp"
#include "logic/viewer_rand.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace argon
{

struct Vertex;

// where the water is: 0xce6b0's out value at a point, when a water box holds it
class WaterQuery
{
  public:
    virtual ~WaterQuery() = default;
    virtual std::optional<float> depthAt(const glm::vec3 &point) const = 0;
};

// one craft's update: its slot, its matrix (columns right, up, forward, position), both rear corners in water
// (argon::kWakeCornersNeeded)
struct WakeCraft
{
    int slot;
    glm::mat4 matrix;
    bool inWater;
};

// what an update draws from: the effects' clock, the viewer's rand, the water
struct WakeStep
{
    float now;
    ViewerRand &rand;
    const WaterQuery &water;
};

class WakeSpray
{
  public:
    static constexpr int kMax = 4096;   // the game's puff and droplet pools hold 4096 each
    static constexpr uint32_t kMaxVertices = uint32_t(kMax) * 12;

    // no puffs or drops, and every craft's next update counts as its first in water
    void clear();
    // once per 60 Hz step after the clock moved: old puffs and drops go, the drops fall
    void age(float now);
    // no walk on a craft's first update in water (+0x94), then from its last update's position (+0xd0)
    void follow(const WakeCraft &craft, const WakeStep &step);
    size_t puffs() const;
    size_t drops() const;
    bool empty() const;
    // a puff or a droplet sheet close enough to wet the lens at `eye` (logic\rain.hpp sprayReachesLens)
    bool reachesLens(const glm::vec3 &eye) const;
    // the puffs (upright quads), then the drops after them up to kMaxVertices in all; each returns the new total
    uint32_t writePuffs(const glm::mat4 &view, float now, Vertex *out) const;
    uint32_t writeDrops(float now, Vertex *out, uint32_t written) const;

  private:
    struct Puff
    {
        glm::vec3 pos;
        float born, life, fade, halfWidth, height;
        uint8_t alpha;
    };
    struct Drop
    {
        glm::vec3 pos, vel, side;
        float born;
        uint8_t alpha;
    };
    struct StepPoint
    {
        glm::mat4 matrix;
        glm::vec3 at, displacement;
    };

    void emitStep(const StepPoint &point, const WakeStep &step);
    void addPuff(const StepPoint &point, float side, const WakeStep &step);
    void addDrop(const StepPoint &point, float side, const WakeStep &step);

    std::vector<Puff> puffs_;
    std::vector<Drop> drops_;
    glm::vec3 lastPos_[kEffectSlots]{};
    bool hadWater_[kEffectSlots]{};
};

}  // namespace argon
