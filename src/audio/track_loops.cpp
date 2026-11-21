// Argon: a track's looping sounds, see track_loops.hpp.
#include "audio/track_loops.hpp"

#include <cmath>

#include "audio/audio_output.hpp"
#include "audio/loop_voice.hpp"
#include "audio/spatial.hpp"
#include "logic/sound_rules.hpp"

namespace argon
{
namespace
{
constexpr float kGlideSeconds = 0.08f;   // ~80 ms: no zipper or steps when the loudest zone switches
}  // namespace

void TrackLoops::play(const TrackSounds &sounds)
{
    stop();
    startEmitters(sounds);
}

void TrackLoops::stop()
{
    stopVoices(emitters_);
    emitters_.clear();
}

void TrackLoops::hold()
{
    holdVoices(emitters_);
}

void TrackLoops::resume()
{
    resumeVoices(emitters_);
}

void TrackLoops::update(const spatial::Listener &listener, const Levels &levels, float dt)
{
    const float glide = 1.f - std::exp(-dt / kGlideSeconds);
    for (EmitterGroup &group : emitters_)
    {
        const Loudest loudest = loudestPoint(group, listener.position);
        group.gain += (loudest.gain * levels.emitters - group.gain) * glide;
        group.voice.setGains(group.gain, group.gain);
        group.voice.setRouting(output_.routingFor(hrtf::directionOf(spatial::headOf(listener), loudest.at)));
        if (group.voice.active())
        {
            group.voice.setVolume(1.f);
        }
    }
}

void TrackLoops::startEmitters(const TrackSounds &sounds)
{
    for (const TrackSounds::EmitterLoop &loop : sounds.emitterLoops)
    {
        emitters_.push_back({});
        EmitterGroup &group = emitters_.back();
        for (size_t point : loop.points)
        {
            group.points.push_back(sounds.emitters[point]);
        }
        if (loop.loop)
        {
            group.voice.setBus(Bus::World);
            startLoop(output_, group.voice, loop.loop);
        }
    }
}

// the group's point loudest at the listener (the rolloff curve from its MINDIST to its MAXDIST); none: the listener
TrackLoops::Loudest TrackLoops::loudestPoint(const EmitterGroup &group, const glm::vec3 &listener)
{
    Loudest loudest{0.f, listener};
    for (const SoundEmitter &point : group.points)
    {
        const float gain = sound::rolloffGain(glm::length(point.pos - listener), point.minDist, point.maxDist);
        if (gain > loudest.gain)
        {
            loudest = Loudest{gain, point.pos};
        }
    }
    return loudest;
}

}  // namespace argon
