// Argon: the point particles (logic\particles.hpp; 0xbc640 update, 0xbc900 draw, 0xbcb60 add, bursts 0x88750).
#include "logic/particles.hpp"
#include "model/cmm.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{
namespace
{
const float kStepSeconds = 1.f / 60.f;

// a colour between two ARGB colours by t (each channel on its own)
uint32_t blendedColour(uint32_t from, uint32_t to, float share)
{
    uint32_t blended = 0;
    for (int shift = 0; shift < 32; shift += 8)
    {
        const float fromChannel = float((from >> shift) & 0xff), toChannel = float((to >> shift) & 0xff);
        blended |= uint32_t(uint8_t(std::clamp(fromChannel + (toChannel - fromChannel) * share, 0.f, 255.f))) << shift;
    }
    return blended;
}

// a camera-facing square of half size `half` around `centre`, as two triangles; additive: the colour carries the fade
// (alpha scales it, as the game's alpha x additive point sprites)
struct Billboard
{
    glm::vec3 centre, right, up;
    float half;
    uint32_t argb;
};

Vertex billboardCorner(const Billboard &quad, float across, float upward, const glm::vec2 &uv)
{
    Vertex corner{};
    const glm::vec3 at = quad.centre + quad.right * across + quad.up * upward;
    corner.pos[0] = at.x;
    corner.pos[1] = at.y;
    corner.pos[2] = at.z;
    corner.uv[0][0] = uv.x;
    corner.uv[0][1] = uv.y;
    const uint32_t alpha = quad.argb >> 24;
    corner.rgba[0] = uint8_t(((quad.argb >> 16) & 0xff) * alpha / 255);
    corner.rgba[1] = uint8_t(((quad.argb >> 8) & 0xff) * alpha / 255);
    corner.rgba[2] = uint8_t((quad.argb & 0xff) * alpha / 255);
    corner.rgba[3] = 255;
    return corner;
}

void writeBillboard(const Billboard &quad, Vertex *out)
{
    const float half = quad.half;
    out[0] = billboardCorner(quad, -half, -half, {0, 1});
    out[1] = billboardCorner(quad, half, -half, {1, 1});
    out[2] = billboardCorner(quad, -half, half, {0, 0});
    out[3] = billboardCorner(quad, half, -half, {1, 1});
    out[4] = billboardCorner(quad, half, half, {1, 0});
    out[5] = billboardCorner(quad, -half, half, {0, 0});
}

// the mega-up's colour per direction: red, then yellow or blue on a coin each
uint32_t megaUpColour(ViewerRand &rand)
{
    uint32_t colour = 0xffff3030u;
    if (rand.next() & 1)
    {
        colour = 0xffffff30u;
    }
    if (rand.next() & 1)
    {
        colour = 0xff3030ffu;
    }
    return colour;
}

// (rand & 511) x 0.7045 degrees
float burstAngle(ViewerRand &rand)
{
    const float degree = 3.14159265f / 180.f;
    return float(rand.next() & 0x1ff) * 0.7045f * degree;
}

float bubbleJitter(ViewerRand &rand)
{
    return float(int(rand.next() & 255) - 127);
}
}  // namespace

void Particles::add(const Particle &particle)
{
    if (hasRoom())
    {
        alive_.push_back(particle);
    }
}

// 0x88750: 75 random directions (two angles); per direction a duration of 0.5 + (rand & 127) / 255 s and two rounds
// of three particles: velocity speed x (cos a cos b, cos a sin b, sin a), x 1.1 after every particle and x 3 after each
// round; the colour halves per particle and resets per round. Collected: implode onto the moving ship, speed 10.
// Respawn: implode onto the pickup, speed 20.
void Particles::burst(const ParticleBurst &burst, float now, ViewerRand &rand)
{
    for (int direction = 0; direction < 75; ++direction)
    {
        const float yaw = burstAngle(rand), pitch = burstAngle(rand);
        glm::vec3 velocity = burst.speed * glm::vec3(std::cos(yaw) * std::cos(pitch), std::cos(yaw) * std::sin(pitch), std::sin(yaw));
        const float life = float(rand.next() & 0x7f) * (1.f / 255.f) + 0.5f;
        for (int round = 0; round < 2; ++round)
        {
            uint32_t colour = burst.colour == 0 ? megaUpColour(rand) : burst.colour;
            for (int k = 0; k < 3; ++k)
            {
                Particle particle;
                particle.vel = velocity;
                particle.src = burst.src;
                particle.ship = burst.followSlot;
                particle.born = now;
                particle.dur = life;
                particle.colour = colour;
                add(particle);
                velocity *= 1.1f;
                colour = (colour & 0xff000000u) | ((colour >> 1) & 0x007f7f7fu);
            }
            velocity *= 3.f;
        }
    }
}

// x / z +-64, y +-12.7 ((rand & 255) - 127 x 0.5 / 0.1), rising 0.1 m/s, white fading out from 0.64 s to 0.8 s; the
// bubble system holds 1000 (neon_add_particle 0xbcb60 drops the rest)
void Particles::emitBubbles(const glm::vec3 &camera, float now, ViewerRand &rand)
{
    const int kBubbleSystem = 2, kBubbleCap = 1000, kPerFrame = 30;
    int bubbles = 0;
    for (const Particle &particle : alive_)
    {
        bubbles += particle.sys == kBubbleSystem;
    }
    for (int i = 0; i < kPerFrame && bubbles < kBubbleCap; ++i, ++bubbles)
    {
        Particle bubble;
        bubble.src = camera;
        bubble.src.x += bubbleJitter(rand) * 0.5f;
        bubble.src.y += bubbleJitter(rand) * 0.1f;
        bubble.src.z += bubbleJitter(rand) * 0.5f;
        bubble.vel = glm::vec3(0.f, 0.1f, 0.f);
        bubble.ship = -1;
        bubble.born = now;
        bubble.dur = 0.8f;
        bubble.delay = 0.64f;
        bubble.from = 0xffffffffu;
        bubble.colour = 0;
        bubble.sys = kBubbleSystem;
        bubble.implode = false;
        alive_.push_back(bubble);
    }
}

// 0xbc640: pos += vel dt, vel.y += gravity dt; a particle goes at the end of its life
void Particles::step(std::span<const glm::vec3> slotPositions, float now)
{
    static const float gravity[kSystems] = {-10.f, -13.f, 0.f};
    for (Particle &particle : alive_)
    {
        if (particle.ship >= 0)
        {
            particle.src = size_t(particle.ship) < slotPositions.size() ? slotPositions[size_t(particle.ship)] : glm::vec3(0.f);
        }
        else if (!particle.implode)
        {
            particle.src += particle.vel * kStepSeconds;
            particle.vel.y += gravity[std::clamp(particle.sys, 0, kSystems - 1)] * kStepSeconds;
        }
    }
    for (size_t i = 0; i < alive_.size();)
    {
        if (now - alive_[i].born >= alive_[i].dur)
        {
            alive_[i] = alive_.back();
            alive_.pop_back();
        }
        else
        {
            ++i;
        }
    }
}

bool Particles::hasRoom() const
{
    return alive_.size() < size_t(kMax);
}

void Particles::clear()
{
    alive_.clear();
}

size_t Particles::size() const
{
    return alive_.size();
}

bool Particles::empty() const
{
    return alive_.empty();
}

// 0xbc640: position = source + velocity x (1 - t) when imploding, colour `from` -> `colour` after the delay; drawn
// (0xbc900) as point sprites of its system's size / distance x screen height, 4..16 px, additive, no fog
Particles::Ranges Particles::writeQuads(const ParticleCamera &camera, float now, Vertex *out) const
{
    static const float sizes[kSystems] = {0.6f, 0.5f, 0.35f};
    const glm::mat4 &view = camera.view;
    const glm::vec3 right(view[0][0], view[1][0], view[2][0]), up(view[0][1], view[1][1], view[2][1]);
    const glm::vec3 eye = glm::vec3(glm::inverse(view)[3]);
    const float tanHalf = std::tan(glm::radians(camera.fovDegrees) * 0.5f), height = camera.screenHeight;
    const uint32_t capacity = kMax * 6;
    Ranges ranges{};
    uint32_t written = 0;
    for (int system = 0; system < kSystems; ++system)
    {
        ranges[system].first = written;
        for (const Particle &particle : alive_)
        {
            if (particle.sys != system || written + 6 > capacity)
            {
                continue;
            }
            const float age = std::clamp((now - particle.born) / particle.dur, 0.f, 1.f);
            const glm::vec3 at = particle.implode ? particle.src + particle.vel * (1.f - age) : particle.src;
            const float distance = std::max(0.1f, glm::length(at - eye));
            const float pixels = std::clamp(height * sizes[system] / distance, 4.f, 16.f);
            const float half = pixels * 0.5f * distance * 2.f * tanHalf / height;   // pixels -> world units there
            const float fadeSpan = particle.dur - particle.delay;
            const float fade = fadeSpan > 0.f ? std::clamp((now - particle.born - particle.delay) / fadeSpan, 0.f, 1.f) : 1.f;
            writeBillboard(Billboard{at, right, up, half, blendedColour(particle.from, particle.colour, fade)}, out + written);
            written += 6;
        }
        ranges[system].count = written - ranges[system].first;
    }
    return ranges;
}

}  // namespace argon
