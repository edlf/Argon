#include "logic/trail_sprite.hpp"

#include <algorithm>
#include <cmath>

namespace argon::weapons
{
namespace
{
const uint32_t kRandomSpin = 1, kAlphaBlended = 2, kFadeIn = 4;
const float kRandomScale = 1.f / 1023.f;   // 0x1a3718
const float kGrowthRate = 20.f;            // 0x1a35d8
const float kCellPixels = 128.f;
const int kCellsPerRow = 4;

int channel(uint32_t argb, int shift)
{
    return int((argb >> shift) & 0xffu);
}

// 0x9c540: k = clamp(round(part x 255), 0, 255), rounded as fistp (to even)
int blendWeight(float part)
{
    return std::clamp(int(std::nearbyint(part * 255.f)), 0, 255);
}

// (from x (255 - k) + to x k) >> 8 per channel
uint32_t blendColours(uint32_t from, uint32_t to, int share)
{
    uint32_t out = 0;
    for (int shift = 0; shift < 32; shift += 8)
    {
        out |= uint32_t((channel(from, shift) * (255 - share) + channel(to, shift) * share) >> 8) << shift;
    }
    return out;
}

uint32_t scaleChannels(uint32_t argb, int scale, uint32_t mask)
{
    uint32_t out = argb & ~mask;
    for (int shift = 0; shift < 32; shift += 8)
    {
        if ((mask >> shift) & 0xffu)
        {
            out |= uint32_t((channel(argb, shift) * scale) >> 8) << shift;
        }
    }
    return out;
}
}  // namespace

bool TrailSprite::advance(float now, float dt)
{
    if (now - born >= life)
    {
        return false;
    }
    pos += vel * dt;
    halfSize *= 1.f + (grow - 1.f) * dt * dt * kGrowthRate;
    return true;
}

uint32_t TrailSprite::colourAt(float now) const
{
    const float age = now - born;
    const float sinceFade = age - fade;
    if (sinceFade >= 0.f)
    {
        return blendColours(from, to, blendWeight(sinceFade / (life - fade)));
    }
    if (!fadeIn || fade == 0.f)
    {
        return from;
    }
    const uint32_t faded = additive ? 0x00ffffffu : 0xff000000u;
    return scaleChannels(from, blendWeight(age / fade), faded);
}

float TrailSprite::angleAt(float now) const
{
    return glm::radians((now - born) * spin + startAngle);
}

TrailSprite makeTrailSprite(const TrailSpriteStyle &style, const glm::vec3 &at, const glm::vec3 &vel, float now, unsigned random10a, unsigned random10b,
                            bool flip)
{
    TrailSprite sprite;
    sprite.pos = at;
    sprite.vel = vel;
    sprite.born = now;
    sprite.life = style.life;
    sprite.fade = style.fade;
    sprite.from = style.from;
    sprite.to = style.to;
    sprite.cell = style.cell;
    sprite.grow = style.grow;
    sprite.spin = style.spin + float(random10a & 0x3ffu) * kRandomScale * style.spinRange;
    if ((style.flags & kRandomSpin) && flip)
    {
        sprite.spin = -sprite.spin;
    }
    sprite.halfSize = style.size + float(random10b & 0x3ffu) * kRandomScale * style.sizeRange;
    sprite.additive = !(style.flags & kAlphaBlended);
    sprite.fadeIn = (style.flags & kFadeIn) != 0;
    return sprite;
}

glm::vec2 trailSpriteCell(int cell)
{
    return glm::vec2(float(cell % kCellsPerRow), float(cell / kCellsPerRow)) * kCellPixels;
}

}  // namespace argon::weapons
