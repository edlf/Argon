// Argon: the story scenes' head turns (movie\head_turn.hpp)
#include "movie/head_turn.hpp"

#include <string>

namespace argon::story
{

namespace
{
const char *const kHead = "skel_head";   // 0x1a2f98
const char *const kNeck = "_neck";       // 0x19bf04
const char *const kLeftEye = "eyeL";     // 0x19befc
const char *const kRightEye = "eyeR";    // 0x19bef4

bool contains(const std::string &name, const char *part)
{
    return name.find(part) != std::string::npos;
}

BoneAim aimOf(const std::string &name, const glm::vec3 &target, float share)
{
    if (contains(name, kHead))
    {
        return {AimKind::TurnsChildren, target, share};
    }
    if (contains(name, kNeck))
    {
        return {AimKind::KeepsChildren, target, share * kNeckShare};
    }
    if (name == kLeftEye || name == kRightEye)
    {
        return {AimKind::TurnsChildren, target, 1.f};
    }
    return {};
}
} // namespace

bool turnsHead(int lookAt, int objectCount)
{
    return lookAt > 0 && lookAt < objectCount;
}

std::vector<BoneAim> headTurnAims(const Cmm &model, const glm::vec3 &target, float share)
{
    std::vector<BoneAim> aims;
    aims.reserve(model.bones.size());
    for (const CmmBone &bone : model.bones)
    {
        aims.push_back(aimOf(bone.name, target, share));
    }
    return aims;
}

int lookedAtBone(const Cmm &model)
{
    int found = -1;
    for (int b = 0; b < int(model.bones.size()); ++b)
    {
        found = contains(model.bones[size_t(b)].name, kHead) ? b : found;
    }
    return found;
}

} // namespace argon::story
