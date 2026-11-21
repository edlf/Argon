// Argon: meshes placed by the camera (docs\models\README.md surface flags 0x400 / 0x800 / 0x100000, docs\effects\README.md
// "Camera-facing meshes"). The render list (0xb7920, models drawn bone by bone) moves a 0x400 mesh by the eye and turns
// a 0x800 mesh to it; cmm_draw_mesh (0xb6e60 at 0xb7765) slides a 0x100000 mesh towards the view axis by its
// billboard factor (the sun's lens pieces) and only then turns it.
#pragma once
#include <glm/glm.hpp>

namespace argon
{
// the camera as the game keeps it beside the view matrix (0xa7900): its position (0x50e308) and its view axis, the
// view's third column (0x50e1f0 [2], [6], [10]); the axis's sign does not matter here
struct ViewPoint
{
    glm::vec3 eye{0.f};
    glm::vec3 axis{0.f, 0.f, 1.f};

    static ViewPoint of(const glm::mat4 &view);
};

// a mesh's surface flags that place it by the camera
struct CameraPlacement
{
    bool followsCamera = false;   // 0x400: its world position is an offset from the eye
    bool facesCamera = false;     // 0x800: its axes replaced by a frame turned to the eye (0xb4e70)
    bool billboard = false;       // 0x100000: moved towards the view axis by billboardFactor
    float billboardFactor = 1.f;  // surface lighting +0x14 (0xbb270): 1 in place, 0 on the axis, < 0 across it

    bool any() const
    {
        return followsCamera || facesCamera || billboard;
    }
    // 0xb7e72: the render list's step, the matrix the draw (and the flare occlusion test) starts from
    glm::mat4 listed(glm::mat4 world, const ViewPoint &view) const;
    // 0xb7765: cmm_draw_mesh's step for the billboards, after the occlusion test
    glm::mat4 drawn(glm::mat4 world, const ViewPoint &view) const;
};

// 0xb77b6: the offset from the eye split along the view axis and across it; the across part scaled by the factor
glm::vec3 billboardPosition(const glm::vec3 &position, const ViewPoint &view, float factor);

// 0xb4e70: D3DXMatrixLookAtRH from the mesh to the eye with up (0, 1, 0), transposed into the world's axes: z away
// from the eye, x = up x z, y = z x x, unit length (the mesh's scale is dropped). Left as it is when the eye is
// straight above or below the mesh (the game's normalize of a zero vector is not followed)
glm::mat4 facingEye(const glm::mat4 &world, const glm::vec3 &eye);
} // namespace argon
