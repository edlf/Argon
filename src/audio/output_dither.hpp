// TPDF dither on the final float mix at 16-bit resolution. Windows' shared-mode mix is float, and a device set to 16 bit
// gets it rounded to 16 bits without dither: the music, always scaled by its mix level and volume and resampled, then
// carries rounding distortion that follows the signal (a faint crackle in quiet passages at high volume). Triangular
// noise of +-1 step at 16 bits turns that into a steady hiss at about -96 dB, inaudible on 24- and 32-bit devices.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon
{

class OutputDither
{
  public:
    static constexpr float kStep = 1.f / 32768.f;   // one 16-bit step in the float mix

    void process(float *interleaved, size_t samples);

  private:
    float nextUniform();   // [0, 1)

    uint32_t state_ = 0x9e3779b9u;   // xorshift32, never zero
};

}  // namespace argon
