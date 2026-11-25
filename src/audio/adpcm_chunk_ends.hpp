// The game's Xbox ADPCM encoder worked in chunks of 128 blocks (8192 samples per channel) and coded each chunk's last
// sample against silence: that sample's nibble pulls it toward 0, a one-sample click every 0.37 s at 22050 Hz (heard
// in frontend_music and levelload_music, docs\audio\README.md). The next block's header holds the next sample as
// PCM, so the lost sample is rebuilt halfway between its neighbours.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace argon
{

constexpr size_t kAdpcmBlockFrames = 64;
constexpr size_t kAdpcmEncoderChunkBlocks = 128;

// `interleaved`: whole decoded blocks of kAdpcmBlockFrames frames from the data's start
void repairAdpcmChunkEnds(std::vector<int16_t> &interleaved, int channels);

}  // namespace argon
