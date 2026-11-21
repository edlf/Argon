// Argon: the game's two listener filters (docs\audio\race-sounds.md "Reverb and filters"). audio\filters.txt holds "Internal" (the
// internal camera) and "Underwater" (the eye under the water), each "<flags> <cutoff Hz> <resonance dB> ..." (FUN_000c97d0);
// flags 0xff00 is the DLS2 resonant low-pass (DSFILTER mode 1: the cutoff 0xc8910, the resonance 0xc8990, at most 22.5
// dB) that sound_play_ui / 0xc8500 put on every positional sound and the engines. ResonantLowPass is a biquad of it.
#pragma once
#include <array>
#include <string>

namespace argon
{

struct LowPassSetting
{
    bool on = false;
    float cutoffHz = 20000.f, resonanceDb = 0.f;
};

struct ListenerFilters
{
    LowPassSetting internal, underwater;
};
ListenerFilters parseListenerFilters(const std::string &text);

class ResonantLowPass
{
  public:
    void configure(const LowPassSetting &setting, int sampleRate);   // off: passes everything
    // interleaved, in place; at most kMaxChannels
    void process(float *samples, size_t frames, size_t channels);
    static constexpr size_t kMaxChannels = 6;

  private:
    float b0_ = 1.f, b1_ = 0.f, b2_ = 0.f, a1_ = 0.f, a2_ = 0.f;
    std::array<std::array<float, 4>, kMaxChannels> state_{};   // per channel: x1, x2, y1, y2
    bool on_ = false;
};

}  // namespace argon
