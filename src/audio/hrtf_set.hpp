// Argon: which HRTF the 3D voices play through (Argon Options' HRTF, settings.json "hrtf"; docs\argon\differences.md):
// the Xbox's own (hrtf.hpp) or the MIT KEMAR measurements (kemar.hpp), cut to Argon Options' KEMAR Taps
// (settings.json "kemarTaps").
#pragma once

namespace argon::hrtf
{

enum class Set
{
    Xbox,
    Kemar
};

constexpr int kKemarTapChoices[] = {64, 128, 256, 512};   // the longest is hrtf_kemar.bin's own length
constexpr int kDefaultKemarTaps = kKemarTapChoices[0];

}  // namespace argon::hrtf
