// The master limiter on the final mix: music + ambience + emitters + up to 7 engine layers can sum past full scale,
// and without it the output hard-clips (crackle). The gain drops at once to keep every frame under the threshold and
// recovers over the release time.
#pragma once
#include <cstddef>

namespace argon
{

class PeakLimiter
{
  public:
    PeakLimiter() = default;
    PeakLimiter(float threshold, float releaseSeconds, int sampleRate);
    void process(float *interleaved, size_t frames, int channels);

  private:
    float threshold_ = 1.f;
    float releasePerFrame_ = 0.f;   // how much of the envelope is left after one frame
    float envelope_ = 0.f;          // the recent peak, falling back over the release time
};

}  // namespace argon
