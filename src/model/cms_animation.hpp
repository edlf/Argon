// A .cms animation (0xb5270): an animation track block of its own, bound to a model's bones by name. The story
// scenes keep their actors' body animations inside the .nms file and load the speech's face animations from
// data\characters\<pilot>\ (docs\sequences\README.md "Actors").
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "model/cmm.hpp"

namespace argon
{

struct CmsAnimation
{
    float seconds = 0.f;   // header +0
    std::vector<CmmTrack> tracks;
};

// the animation at `at` and the offset after it; nullopt when it is broken
std::optional<size_t> readCms(const std::vector<uint8_t> &data, size_t at, CmsAnimation &out);

// 0xb55e0 / 0xb5710: per bone of the model, the animation's track of the same name (null: none)
std::vector<const CmmTrack *> tracksByBone(const Cmm &model, const CmsAnimation &animation);

} // namespace argon
