// Argon: the crafts' engine loops, see engine_voices.hpp.
#include "audio/engine_voices.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "audio/audio_output.hpp"
#include "audio/loop_voice.hpp"
#include "audio/mix_paths.hpp"
#include "audio/spatial.hpp"
#include "logic/sound_rules.hpp"

namespace argon
{
namespace
{
constexpr float kOtherMinDist = 50.f, kOtherMaxDist = 200.f;   // 0xc3220
constexpr float kMaxCurveInput = 500.f, kSilentMb = -10000.f, kMaxPitch = 8142.f, kPitchPerOctave = 4096.f;

// a layer's curve: straight lines between its points, the end points' values outside them
float curveAt(const std::vector<glm::vec2> &points, float x)
{
    if (points.empty())
    {
        return 0.f;
    }
    if (x <= points.front().x)
    {
        return points.front().y;
    }
    for (size_t i = 1; i < points.size(); ++i)
    {
        if (x <= points[i].x)
        {
            const float along = (x - points[i - 1].x) / std::max(1e-4f, points[i].x - points[i - 1].x);
            return points[i - 1].y + (points[i].y - points[i - 1].y) * along;
        }
    }
    return points.back().y;
}
}  // namespace

void EngineVoices::playOwn(EngineSounds &sounds)
{
    start(own_, sounds);
}

void EngineVoices::playOthers(std::vector<EngineSounds> &engines)
{
    for (Layers &craft : others_)
    {
        stopVoices(craft);
    }
    others_.clear();
    others_.resize(engines.size());
    for (size_t craft = 0; craft < engines.size(); ++craft)
    {
        start(others_[craft], engines[craft]);
        std::printf("audio: player %zu's engine, %zu layers\n", craft + 2, others_[craft].size());
    }
}

void EngineVoices::stop()
{
    stopVoices(own_);
    for (Layers &craft : others_)
    {
        stopVoices(craft);
    }
    others_.clear();   // the drones' engines: Audio::update no longer runs once the front end is open
}

void EngineVoices::hold()
{
    holdVoices(own_);
    for (Layers &craft : others_)
    {
        holdVoices(craft);
    }
}

void EngineVoices::resume()
{
    resumeVoices(own_);
    for (Layers &craft : others_)
    {
        resumeVoices(craft);
    }
}

void EngineVoices::driveOwn(float curveInput, float gain)
{
    const hrtf::Direction ahead{0.f, 0.f, hrtf::kAheadDistance};   // the craft, ahead of the camera
    drive(own_, curveInput, Mix{gain, 1.f, output_.routingFor(ahead)});
}

void EngineVoices::driveOthers(const std::vector<Audio::CraftEngine> &crafts, const spatial::Listener &listener, const OthersLevel &level)
{
    for (size_t craft = 0; craft < others_.size(); ++craft)
    {
        const Audio::CraftEngine engine = craft < crafts.size() ? crafts[craft] : Audio::CraftEngine{};
        drive(others_[craft], engine.throttle, otherMix(engine, listener, level));
    }
}

// a craft's layers started on their loops (the ones they replace stopped)
void EngineVoices::start(Layers &layers, EngineSounds &sounds)
{
    stopVoices(layers);
    layers.clear();
    for (EngineSounds::Layer &layer : sounds.layers)
    {
        layers.push_back({});
        Layer &started = layers.back();
        started.vol = std::move(layer.vol);
        started.pitch = std::move(layer.pitch);
        if (layer.loop)
        {
            started.fileRate = float(layer.loop->rate);
            started.voice.setBus(Bus::World);
            startLoop(output_, started.voice, layer.loop);
        }
    }
}

// another craft's engine buffer (0xc3220): MINDIST 50, MAXDIST 200, the rolloff curve, doppler x 3; on the listener
// in split screen
EngineVoices::Mix EngineVoices::otherMix(const Audio::CraftEngine &craft, const spatial::Listener &listener, const OthersLevel &level) const
{
    if (level.onListener)
    {
        return {level.gain, 1.f, output_.routingFor({})};
    }
    const float faded = level.gain * sound::rolloffGain(glm::length(craft.position - listener.position), kOtherMinDist, kOtherMaxDist);
    const hrtf::Direction direction = hrtf::directionOf(spatial::headOf(listener), craft.position);
    return {faded, spatial::dopplerRatio(listener, craft.position, craft.velocity), output_.routingFor(direction)};
}

// engine layers (0xc5200): curve input clamped 0..500, volume -10000..0 mB, pitch +-8142 (1/4096 octave). The pitch
// goes to IDirectSoundBuffer_SetPitch (0x13abd7), which is absolute: 0 plays at 48 kHz whatever the file's rate (the
// MCPX's base rate), so a 22050 Hz layer plays 48000 / 22050 faster than its own rate at pitch 0
void EngineVoices::drive(Layers &layers, float curveInput, const Mix &mix)
{
    const float input = std::clamp(curveInput, 0.f, kMaxCurveInput);
    for (Layer &layer : layers)
    {
        if (!layer.voice.active())
        {
            continue;
        }
        const float mb = std::clamp(curveAt(layer.vol, input), kSilentMb, 0.f);
        const float pitch = std::clamp(curveAt(layer.pitch, input), -kMaxPitch, kMaxPitch);
        const float playbackRatio = std::pow(2.f, pitch / kPitchPerOctave) * kMcpxBaseRate / layer.fileRate;
        layer.voice.setPitch(std::clamp(playbackRatio, 1.f / 16.f, 16.f) * mix.pitch);
        layer.voice.setVolume(mix::gainOf(mb + mix::kEngineMb));
        layer.voice.setGains(mix.gain, mix.gain);
        layer.voice.setRouting(mix.routing);
    }
}

}  // namespace argon
