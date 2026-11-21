// Argon: one 3D voice's HRTF and speaker voices into the output's channels, see voice_router.hpp.
#include "audio/voice_router.hpp"

#include <algorithm>
#include <array>

namespace argon
{
namespace
{
constexpr float kGlide = 0.01f;   // per sample, as the HRTF's
constexpr size_t kBlockFrames = 256;
constexpr size_t kStereo = 2;
}  // namespace

VoiceRouter::VoiceRouter(mix::SpeakerLayout layout) : layout_(layout)
{
}

void VoiceRouter::setTarget(const VoiceRouting &routing)
{
    ears_.setTarget(routing.ears);
    targetFrontRear_ = routing.frontRear;
    targetSpeakers_ = routing.speakers;
}

void VoiceRouter::jumpTo(const VoiceRouting &routing)
{
    ears_.jumpTo(routing.ears);
    frontRear_ = targetFrontRear_ = routing.frontRear;
    speakers_ = targetSpeakers_ = routing.speakers;
}

void VoiceRouter::glideGains()
{
    frontRear_.front += kGlide * (targetFrontRear_.front - frontRear_.front);
    frontRear_.rear += kGlide * (targetFrontRear_.rear - frontRear_.rear);
    for (size_t speaker = 0; speaker < mix::kSpeakerCount; ++speaker)
    {
        speakers_[speaker] += kGlide * (targetSpeakers_[speaker] - speakers_[speaker]);
    }
}

// one frame: the ears' pair on the 3D bins, the source on the speaker voices' bins
void VoiceRouter::route(float source, const float *ears, float *out) const
{
    if (layout_ == mix::SpeakerLayout::Stereo)
    {
        const glm::vec2 share = mix::stereoDownmix(speakers_) * source;
        out[0] = ears[0] * frontRear_.front + share.x;
        out[1] = ears[1] * frontRear_.front + share.y;
        return;
    }
    for (size_t speaker = 0; speaker < mix::kSpeakerCount; ++speaker)
    {
        out[speaker] = speakers_[speaker] * source;
    }
    out[mix::kFrontLeft] += ears[0] * frontRear_.front;
    out[mix::kFrontRight] += ears[1] * frontRear_.front;
    out[mix::kBackLeft] += ears[0] * frontRear_.rear;
    out[mix::kBackRight] += ears[1] * frontRear_.rear;
}

void VoiceRouter::process(const float *in, float *out, size_t frames)
{
    const size_t channels = size_t(mix::channelCount(layout_));
    std::array<float, kBlockFrames * kStereo> ears{};
    for (size_t done = 0; done < frames;)
    {
        const size_t block = std::min(kBlockFrames, frames - done);
        ears_.process(in + done * kStereo, ears.data(), block);
        for (size_t frame = 0; frame < block; ++frame)
        {
            glideGains();
            route(in[(done + frame) * kStereo], ears.data() + frame * kStereo, out + (done + frame) * channels);
        }
        done += block;
    }
}

}  // namespace argon
