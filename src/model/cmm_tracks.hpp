// Animation track blocks (docs\models\meshes.md "Animation"): records of 0x80 bytes, then every track's keys in record order.
// A .cmm keeps one after its effect programs (0xb6070); a .cms animation is one behind its 0x30-byte header (0xb5270).
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "model/cmm.hpp"

namespace argon
{

// a NUL-padded name of at most `capacity` bytes
std::string fixedString(const char *chars, size_t capacity);

struct AnimationBlock
{
    size_t at = 0;   // the first record
    int count = 0;
};

// the tracks (rotation keys turned from Euler degrees into quaternions, 0xf2ec4 .. 0xf33fe), and the offset after
// the last key; nullopt when a record is broken or its keys run past the data
std::optional<size_t> readAnimationTracks(const std::vector<uint8_t> &data, AnimationBlock block, std::vector<CmmTrack> &out);

} // namespace argon
