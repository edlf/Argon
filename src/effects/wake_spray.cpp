// Argon: the crafts' water spray (effects\wake_spray.hpp; 0x50960, 0x9c540, 0x9d620, 0x9d2c0).
#include "effects/wake_spray.hpp"
#include "logic/rain.hpp"
#include "logic/water_wake.hpp"
#include "model/cmm.hpp"
#include "render/sprite_pool.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{
namespace
{
const float kStepSeconds = 1.f / 60.f;
const float kDropLife = 0.5f, kDropHold = 0.2f, kDropFade = 0.3f, kDropGravity = -10.f;
const float kSpriteCellPixels = 128.f;
const glm::vec2 kPuffCell(0.f, 384.f), kDropCell(128.f, 384.f);   // 0x9d620 / the wake's 0x9d2c0 (cell 13)

Vertex whiteSpriteVertex(const glm::vec3 &at, const glm::vec2 &uv, uint8_t alpha)
{
    Vertex vertex{};
    vertex.pos[0] = at.x;
    vertex.pos[1] = at.y;
    vertex.pos[2] = at.z;
    vertex.uv[0][0] = uv.x;
    vertex.uv[0][1] = uv.y;
    vertex.rgba[0] = vertex.rgba[1] = vertex.rgba[2] = 255;
    vertex.rgba[3] = alpha;
    return vertex;
}

void writeWhiteQuad(Vertex *out, const glm::vec3 (&corners)[kSpriteCorners], const SpriteCellUv &cell, uint8_t alpha)
{
    for (int k = 0; k < kSpriteTriangleVertices; ++k)
    {
        const int corner = kSpriteTriangleCorners[k];
        out[k] = whiteSpriteVertex(corners[corner], cell.at(corner), alpha);
    }
}
}  // namespace

void WakeSpray::clear()
{
    puffs_.clear();
    drops_.clear();
    for (bool &hadWater : hadWater_)
    {
        hadWater = false;
    }
}

void WakeSpray::age(float now)
{
    for (size_t i = 0; i < puffs_.size();)
    {
        if (now - puffs_[i].born >= puffs_[i].life)
        {
            puffs_[i] = puffs_.back();
            puffs_.pop_back();
        }
        else
        {
            ++i;
        }
    }
    for (size_t i = 0; i < drops_.size();)
    {
        Drop &drop = drops_[i];
        if (now - drop.born >= kDropLife)
        {
            drop = drops_.back();
            drops_.pop_back();
            continue;
        }
        drop.pos += drop.vel * kStepSeconds;
        drop.vel.y += kDropGravity * kStepSeconds;
        ++i;
    }
}

void WakeSpray::follow(const WakeCraft &craft, const WakeStep &step)
{
    const int slot = std::clamp(craft.slot, 0, kEffectSlots - 1);
    const glm::vec3 pos(craft.matrix[3]);
    if (craft.inWater && hadWater_[slot])
    {
        const glm::vec3 displacement = pos - lastPos_[slot];
        const glm::vec3 dir = glm::length(displacement) > 0.f ? glm::normalize(displacement) : glm::vec3(0.f);
        for (float distance : wakeStepDistances(displacement))
        {
            emitStep(StepPoint{craft.matrix, lastPos_[slot] + dir * distance, displacement}, step);
        }
    }
    lastPos_[slot] = pos;
    hadWater_[slot] = craft.inWater;
}

void WakeSpray::emitStep(const StepPoint &point, const WakeStep &step)
{
    for (float side : {-1.f, 1.f})
    {
        addPuff(point, side, step);
        if ((step.rand.next() & 1) == 0)
        {
            addDrop(point, side, step);
        }
    }
}

// the rand expressions here keep the shape of the code they came from: two rand() calls in one expression are
// evaluated in the compiler's order, and the spray's stream is shared with the other effects
// 0x50dec..0x50f8c: at the step - 5 x forward -+ 2 x right, +-2.55 m jitter in x / z; the height from the craft's
// own position (not the step's): y - 4 x forward.y -+ 2 x right.y, plus the water query's out value, - 1.5
void WakeSpray::addPuff(const StepPoint &point, float side, const WakeStep &step)
{
    ViewerRand &rand = step.rand;
    const glm::vec3 right(point.matrix[0]), fwd(point.matrix[2]), pos(point.matrix[3]);
    glm::vec3 centre = point.at - fwd * 5.f + side * 2.f * right;
    centre.x += float(rand.next() & 255) * 0.01f - float(rand.next() & 255) * 0.01f;
    centre.z += float(rand.next() & 255) * 0.01f - float(rand.next() & 255) * 0.01f;
    centre.y = wakeSpriteHeight(pos.y - fwd.y * 4.f + side * 2.f * right.y, centre.y, step.water.depthAt(centre));
    Puff puff;
    puff.pos = centre;
    puff.life = float(rand.next() & 31) * (1.f / 30.f) + 0.5f;
    puff.fade = puff.life * 0.85f;
    puff.halfWidth = (float(rand.next() & 31) * (1.f / 30.f) + 2.f) * 0.6f * ((rand.next() & 1) ? -1.f : 1.f);
    puff.height = (float(rand.next() & 31) * 0.125f + 4.f) * 0.6f;
    puff.alpha = uint8_t((rand.next() & 0x3f) + 0x10);
    puff.born = step.now;
    if (int(puffs_.size()) < kMax)
    {
        puffs_.push_back(puff);
    }
}

// 0x50fa0..0x51100: at the step - 4 x forward -+ 1.2 x right, its height the query's own y + the out value - 1.5
void WakeSpray::addDrop(const StepPoint &point, float side, const WakeStep &step)
{
    ViewerRand &rand = step.rand;
    const glm::vec3 right(point.matrix[0]), fwd(point.matrix[2]);
    Drop drop;
    drop.pos = point.at - fwd * 4.f + side * 1.2f * right;
    drop.pos.y = wakeSpriteHeight(drop.pos.y, drop.pos.y, step.water.depthAt(drop.pos));
    drop.vel = point.displacement + side * 15.f * right;
    drop.vel.y += float((rand.next() & 31) + 64) * 0.04f;
    drop.side = fwd * 4.f;
    drop.alpha = uint8_t((rand.next() & 31) + 32);
    drop.born = step.now;
    if (int(drops_.size()) < kMax)
    {
        drops_.push_back(drop);
    }
}

size_t WakeSpray::puffs() const
{
    return puffs_.size();
}

size_t WakeSpray::drops() const
{
    return drops_.size();
}

bool WakeSpray::empty() const
{
    return puffs_.empty() && drops_.empty();
}

bool WakeSpray::reachesLens(const glm::vec3 &eye) const
{
    for (const Puff &puff : puffs_)
    {
        if (rain::sprayReachesLens(eye, puff.pos))
        {
            return true;
        }
    }
    for (const Drop &sheet : drops_)
    {
        if (rain::sprayReachesLens(eye, sheet.pos))
        {
            return true;
        }
    }
    return false;
}

// 0xbd670 EAX 1 (upright: the view's flattened x axis, y up), 0x9c540's mode 1 offsets, not turned
uint32_t WakeSpray::writePuffs(const glm::mat4 &view, float now, Vertex *out) const
{
    const SpriteBasis upright = spriteBasis(SpriteFacing::Upright, view);
    const SpriteCellUv cell = SpriteCellUv::of(kPuffCell, kSpriteCellPixels);
    uint32_t written = 0;
    for (const Puff &puff : puffs_)
    {
        const float age = now - puff.born;
        const float height = std::sin(std::clamp(age / puff.life, 0.f, 1.f) * 3.14159265f) * puff.height;
        float alpha = puff.alpha;
        if (age >= puff.fade)
        {
            alpha = puff.alpha * (1.f - std::clamp((age - puff.fade) / std::max(1e-3f, puff.life - puff.fade), 0.f, 1.f));
        }
        glm::vec3 corners[kSpriteCorners];
        for (int c = 0; c < kSpriteCorners; ++c)
        {
            corners[c] = puff.pos + spriteCornerOffset(standingCornerOffset(puff.halfWidth, height, c), SpriteTurn{}, upright);
        }
        writeWhiteQuad(out + written, corners, cell, uint8_t(alpha));
        written += kSpriteTriangleVertices;
    }
    return written;
}

// EAX 0 (world offsets): -+ side at the base, the streak -+ side at the head
uint32_t WakeSpray::writeDrops(float now, Vertex *out, uint32_t written) const
{
    const SpriteCellUv cell = SpriteCellUv::of(kDropCell, kSpriteCellPixels);
    for (const Drop &drop : drops_)
    {
        if (written + kSpriteTriangleVertices > kMaxVertices)
        {
            break;
        }
        const float age = now - drop.born;
        float alpha = drop.alpha;
        if (age >= kDropHold)
        {
            alpha = drop.alpha * (1.f - std::clamp((age - kDropHold) / kDropFade, 0.f, 1.f));
        }
        const float length = glm::length(drop.vel);
        const glm::vec3 streak = length > 1e-4f ? drop.vel * (8.f / length) : glm::vec3(0.f);
        const glm::vec3 corners[kSpriteCorners] = {drop.pos - drop.side, drop.pos + drop.side, drop.pos + streak - drop.side, drop.pos + streak + drop.side};
        writeWhiteQuad(out + written, corners, cell, uint8_t(alpha));
        written += kSpriteTriangleVertices;
    }
    return written;
}

}  // namespace argon
