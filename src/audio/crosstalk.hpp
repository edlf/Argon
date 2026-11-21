// Argon: the DSP's crosstalk canceller (audio\dsstdfx.bin's crosstalk program; docs\audio\mix-levels.md "Mix levels"). With
// speakers (no headphones) it runs on the 3D mixbins on their way to the front speakers: each side gets the other
// side's output 11 samples (at 48 kHz) late through a one-pole filter y = 0.924 y + -0.0585 x, added to its input.
#pragma once
#include <vector>

namespace argon
{

class Crosstalk
{
  public:
    void configure(int sampleRate);
    // one speaker pair in place: `pair` points at the left sample of the first frame, the right one follows, frames
    // `stride` samples apart
    void process(float *pair, size_t frames, size_t stride);

  private:
    std::vector<float> lateLeft_, lateRight_;   // the outputs, one delay long
    size_t at_ = 0;
    float filteredLeft_ = 0.f, filteredRight_ = 0.f;
};

}  // namespace argon
