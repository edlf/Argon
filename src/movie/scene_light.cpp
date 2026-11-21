// Argon: a story scene's own light (movie\scene_light.hpp)
#include "movie/scene_light.hpp"

#include <cmath>

namespace argon::story
{

namespace
{
constexpr float kNearZero = 1e-6f;   // 0x1a34f8
constexpr float kLargest = 1000.f;   // 0x1a34d8

bool isUsable(const glm::vec3 &direction)
{
    bool allNearZero = true;
    for (int axis = 0; axis < 3; ++axis)
    {
        if (!std::isfinite(direction[axis]) || direction[axis] > kLargest)
        {
            return false;
        }
        allNearZero = allNearZero && std::abs(direction[axis]) < kNearZero;
    }
    return !allNearZero;
}

float colourChannel(uint32_t colour, int shift)
{
    return float((colour >> shift) & 0xffu) / 255.f;
}
} // namespace

SceneLight sceneLight(const glm::vec3 &storedDirection, uint32_t storedColour)
{
    if (!isUsable(storedDirection))
    {
        return {};
    }
    return {glm::normalize(storedDirection), glm::vec3(colourChannel(storedColour, 16), colourChannel(storedColour, 8), colourChannel(storedColour, 0))};
}

} // namespace argon::story
