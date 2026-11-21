// Argon: the craft's row-major 3 x 3 matrices (0xcexxx), shared by the craft's source files.
#pragma once
#include <cstring>

#include <glm/glm.hpp>

namespace argon
{

// ------------------------------------------------------------------ helpers (row-major 3x3, 0xcexxx)
inline glm::vec3 col(const float *rotation, int column)
{
    return {rotation[column], rotation[3 + column], rotation[6 + column]};
}
inline glm::mat3 rotationOf(const float *rotation)
{
    return glm::mat3(col(rotation, 0), col(rotation, 1), col(rotation, 2));
}
inline void mul(const float *left, const float *right, float *out)
{   // out = A * B
    float product[9];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            product[i * 3 + j] = left[i * 3] * right[j] + left[i * 3 + 1] * right[3 + j] + left[i * 3 + 2] * right[6 + j];
        }
    }
    std::memcpy(out, product, sizeof product);
}
inline glm::vec3 mulv(const float *matrix, const glm::vec3 &v)
{   // 0xcec60
    return {matrix[0] * v.x + matrix[1] * v.y + matrix[2] * v.z, matrix[3] * v.x + matrix[4] * v.y + matrix[5] * v.z, matrix[6] * v.x + matrix[7] * v.y + matrix[8] * v.z};
}
inline glm::vec3 clampAbs(glm::vec3 v, float limit)
{
    return glm::clamp(v, glm::vec3(-limit), glm::vec3(limit));
}

} // namespace argon
