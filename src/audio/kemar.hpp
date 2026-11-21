// Argon: the MIT KEMAR HRTF (Gardner & Martin, MIT Media Lab, 1994; free to use with the authors cited), the viewer's
// alternative to the Xbox's Sensaura filters (Argon Options' HRTF; docs\argon\differences.md). tools\assets\
// convert_kemar.py turns the measurements into <project>\hrtf_kemar.bin: 48 kHz, diffuse-field equalized, 512
// minimum-phase taps per ear and the far ear's delay, the front pair cut to 64 taps at the Xbox front filter's level;
// elevations -40..90 in 10 degree rings, azimuths 0..180 (the left side mirrors the right, as the Xbox's). A voice takes
// its nearest measured direction, as DirectSound takes its nearest filter (hrtf.hpp), cut to Argon Options' KEMAR Taps
// (hrtf_set.hpp) with the converter's tail fade, and glides there. A file of fewer taps (an older converter's 64) plays as it is.
#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "audio/hrtf.hpp"

namespace argon::hrtf
{

struct KemarTables
{
    struct Measurement
    {
        int azimuth = 0;   // degrees to the right, 0..180
        Ears ears;
    };
    struct Ring
    {
        int elevation = 0;
        std::vector<Measurement> measurements;   // azimuth ascending
    };
    std::vector<Ring> rings;   // elevation ascending
    bool empty() const
    {
        return rings.empty();
    }
};

// throws std::runtime_error when the file is missing or not convert_kemar.py's
KemarTables loadKemar(const std::filesystem::path &file);
// the nearest measurement's ears (the source to the left: the mirrored ones), cut to `taps` (the last kKemarFadeTaps
// faded out, cos^2) when the file holds more; without tables, a pass-through
Ears kemarEarsFor(const KemarTables &tables, const Direction &direction, int taps);
constexpr int kKemarFadeTaps = 8;   // convert_kemar.py's FADE_TAPS

}  // namespace argon::hrtf
