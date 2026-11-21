// Argon: the spray pool's sprites and the one-frame weapon flashes (effects\weapon_sprites.hpp; 0x9d000, 0x9c540,
// 0xbd670, 0xbd8f0).
#include "effects/weapon_sprites.hpp"
#include "model/cmm.hpp"

namespace argon
{
namespace
{
const float kCellPixels = 128.f;                   // 0x9d000 / 0x68150 / 0x68620: 0xbd670's cell size
const glm::vec2 kFlashCell(256.f, 256.f);          // the weapon flashes' cell (cell 10)

uint8_t channelOf(uint32_t argb, int shift)
{
    return uint8_t((argb >> shift) & 0xffu);
}

Vertex spriteVertex(const glm::vec3 &at, const glm::vec2 &uv, uint32_t argb)
{
    Vertex vertex{};
    vertex.pos[0] = at.x;
    vertex.pos[1] = at.y;
    vertex.pos[2] = at.z;
    vertex.uv[0][0] = uv.x;
    vertex.uv[0][1] = uv.y;
    vertex.rgba[0] = channelOf(argb, 16);
    vertex.rgba[1] = channelOf(argb, 8);
    vertex.rgba[2] = channelOf(argb, 0);
    vertex.rgba[3] = channelOf(argb, 24);
    return vertex;
}

// a camera-facing square (0xbd670, EAX 2) as its two triangles
struct SquareSprite
{
    glm::vec3 pos{0.f};
    float halfSize = 0.f;
    SpriteTurn turn;
    SpriteCellUv cell;
    uint32_t argb = 0;
};

void writeSquare(Vertex *out, const SquareSprite &square, const SpriteBasis &basis)
{
    for (int k = 0; k < kSpriteTriangleVertices; ++k)
    {
        const int corner = kSpriteTriangleCorners[k];
        const glm::vec3 at = square.pos + spriteCornerOffset(squareCornerOffset(square.halfSize, corner), square.turn, basis);
        out[k] = spriteVertex(at, square.cell.at(corner), square.argb);
    }
}

// 0x9d000 -> 0xbd670, 0x9c540 each frame: the square turned by the sprite's angle
SquareSprite trailSquare(const weapons::TrailSprite &sprite, float now)
{
    const SpriteCellUv cell = SpriteCellUv::of(weapons::trailSpriteCell(sprite.cell), kCellPixels);
    return SquareSprite{sprite.pos, sprite.halfSize, SpriteTurn::ofAngle(sprite.angleAt(now)), cell, sprite.colourAt(now)};
}

// 0xbd670 alone: the flash keeps its allocation angle's slots
SquareSprite flashSquare(const weapons::WeaponFlash &flash)
{
    const SpriteTurn turn = SpriteTurn::ofAllocation(glm::radians(flash.angleDegrees));
    return SquareSprite{flash.pos, flash.halfSize, turn, SpriteCellUv::of(kFlashCell, kCellPixels), flash.argb};
}
}  // namespace

bool WeaponSprites::hasRoom() const
{
    return int(sprites_.size()) < kMaxSprites;
}

void WeaponSprites::add(const weapons::TrailSprite &sprite)
{
    if (hasRoom())
    {
        sprites_.push_back(sprite);
    }
}

void WeaponSprites::addFlash(const weapons::WeaponFlash &flash)
{
    if (int(flashes_.size()) < kMaxFlashes)
    {
        flashes_.push_back(flash);
    }
}

void WeaponSprites::age(float now, float step)
{
    for (size_t i = 0; i < sprites_.size();)
    {
        if (sprites_[i].advance(now, step))
        {
            ++i;
            continue;
        }
        sprites_[i] = sprites_.back();
        sprites_.pop_back();
    }
}

void WeaponSprites::dropFlashes()
{
    flashes_.clear();
}

void WeaponSprites::clear()
{
    sprites_.clear();
    flashes_.clear();
}

size_t WeaponSprites::sprites() const
{
    return sprites_.size();
}

bool WeaponSprites::empty() const
{
    return sprites_.empty() && flashes_.empty();
}

// camera-facing quads turned by their spin (the game's turn and mirrored x axis): list A first, then list B and the
// flashes
WeaponSprites::Written WeaponSprites::writeQuads(const glm::mat4 &view, float now, Vertex *out) const
{
    const Frame frame{spriteBasis(SpriteFacing::Camera, view), now};
    Written written;
    written.alpha = writeList(SpriteList::Alpha, frame, out);
    written.additive = writeList(SpriteList::Additive, frame, out + written.alpha);
    written.additive += writeFlashes(frame, out + written.alpha + written.additive);
    return written;
}

uint32_t WeaponSprites::writeList(SpriteList list, const Frame &frame, Vertex *out) const
{
    const bool additive = list == SpriteList::Additive;
    uint32_t written = 0;
    for (const weapons::TrailSprite &sprite : sprites_)
    {
        if (sprite.additive != additive)
        {
            continue;
        }
        writeSquare(out + written, trailSquare(sprite, frame.now), frame.basis);
        written += kSpriteTriangleVertices;
    }
    return written;
}

uint32_t WeaponSprites::writeFlashes(const Frame &frame, Vertex *out) const
{
    uint32_t written = 0;
    for (const weapons::WeaponFlash &flash : flashes_)
    {
        writeSquare(out + written, flashSquare(flash), frame.basis);
        written += kSpriteTriangleVertices;
    }
    return written;
}

}  // namespace argon
