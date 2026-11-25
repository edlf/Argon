#include "audio/adpcm_chunk_ends.hpp"

namespace argon
{

void repairAdpcmChunkEnds(std::vector<int16_t> &interleaved, int channels)
{
    const size_t width = size_t(channels);
    const size_t chunkFrames = kAdpcmBlockFrames * kAdpcmEncoderChunkBlocks;
    const size_t frames = interleaved.size() / width;
    for (size_t lastFrame = chunkFrames - 1; lastFrame + 1 < frames; lastFrame += chunkFrames)
    {
        for (size_t channel = 0; channel < width; ++channel)
        {
            const int before = interleaved[(lastFrame - 1) * width + channel];
            const int after = interleaved[(lastFrame + 1) * width + channel];
            interleaved[lastFrame * width + channel] = int16_t((before + after) / 2);
        }
    }
}

}  // namespace argon
