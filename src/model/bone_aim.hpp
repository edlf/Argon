// A bone turned towards a point while posing (cmm_bone_animate 0xb8630, bone flags 0x2000 / 0x4000, the point at bone
// +0x13c, the share at +0x198; docs\models\meshes.md "Bone flags at run time"). The story scenes' head turns set them
// (movie\head_turn.hpp).
#pragma once
#include <glm/glm.hpp>

namespace argon
{

enum class AimKind
{
    None,
    TurnsChildren,   // 0x2000: the bone's children follow the turned bone
    KeepsChildren    // 0x4000: the children hang off the bone as it was before the turn (+0xc0)
};

struct BoneAim
{
    AimKind kind = AimKind::None;
    glm::vec3 target{0.f};
    float share = 0.f;   // 0: as posed, 1: its z axis straight at the target
    // the model's own world scale when the bones pose in model space: the game turns in world space, so a turned bone
    // keeps only its own scale there
    float modelScale = 1.f;
};

// 0xb8630 after the bone's world W (glm columns = the game's rows: x, y, z axes, translation): z = the unit vector
// from the bone to the target; the y axis turned to stay square to it, both blended from the posed axes by the share,
// renormalised and squared up again; each axis then has the length rowScale gives (the bone's +0x11c..+0x124). The
// translation stays. A target on the bone leaves it as posed.
glm::mat4 aimBone(const glm::mat4 &world, const BoneAim &aim, const glm::vec3 &rowScale);

} // namespace argon
