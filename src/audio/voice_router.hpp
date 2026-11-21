// Argon: where one of the game's 3D voices goes (docs\audio\mix-levels.md "Mix levels", docs\audio\README.md "5.1"). The VP runs the voice through its
// HRTF pair into the four 3D mixbins, the front pair at the front volume and the rear pair at the rear one
// (hrtf::surroundGains; in stereo the rear is silent); the record's 2D speaker voices play the same data on their
// speaker mixbins, unfiltered. VoiceRouter does both for one voice, on the audio thread, into the output's channels:
// stereo gets the front pair plus the speaker share's downmix, 5.1 the front pair on FL / FR, the rear pair on BL / BR
// and the speaker share on its own channels.
#pragma once
#include <cstddef>

#include "audio/hrtf.hpp"
#include "audio/mix_paths.hpp"

namespace argon
{

struct VoiceRouting
{
    hrtf::Ears ears;
    hrtf::FrontRear frontRear;
    mix::SpeakerGains speakers{};   // the 2D speaker voices' share (mix::speakerShare)
};

class VoiceRouter
{
  public:
    VoiceRouter() = default;
    explicit VoiceRouter(mix::SpeakerLayout layout);
    void setTarget(const VoiceRouting &routing);   // glides there, as the VP's per-sample smoothing
    void jumpTo(const VoiceRouting &routing);      // no glide (a voice's first parameters)
    // interleaved stereo in (the voice's sound), the layout's channels out
    void process(const float *in, float *out, size_t frames);

  private:
    void glideGains();
    void route(float source, const float *ears, float *out) const;

    mix::SpeakerLayout layout_ = mix::SpeakerLayout::Stereo;
    hrtf::EarFilter ears_;
    hrtf::FrontRear frontRear_, targetFrontRear_;
    mix::SpeakerGains speakers_{}, targetSpeakers_{};
};

}  // namespace argon
