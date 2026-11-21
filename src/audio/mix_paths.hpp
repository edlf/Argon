// Argon: the level each of the game's voice paths loses to DirectSound's headroom (docs\audio\mix-levels.md "Mix levels"). The
// game's DirectSound gives a 2D voice 600 mB of headroom and a 3D one none; every speaker
// mixbin then halves its sum (headroom 1 on every mixbin but the submix one), the four 3D (crosstalk) bins the
// HRTF filters feed too; the HRTF stage itself adds none (HRTF headroom 0).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include <glm/glm.hpp>

namespace argon::mix
{

// a mono sound on one of the sound manager's 26 3D voices (0xc47de: DSBCAPS_CTRL3D), positional or not
constexpr float kVoice3dMb = 0.f;
// what a 3D voice's HRTF output loses to the 3D mixbins' headroom 1 (applied to its ears, not to the speaker voices'
// share added on them)
constexpr float kHrtfBinsMb = -600.f;
// a sound of per-speaker files (LEFT / RIGHT / LS / RS, the 5.1 ambience) on the 2D speaker voices (0xc484e..): the
// voice's 600 mB and the speaker mixbins' 6 dB
constexpr float kSpeakerFilesMb = -1200.f;
// the story scenes' 5.1 ambience (cut_ambient) on those voices: 0xca540 sets the SFX option less 500 mB (clamped to
// -9600..0)
constexpr float kSceneAmbienceMb = -500.f;
// an engine layer: a 2D voice (600 mB) into its craft's 3D submix buffer, which 0xc473c gives SetHeadroom(100)
constexpr float kEngineMb = -700.f;
// the music stream: SetHeadroom(100) (0xdf854) and the speaker mixbins' 6 dB
constexpr float kMusicMb = -700.f;

inline float gainOf(float mb)
{
    return std::pow(10.f, mb / 2000.f);
}

inline bool endsWith(const std::string &text, const std::string &suffix)
{
    return text.size() > suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// the dashboard's speaker setting: Stereo has the DSP downmix the six speaker mixbins, Dolby Digital encodes all six
enum class SpeakerLayout
{
    Stereo,
    Surround
};
// the Xbox's six speaker mixbins, also the 5.1 output's channel order
enum Speaker : size_t
{
    kFrontLeft,
    kFrontRight,
    kCentre,
    kLfe,
    kBackLeft,
    kBackRight,
    kSpeakerCount
};
using SpeakerGains = std::array<float, kSpeakerCount>;

inline int channelCount(SpeakerLayout layout)
{
    return layout == SpeakerLayout::Surround ? int(kSpeakerCount) : 2;
}

// In Stereo mode DirectSound has the Dolby DSP downmix the six mixbins (the digital output, which an XboxHD+ takes).
// Its exact program is unknown: the standard Lo / Ro, centre and rears at -3 dB, the LFE dropped.
constexpr float kDownmix = 0.7071f;
// the -5 dB centre copy the front speaker voices add (0xc498c / 0xc49dd)
constexpr float kCentreCopyGain = 0.5623f;

inline glm::vec2 stereoDownmix(const SpeakerGains &gains)
{
    const float centre = gains[kCentre] * kDownmix;
    return {gains[kFrontLeft] + centre + gains[kBackLeft] * kDownmix, gains[kFrontRight] + centre + gains[kBackRight] * kDownmix};
}

// six speaker mixbins as the output's channels, interleaved: downmixed for stereo, as they are for 5.1
inline void foldSpeakers(const float *mixbins, float *out, size_t frames, SpeakerLayout layout)
{
    for (size_t frame = 0; frame < frames; ++frame)
    {
        SpeakerGains bins;
        std::copy_n(mixbins + frame * kSpeakerCount, kSpeakerCount, bins.begin());
        if (layout == SpeakerLayout::Surround)
        {
            std::copy(bins.begin(), bins.end(), out + frame * kSpeakerCount);
            continue;
        }
        const glm::vec2 stereo = stereoDownmix(bins);
        out[frame * 2] = stereo.x;
        out[frame * 2 + 1] = stereo.y;
    }
}

// the 2D speaker voices a sound's file also plays on (the record's +0x91..+0x96, set by CENTRE / LFE / LS / RS / RIGHT /
// LEFT, ROUTETO's targets, and NOPOSITION: centre, right, left). Each 3D voice record's six (0xc484e..0xc49f0): FC,
// LFE, BACK_RIGHT (the LS flag's), BACK_LEFT (RS's), FR + its centre copy, FL + its centre copy
struct SpeakerFlags
{
    bool centre = false, lfe = false, ls = false, rs = false, right = false, left = false;
};
constexpr SpeakerFlags kNoPositionSpeakers{true, false, false, false, true, true};

// what the flagged speaker voices add to each speaker mixbin, at their -12 dB (sound_play_ui 0xc4108 plays the sound's
// data on each, at its volume)
inline SpeakerGains speakerShare(const SpeakerFlags &speakers)
{
    SpeakerGains gains{};
    const float voice = gainOf(kSpeakerFilesMb);
    gains[kCentre] = ((speakers.centre ? 1.f : 0.f) + (speakers.left ? kCentreCopyGain : 0.f) + (speakers.right ? kCentreCopyGain : 0.f)) * voice;
    gains[kLfe] = speakers.lfe ? voice : 0.f;
    gains[kFrontLeft] = speakers.left ? voice : 0.f;
    gains[kFrontRight] = speakers.right ? voice : 0.f;
    gains[kBackRight] = speakers.ls ? voice : 0.f;
    gains[kBackLeft] = speakers.rs ? voice : 0.f;
    return gains;
}

// a per-speaker file's speaker mixbins, by its lower-case name's suffix: _l / _r on their front speaker and its centre
// copy, _ls / _rs on the rear ones (0xc4908 / 0xc494a), _c on the centre, _lfe on the LFE (the 5.1 ambience). Any
// other file plays on both fronts in full.
inline SpeakerGains speakerFileGains(const std::string &lowerStem)
{
    SpeakerGains gains{};
    if (endsWith(lowerStem, "_ls") || endsWith(lowerStem, ".ls"))
    {
        gains[kBackLeft] = 1.f;
    }
    else if (endsWith(lowerStem, "_rs") || endsWith(lowerStem, ".rs"))
    {
        gains[kBackRight] = 1.f;
    }
    else if (endsWith(lowerStem, "_lfe") || endsWith(lowerStem, ".lfe"))
    {
        gains[kLfe] = 1.f;
    }
    else if (endsWith(lowerStem, "_l") || endsWith(lowerStem, ".l"))
    {
        gains[kFrontLeft] = 1.f;
        gains[kCentre] = kCentreCopyGain;
    }
    else if (endsWith(lowerStem, "_r") || endsWith(lowerStem, ".r"))
    {
        gains[kFrontRight] = 1.f;
        gains[kCentre] = kCentreCopyGain;
    }
    else if (endsWith(lowerStem, "_c") || endsWith(lowerStem, ".c"))
    {
        gains[kCentre] = 1.f;
    }
    else
    {
        gains[kFrontLeft] = gains[kFrontRight] = 1.f;
    }
    return gains;
}

}  // namespace argon::mix
