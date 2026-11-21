// Argon: starting a looping voice: the buffer gets its click-free seam first (makeLoopable), nothing without a device
// or a sound.
#pragma once
#include <memory>

#include "audio/voice.hpp"

namespace argon
{

class AudioOutput;
struct Pcm;

bool startLoop(AudioOutput &output, Voice &voice, std::shared_ptr<Pcm> loop);

// the pause menu's hold, for every voice of a list
template <typename Holders>
void holdVoices(Holders &holders)
{
    for (auto &holder : holders)
    {
        holder.voice.pause();
    }
}

template <typename Holders>
void resumeVoices(Holders &holders)
{
    for (auto &holder : holders)
    {
        holder.voice.resume();
    }
}

template <typename Holders>
void stopVoices(Holders &holders)
{
    for (auto &holder : holders)
    {
        holder.voice.stop();
    }
}

// one-shots that played to their end leave the list
template <typename Holders>
void dropFinishedVoices(Holders &holders)
{
    for (auto it = holders.begin(); it != holders.end();)
    {
        if (it->voice.finished())
        {
            it->voice.stop();
            it = holders.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

}  // namespace argon
