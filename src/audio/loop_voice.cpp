// Argon: starting a looping voice, see loop_voice.hpp.
#include "audio/loop_voice.hpp"

#include "audio/audio.hpp"
#include "audio/audio_output.hpp"

namespace argon
{

bool startLoop(AudioOutput &output, Voice &voice, std::shared_ptr<Pcm> loop)
{
    if (!output.isOpen() || !loop || loop->samples.empty())
    {
        return false;
    }
    makeLoopable(*loop);
    return voice.start(output, std::move(loop), Playback::Looped);
}

}  // namespace argon
