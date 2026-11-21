#include "logic/water_wake.hpp"

#include <cmath>

namespace argon
{

namespace
{
constexpr float kMinSquaredDisplacement = 0.1f;
constexpr float kStepMetres = 2.f;
constexpr float kBelowSurface = 1.5f;
} // namespace

std::vector<float> wakeStepDistances(const glm::vec3 &displacement)
{
    std::vector<float> steps;
    const float squared = glm::dot(displacement, displacement);
    if (squared <= kMinSquaredDisplacement)
    {
        return steps;
    }
    const float length = std::sqrt(squared);
    for (float s = 0.f; s < length; s += kStepMetres)
    {
        steps.push_back(s);
    }
    return steps;
}

float wakeSpriteHeight(float baseY, float queryY, std::optional<float> depthAtQuery)
{
    return baseY + depthAtQuery.value_or(queryY) - kBelowSurface;
}

} // namespace argon
