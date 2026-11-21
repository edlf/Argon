// Argon: the spray pool's camera-facing sprites (0x9d000 puts them there, 0x9c540 moves, grows, spins and colours
// them, the sprite pass 0xbd8f0 draws list A alpha blended and list B additive): the weapons' trail sprites (trail
// types > 3, docs\weapons\firing.md "Trail sprites"), the hover's dust (docs\effects\README.md "Dust") and the explosions' flashes;
// plus the beams' and sparks' one-frame flashes (0x68620 / 0x68150 -> 0xbd670), dropped at the next weapon step
// (0xbd5f0) and drawn after list B.
#pragma once
#include "logic/trail_sprite.hpp"
#include "logic/weapon_flash.hpp"
#include "render/sprite_pool.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace argon
{

struct Vertex;

class WeaponSprites
{
  public:
    static constexpr int kMaxSprites = 2048;
    static constexpr int kMaxFlashes = 256;
    static constexpr uint32_t kMaxVertices = uint32_t(kMaxSprites + kMaxFlashes) * 6;

    // the vertices written: list A's (alpha blended) first, then list B's and the flashes (additive)
    struct Written
    {
        uint32_t alpha = 0, additive = 0;
    };

    // full: the pool takes no more (the callers roll their random numbers only when it has room)
    bool hasRoom() const;
    void add(const weapons::TrailSprite &sprite);
    void addFlash(const weapons::WeaponFlash &flash);
    // once per 60 Hz step
    void age(float now, float step);
    void dropFlashes();
    void clear();
    size_t sprites() const;
    bool empty() const;
    Written writeQuads(const glm::mat4 &view, float now, Vertex *out) const;

  private:
    enum class SpriteList
    {
        Alpha,      // list A
        Additive    // list B
    };
    // what every quad of a frame shares: the camera's facing and the clock
    struct Frame
    {
        SpriteBasis basis;
        float now;
    };
    uint32_t writeList(SpriteList list, const Frame &frame, Vertex *out) const;
    uint32_t writeFlashes(const Frame &frame, Vertex *out) const;

    std::vector<weapons::TrailSprite> sprites_;
    std::vector<weapons::WeaponFlash> flashes_;
};

}  // namespace argon
