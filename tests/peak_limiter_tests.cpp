// Argon: tests for the master limiter on the final mix (audio/peak_limiter.hpp). No window, GPU or sound
// card: run with test.bat / test.sh (or ctest in build).
#include "audio/peak_limiter.hpp"
#include "check.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

const int kRate = 48000;
const float kThreshold = 0.97f;

// a stereo sine `seconds` long at `amplitude`
std::vector<float> stereoSine(float amplitude, float seconds)
{
    std::vector<float> samples;
    for (int i = 0; i < int(seconds * kRate); ++i)
    {
        const float v = amplitude * std::sin(2.f * 3.14159265f * 440.f * float(i) / kRate);
        samples.push_back(v);
        samples.push_back(v);
    }
    return samples;
}

float peakOf(const std::vector<float> &samples, size_t firstFrame = 0)
{
    float peak = 0.f;
    for (size_t i = firstFrame * 2; i < samples.size(); ++i)
    {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak;
}

void testQuietSignalIsUnchanged()
{
    std::vector<float> samples = stereoSine(0.5f, 0.1f);
    const std::vector<float> original = samples;
    argon::PeakLimiter limiter(kThreshold, 0.05f, kRate);
    limiter.process(samples.data(), samples.size() / 2, 2);
    CHECK(samples == original);
}

void testLoudSignalStaysUnderThreshold()
{
    std::vector<float> samples = stereoSine(3.f, 0.1f);
    argon::PeakLimiter limiter(kThreshold, 0.05f, kRate);
    limiter.process(samples.data(), samples.size() / 2, 2);
    CHECK(peakOf(samples) <= kThreshold + 1e-5f);
}

void testGainRecoversAfterLoudBurst()
{
    std::vector<float> samples = stereoSine(3.f, 0.05f);
    const std::vector<float> quiet = stereoSine(0.5f, 1.f);
    samples.insert(samples.end(), quiet.begin(), quiet.end());
    argon::PeakLimiter limiter(kThreshold, 0.05f, kRate);
    limiter.process(samples.data(), samples.size() / 2, 2);
    CHECK(std::abs(peakOf(samples, size_t(0.8f * kRate)) - 0.5f) < 0.01f);
}

void testOneLoudChannelLowersBoth()
{
    std::vector<float> frame = {2.f, 0.5f};
    argon::PeakLimiter limiter(kThreshold, 0.05f, kRate);
    limiter.process(frame.data(), 1, 2);
    CHECK(std::abs(frame[1] - 0.5f * kThreshold / 2.f) < 1e-5f);
}
}  // namespace

int main()
{
    testQuietSignalIsUnchanged();
    testLoudSignalStaysUnderThreshold();
    testGainRecoversAfterLoudBurst();
    testOneLoudChannelLowersBoth();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
