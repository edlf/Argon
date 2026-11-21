// The water wake's walk and sprite heights (craft_wake_emitter 0x50960, docs/effects/water-wake.md "Water wake").
// Each frame both rear box corners (2 and 3) are under water (craft_water 0x61340 -> effects object +0x90 > 1), the
// emitter walks from last frame's position (+0xd0) to this one and spawns spray at every step.
#pragma once

#include <optional>
#include <vector>
#include <glm/glm.hpp>

namespace argon
{

// effects object +0x90 > 1: both rear corners under water
constexpr int kWakeCornersNeeded = 2;

// 0x50d5c..0x5144a: the step distances along the frame's displacement: none when its squared length is at most 0.1
// (0x1a3508), else 0, 2, 4, ... below its length (0x1a3534): the first step is at last frame's position
std::vector<float> wakeStepDistances(const glm::vec3 &displacement);

// 0x50ece / 0x5101e: a sprite's height = base + the water query's out value - 1.5 (0x1a3688). 0xce6b0 writes the
// depth (query y - surface) only when a box holds the point; otherwise the out value keeps the query's own y.
float wakeSpriteHeight(float baseY, float queryY, std::optional<float> depthAtQuery);

} // namespace argon
