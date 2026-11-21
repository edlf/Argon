// Argon: where a sound sits for the listener, as the game's DirectSound 3D buffers place it (docs\audio\README.md "Race
// sounds"): the listener's head for the HRTF (hrtf.hpp) and the doppler of a moving source (the crafts' engine buffers
// use a doppler factor of 3, 0xc3220). Distance fades are sound::rolloffGain.
#pragma once
#include <glm/glm.hpp>

#include "audio/hrtf.hpp"

namespace argon::spatial
{

struct Listener
{
    glm::vec3 position{0.f};
    glm::vec3 right{1.f, 0.f, 0.f};
    glm::vec3 velocity{0.f};
    glm::vec3 front{0.f, 0.f, 1.f}, top{0.f, 1.f, 0.f};
};

inline hrtf::Head headOf(const Listener &listener)
{
    return {listener.position, listener.front, listener.top, listener.right};
}

// the pitch ratio a source moving at `velocity` from `at` gets (speed of sound 343 m/s, doppler factor 3),
// kept within an octave either way
float dopplerRatio(const Listener &listener, const glm::vec3 &at, const glm::vec3 &velocity);

}  // namespace argon::spatial
