// Argon: what every draw of the scene pushes. A mesh draw's other data is its record (render\mesh_draw_records.hpp).
#pragma once
#include <glm/glm.hpp>

struct PushConstants
{
    glm::mat4 mvp;
    glm::mat4 model;
};
static_assert(sizeof(PushConstants) <= 128, "Vulkan guarantees only 128 bytes of push constants");
