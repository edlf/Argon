// Argon: the point particles (0xbc640 update / 0xbc900 draw): the pickups' starbursts (0x88750), the weapons' trails
// and sparks, Atlantis Mode's bubbles. Three systems (0x88660): 0 starburst (size 0.6, gravity -10), 1 starburstred
// (0.5, -13), 2 bubble (0.35, 0), 2500 particles in all.
#pragma once
#include "logic/viewer_rand.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace argon
{

struct Vertex;

// neon_add_particle 0xbcb60's entry. implode: pos = src + vel (1 - t) (the pickups' bursts); else src moves with vel
// under its system's gravity. Colour: `from` until `delay`, then a blend to `colour` by the end. ship: the followed
// grid slot, -1 = none.
struct Particle
{
    glm::vec3 vel, src;
    int ship;
    float born, dur;
    uint32_t colour;
    int sys = 0;
    bool implode = true;
    uint32_t from = 0xff000000u;
    float delay = 0.f;
};

// a pickup's starburst (0x88750): from src, imploding onto the craft in followSlot (-1: onto src), at `speed`, in
// `colour` (0: the mega-up's random red / yellow / blue per direction)
struct ParticleBurst
{
    glm::vec3 src;
    int followSlot;
    float speed;
    uint32_t colour;
};

// the camera the quads face: the view, the vertical field of view, the screen's height in pixels
struct ParticleCamera
{
    glm::mat4 view;
    float fovDegrees, screenHeight;
};

// one system's vertices in the written buffer
struct ParticleRange
{
    uint32_t first = 0, count = 0;
};

class Particles
{
  public:
    static constexpr int kMax = 2500;   // starburst 1000 + starburstred 500 + bubble 1000
    static constexpr int kSystems = 3;
    using Ranges = std::array<ParticleRange, kSystems>;

    // dropped once all kMax are alive
    void add(const Particle &particle);
    bool hasRoom() const;
    void burst(const ParticleBurst &burst, float now, ViewerRand &rand);
    // Atlantis Mode (scene_viewport 0x94150): 30 bubbles a frame around the camera
    void emitBubbles(const glm::vec3 &camera, float now, ViewerRand &rand);
    // a 60 Hz step: followers move with their craft (slotPositions by grid slot), the rest fly; the old ones go
    void step(std::span<const glm::vec3> slotPositions, float now);
    void clear();
    size_t size() const;
    bool empty() const;
    // the camera-facing quads (two triangles each), system by system, into out (room for kMax x 6 vertices)
    Ranges writeQuads(const ParticleCamera &camera, float now, Vertex *out) const;

  private:
    std::vector<Particle> alive_;
};

}  // namespace argon
