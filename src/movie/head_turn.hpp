// The story scenes' head turns (0x99360 per model, docs\sequences\scenes.md "Head turns"): an actor's animation segment with flag 2
// turns its head, neck and eyes towards an object (+0x18) by a share (+0x1c); the bones turn as bone_aim.hpp has it.
// Pure rules.
#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "model/cmm.hpp"

namespace argon::story
{

constexpr float kNeckShare = 0.3f;   // 0x1a35d0: the neck turns 0.3 of the head's share

// 0x998b2: the segment's object counts when 0 < object < the object count (object 0 never; -1 none)
bool turnsHead(int lookAt, int objectCount);

// 0x999b5 per bone of the actor's model, by name (0xb5190: contains): "skel_head" 0x2000 by the share, else "_neck"
// 0x4000 by 0.3 x the share (its children keep the unturned neck), else exactly "eyeL" / "eyeR" 0x2000 fully; the
// others none
std::vector<BoneAim> headTurnAims(const Cmm &model, const glm::vec3 &target, float share);

// 0x9996e: the bone another actor looks at, the last whose name contains "skel_head" (-1: none, the model's origin)
int lookedAtBone(const Cmm &model);

} // namespace argon::story
