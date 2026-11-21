#include "track/track_sprites.hpp"

#include "original/record_read.hpp"
#include "original/track_records.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{
namespace
{
const float kCullDistance = 1500.f;   // 0x1a3700; list B compares the squared distance with 0x1a34b8 = 2.25e6
const int kLastBucket = 4999;         // 0x420228: 5000 bucket heads
const float kHalf = 0.5f;             // 0x1a3504
const size_t kBlendedSlots = 2000, kAdditiveSlots = 0x800;   // 0xbdf40 logs "Too many ... quadsprites" past these

// v0..v3 <- the record's corner offsets 2, 3, 1, 0 (0xbdf40)
const int kOffsetOfVertex[4] = {2, 3, 1, 0};
// v0..v3 <- the record's uv floats (u, v): (+0x54, +0x70), (+0x5c, +0x68), (+0x6c, +0x58), (+0x64, +0x60)
const int kUvOfVertex[4][2] = {{0, 7}, {2, 5}, {6, 1}, {4, 3}};

glm::vec3 vec3Of(const float *xyz)
{
    return glm::vec3(xyz[0], xyz[1], xyz[2]);
}

TrackSprite spriteOf(const original::QuadSprite &record)
{
    TrackSprite sprite;
    sprite.pos = vec3Of(record.position);
    sprite.facing = record.frame >= 0 && record.frame <= 2 ? SpriteFacing(record.frame) : SpriteFacing::World;
    sprite.additive = (record.flags & 1u) != 0;
    for (int v = 0; v < 4; ++v)
    {
        TrackSpriteCorner &corner = sprite.corners[size_t(v)];
        corner.offset = vec3Of(record.cornerOffsets[kOffsetOfVertex[v]]);
        corner.uv = glm::vec2(record.cornerUvs[kUvOfVertex[v][0]], record.cornerUvs[kUvOfVertex[v][1]] * kHalf + kHalf);
        corner.argb = record.cornerColours[v];
    }
    return sprite;
}

// 0xbdf40 counts the blended and the additive quads apart
struct PoolCounts
{
    size_t blended = 0, additive = 0;

    bool take(const TrackSprite &sprite)
    {
        size_t &used = sprite.additive ? additive : blended;
        if (used >= (sprite.additive ? kAdditiveSlots : kBlendedSlots))
        {
            return false;
        }
        ++used;
        return true;
    }
};

float largestAxisDistance(const glm::vec3 &first, const glm::vec3 &second)
{
    const glm::vec3 distance = glm::abs(first - second);
    return std::max(distance.x, std::max(distance.y, distance.z));
}

}  // namespace

std::vector<TrackSprite> readTrackSprites(const std::vector<uint8_t> &file)
{
    std::vector<TrackSprite> sprites;
    PoolCounts counts;
    const auto header = original::readAt<original::QuadSpriteHeader>(file, 0);
    constexpr size_t kFirst = sizeof(original::QuadSpriteHeader), kSize = sizeof(original::QuadSprite);
    for (uint32_t i = 0; i < header.count && kFirst + (i + 1) * kSize <= file.size(); ++i)
    {
        const TrackSprite sprite = spriteOf(original::readAt<original::QuadSprite>(file, kFirst + i * kSize));
        if (counts.take(sprite))
        {
            sprites.push_back(sprite);
        }
    }
    return sprites;
}

glm::vec3 spriteCornerPosition(const TrackSprite &sprite, int corner, const SpriteBasis &basis)
{
    return sprite.pos + spriteCornerOffset(sprite.corners[size_t(corner)].offset, SpriteTurn{}, basis);
}

std::vector<int> blendedSpriteOrder(const std::vector<TrackSprite> &sprites, const glm::vec3 &eye)
{
    std::vector<std::vector<int>> buckets(size_t(kLastBucket) + 1);
    for (size_t i = 0; i < sprites.size(); ++i)
    {
        const float distance = largestAxisDistance(sprites[i].pos, eye);
        if (sprites[i].additive || distance >= kCullDistance)
        {
            continue;
        }
        buckets[size_t(std::min(int(std::nearbyint(distance)), kLastBucket))].push_back(int(i));
    }
    std::vector<int> order;
    for (int b = kLastBucket; b > 0; --b)
    {
        order.insert(order.end(), buckets[size_t(b)].rbegin(), buckets[size_t(b)].rend());
    }
    return order;
}

std::vector<int> additiveSpriteOrder(const std::vector<TrackSprite> &sprites, const glm::vec3 &eye)
{
    std::vector<int> order;
    for (size_t i = 0; i < sprites.size(); ++i)
    {
        const glm::vec3 offset = sprites[i].pos - eye;
        if (sprites[i].additive && glm::dot(offset, offset) < kCullDistance * kCullDistance)
        {
            order.push_back(int(i));
        }
    }
    return order;
}

}  // namespace argon
