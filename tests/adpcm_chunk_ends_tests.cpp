// Argon: tests for the Xbox ADPCM encoder's lost chunk-end samples (audio/adpcm_chunk_ends.hpp). No window, GPU or
// sound card: run with test.bat / test.sh (or ctest in build).
#include "audio/adpcm_chunk_ends.hpp"
#include "check.hpp"

#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

const size_t kChunkFrames = argon::kAdpcmBlockFrames * argon::kAdpcmEncoderChunkBlocks;

// `frames` stereo frames, the left channel rising by 10 a frame and the right falling by 10
std::vector<int16_t> stereoRamp(size_t frames)
{
    std::vector<int16_t> samples;
    for (size_t frame = 0; frame < frames; ++frame)
    {
        samples.push_back(int16_t(frame % 3000 * 10));
        samples.push_back(int16_t(-int(frame % 3000) * 10));
    }
    return samples;
}

void testChunkEndPulledToSilenceIsRebuilt()
{
    std::vector<int16_t> samples = stereoRamp(2 * kChunkFrames);
    const std::vector<int16_t> expected = samples;
    samples[(kChunkFrames - 1) * 2] = 0;
    samples[(kChunkFrames - 1) * 2 + 1] = 0;
    argon::repairAdpcmChunkEnds(samples, 2);
    CHECK(samples == expected);
}

void testOtherSamplesAreKept()
{
    std::vector<int16_t> samples = stereoRamp(2 * kChunkFrames);
    samples[(kChunkFrames - 2) * 2] = 0;
    const std::vector<int16_t> expected = samples;
    argon::repairAdpcmChunkEnds(samples, 2);
    CHECK(samples[(kChunkFrames - 2) * 2] == expected[(kChunkFrames - 2) * 2]);
}

void testSecondChunkEndIsRebuiltToo()
{
    std::vector<int16_t> samples = stereoRamp(3 * kChunkFrames);
    const std::vector<int16_t> expected = samples;
    samples[(2 * kChunkFrames - 1) * 2] = 0;
    argon::repairAdpcmChunkEnds(samples, 2);
    CHECK(samples == expected);
}

// the song's last sample has no next block to rebuild it from
void testLastFrameOfTheSongIsKept()
{
    std::vector<int16_t> samples(kChunkFrames, 500);
    samples.back() = 0;
    argon::repairAdpcmChunkEnds(samples, 1);
    CHECK(samples.back() == 0);
}
}  // namespace

int main()
{
    testChunkEndPulledToSilenceIsRebuilt();
    testOtherSamplesAreKept();
    testSecondChunkEndIsRebuiltToo();
    testLastFrameOfTheSongIsKept();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
