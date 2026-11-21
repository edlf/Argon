// Argon: a craft's flashes (docs\weapons\hud.md "Flashes"). The hit's red flash on a human player's position list row:
// craft weapon +0x98, raised by a hit that takes shield (0x6d430), faded by craft_weapon_update (0x6d3a7), drawn by
// hud_minimap_draw (0x4fafe) as the red of an additive quad. The shield flash, craft weapon +0x94 (huddata +0x70),
// follows the same rules (0x6d400, faded at 0x6d372), also raised by the corrode; the HUD's shield fill blinks white
// while it is above 0 (gamehud_render 0x4b1dc).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace argon::weapons
{
class HitFlash
{
  public:
    static constexpr float kPerHit = 0.8f;            // 0x6b19a: weapon_damage's amount, with shield damage out 0 > 0
    static constexpr float kPerCorrodeFrame = 0.2f;   // 0x6d332: each frame the corrode eats a shield above 0 (+0x94 only)

    // 0x6d430: += amount, capped at 1
    void raise(float amount)
    {
        level_ = std::min(level_ + amount, 1.f);
    }
    // 0x6d3a7: -= dt while above 0, then 0 (not while the craft is knocked out: 0x6d030 skips to 0x6d3dc)
    void fade(float dt)
    {
        if (level_ > 0.f)
        {
            level_ = std::max(level_ - dt, 0.f);
        }
    }
    void clear()
    {
        level_ = 0.f;
    }
    float level() const
    {
        return level_;
    }
    // 0x4fafe: the quad's red byte, level x 254 rounded to nearest (fistp)
    uint8_t red() const
    {
        return static_cast<uint8_t>(std::lrint(level_ * 254.f));
    }
    // 0x4fb26: ARGB, green and blue 0, alpha 0xff
    uint32_t colour() const
    {
        return 0xff000000u | uint32_t(red()) << 16;
    }

  private:
    float level_ = 0.f;
};
}  // namespace argon::weapons
