// argon_decode <sound> <out.wav>: one sound (Xbox ADPCM or PCM WAV, FLAC, Ogg Vorbis, MP3) decoded as argon plays it,
// written as PCM16 WAV, with one line on stdout: its format, peak, play length (`fact`) and loop (`smpl`).
// tools\assets\convert_assets.py converts the game's ADPCM music and sound effects with it.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "audio/sound_load.hpp"

namespace
{
int peakOf(const argon::Pcm &pcm)
{
    int peak = 0;
    for (const int16_t sample : pcm.samples)
    {
        peak = std::max(peak, std::abs(int(sample)));
    }
    return peak;
}

void printReport(const std::string &in, const argon::Pcm &pcm)
{
    std::printf("%s: %d ch, %d Hz, %.2f s, peak %d, play %zu, loop [%zu, %zu)\n", in.c_str(), pcm.channels, pcm.rate, pcm.seconds(), peakOf(pcm),
                pcm.playFrames, pcm.sampleLoopBegin, pcm.sampleLoopEnd);
    std::fflush(stdout);
}
}  // namespace

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::fprintf(stderr, "usage: argon_decode <sound> <out.wav>\n");
        return 2;
    }
    const std::string in = argv[1], out = argv[2];
    try
    {
        const argon::Pcm pcm = argon::loadSound(std::filesystem::path(std::u8string(in.begin(), in.end())));
        printReport(in, pcm);
        if (!argon::writeWav(out, pcm))
        {
            std::fprintf(stderr, "cannot write %s\n", out.c_str());
            return 1;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    return 0;
}
