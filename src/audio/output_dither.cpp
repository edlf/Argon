#include "audio/output_dither.hpp"

namespace argon
{

void OutputDither::process(float *interleaved, size_t samples)
{
    for (size_t index = 0; index < samples; ++index)
    {
        interleaved[index] += (nextUniform() - nextUniform()) * kStep;
    }
}

float OutputDither::nextUniform()
{
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return float(state_ >> 8) * (1.f / 16777216.f);
}

}  // namespace argon
