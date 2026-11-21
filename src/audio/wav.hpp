// WAV files for the viewer (docs/audio/README.md): the game's WAVs are Xbox ADPCM (format 0x69, 36-byte blocks of
// 64 samples per channel) or PCM16. Decoded to Pcm; writeWav saves PCM16 (argon --decode).
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace argon
{

struct Pcm
{
    int channels = 1, rate = 22050;
    std::vector<int16_t> samples;   // interleaved; ADPCM keeps the whole last block (loops need it)
    size_t playFrames = 0;          // length to play once (`fact`); 0 = all. Loops use every sample.
    size_t loopBegin = 0;           // set by makeLoopable(): loop region = [loopBegin, end)
    size_t sampleLoopBegin = 0, sampleLoopEnd = 0;   // the file's `smpl` loop [begin, end); both 0: none
    bool loopReady = false;
    bool speakerFiles = false;      // mixed from per-speaker files: the game plays it on 2D voices (audio\mix_paths.hpp)
    float storedScale = 1.f;        // a mix scaled down to fit int16; Voice plays it louder by as much
    float seconds() const
    {
        return samples.empty() ? 0.f : float(samples.size()) / channels / rate;
    }
};

// throws std::runtime_error naming the file and what is wrong
Pcm loadWav(const std::string &path);
bool writeWav(const std::string &path, const Pcm &pcm);

} // namespace argon
