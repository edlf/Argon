// Argon: tests for the 16-bit TPDF dither on the final mix (audio/output_dither.hpp). No window, GPU or sound card:
// run with test.bat / test.sh (or ctest in build).
#include "audio/output_dither.hpp"
#include "check.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

const size_t kSamples = 200000;

std::vector<float> ditheredSilence()
{
    std::vector<float> samples(kSamples, 0.f);
    argon::OutputDither dither;
    dither.process(samples.data(), samples.size());
    return samples;
}

void testNoiseStaysWithinOneStep()
{
    const std::vector<float> noise = ditheredSilence();
    float peak = 0.f;
    for (float sample : noise)
    {
        peak = std::max(peak, std::abs(sample));
    }
    CHECK(peak < argon::OutputDither::kStep);
}

void testNoiseHasNoOffset()
{
    const std::vector<float> noise = ditheredSilence();
    double sum = 0.0;
    for (float sample : noise)
    {
        sum += sample;
    }
    CHECK(std::abs(sum / double(noise.size())) < 0.01 * argon::OutputDither::kStep);
}

// triangular noise of +-1 step has a variance of 1/6 step squared (a rectangular one: 1/12)
void testNoiseIsTriangular()
{
    const std::vector<float> noise = ditheredSilence();
    double squares = 0.0;
    for (float sample : noise)
    {
        const double steps = sample / argon::OutputDither::kStep;
        squares += steps * steps;
    }
    CHECK(std::abs(squares / double(noise.size()) - 1.0 / 6.0) < 0.005);
}

void testNoiseIsNotSilent()
{
    const std::vector<float> noise = ditheredSilence();
    CHECK(std::count(noise.begin(), noise.end(), 0.f) < 10);
}
}  // namespace

int main()
{
    testNoiseStaysWithinOneStep();
    testNoiseHasNoOffset();
    testNoiseIsTriangular();
    testNoiseIsNotSilent();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
