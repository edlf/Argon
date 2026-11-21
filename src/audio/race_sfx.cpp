// Argon: the race's sounds, see race_sfx.hpp.
#include "audio/race_sfx.hpp"

#include <algorithm>
#include <cmath>

#include "audio/audio_output.hpp"

namespace argon
{
namespace
{
float linearGain(float mb)
{
    return std::pow(10.f, mb / 2000.f);
}

const SoundOptions kNoOptions;
}  // namespace

RaceSfx::RaceSfx(AudioOutput &output) : output_(output)
{
}

void RaceSfx::setBank(GenericSoundBank bank)
{
    stopAll();
    bank_ = std::move(bank);
}

void RaceSfx::setShipSounds(std::vector<ShipSounds> bySlot)
{
    stopAll();
    ships_ = std::move(bySlot);
}

const SoundOptions &RaceSfx::options(GenericSound id) const
{
    const size_t index = size_t(id);
    return index < bank_.size() ? bank_[index].options : kNoOptions;
}

const SoundOptions &RaceSfx::shipOptions(ShipSound id, int craft) const
{
    const SoundClip *clip = clipOf(kShipKey + int(id), craft);
    return clip ? clip->options : kNoOptions;
}

void RaceSfx::setListener(const spatial::Listener &listener)
{
    listener_ = listener;
}

void RaceSfx::setCentred(bool on)
{
    centred_ = on;
}

void RaceSfx::placeCraft(int craft, const glm::vec3 &position)
{
    if (craft < 0)
    {
        return;
    }
    if (size_t(craft) >= craftPositions_.size())
    {
        craftPositions_.resize(size_t(craft) + 1, glm::vec3(0.f));
    }
    craftPositions_[size_t(craft)] = position;
}

int RaceSfx::playOnListener(GenericSound id)
{
    Playing playing;
    playing.key = int(id);
    return start(std::move(playing), Start::Whole);
}

int RaceSfx::playAtCraft(GenericSound id, int craft)
{
    Playing playing;
    playing.key = int(id);
    playing.craft = craft;
    return start(std::move(playing), Start::Whole);
}

int RaceSfx::playAtPoint(GenericSound id, const glm::vec3 &point)
{
    Playing playing;
    playing.key = int(id);
    playing.atPoint = true;
    playing.point = point;
    return start(std::move(playing), Start::Whole);
}

int RaceSfx::playShipSound(ShipSound id, int craft)
{
    Playing playing;
    playing.key = kShipKey + int(id);
    playing.craft = craft;
    return start(std::move(playing), Start::Whole);
}

void RaceSfx::moveTo(int handle, const glm::vec3 &point)
{
    if (Playing *playing = find(handle))
    {
        playing->atPoint = true;
        playing->point = point;
    }
}

void RaceSfx::setDistances(int handle, float minDist, float maxDist)
{
    if (Playing *playing = find(handle))
    {
        playing->options.minDist = minDist;
        playing->options.maxDist = maxDist;
    }
}

void RaceSfx::stopHandle(int handle)
{
    for (auto it = playing_.begin(); it != playing_.end();)
    {
        it = it->handle == handle ? playing_.erase(it) : it + 1;
    }
}

bool RaceSfx::isPlaying(GenericSound id, int craft) const
{
    return isPlayingKey(int(id), craft);
}

bool RaceSfx::startedWithin(GenericSound id, int craft, float seconds) const
{
    for (const Playing &playing : playing_)
    {
        if (holds(playing, int(id), craft) && clock_ - playing.startedAt <= seconds)
        {
            return true;
        }
    }
    return false;
}

void RaceSfx::stop(GenericSound id, int craft)
{
    for (Playing &playing : playing_)
    {
        if (holds(playing, int(id), craft))
        {
            playing.voice.release();
            playing.released = true;
        }
    }
}

// 0x70d92: held with turbo, SF_SHIP_TURBOFIRE starts unless playing and rounds its loop; held without, it stops and
// TURBOFAIL (the same clip from its loop's end, 0xc3540) plays unless playing; let go, both stop
void RaceSfx::setTurbo(int craft, Turbo turbo)
{
    const int fire = kShipKey + int(ShipSound::TurboFire), fail = kShipKey + int(ShipSound::TurboFail);
    if (turbo == Turbo::Released)
    {
        stopKey(fire, craft);
        stopKey(fail, craft);
        return;
    }
    Playing playing;
    playing.craft = craft;
    if (turbo == Turbo::Pushing)
    {
        if (!isPlayingKey(fire, craft))
        {
            playing.key = fire;
            start(std::move(playing), Start::HoldingLoop);
        }
        return;
    }
    if (isPlayingKey(fire, craft))
    {
        stopKey(fire, craft);
        return;
    }
    if (!isPlayingKey(fail, craft))
    {
        playing.key = fail;
        start(std::move(playing), Start::Tail);
    }
}

void RaceSfx::setVolumeMb(int handle, float mb)
{
    if (Playing *playing = find(handle))
    {
        playing->volumeMb = mb;
        applyLevels(*playing);
    }
}

void RaceSfx::setPitch(int handle, float ratio)
{
    if (Playing *playing = find(handle))
    {
        playing->voice.setPitch(ratio);
    }
}

void RaceSfx::update(float dt, float sfxVolume)
{
    clock_ += dt;
    sfxVolume_ = sfxVolume;
    dropFinished();
    for (Playing &playing : playing_)
    {
        applyLevels(playing);
    }
}

void RaceSfx::pause()
{
    for (Playing &playing : playing_)
    {
        playing.voice.pause();
    }
}

void RaceSfx::resume()
{
    for (Playing &playing : playing_)
    {
        playing.voice.resume();
    }
}

void RaceSfx::stopAll()
{
    playing_.clear();
}

// a generic sound, or the craft's ship sound (TURBOFAIL plays TURBOFIRE's clip)
const SoundClip *RaceSfx::clipOf(int key, int craft) const
{
    if (key < kShipKey)
    {
        return size_t(key) < bank_.size() ? &bank_[size_t(key)] : nullptr;
    }
    if (craft < 0 || size_t(craft) >= ships_.size())
    {
        return nullptr;
    }
    int ship = key - kShipKey;
    if (ship == int(ShipSound::TurboFail))
    {
        ship = int(ShipSound::TurboFire);
    }
    return size_t(ship) < kShipSoundCount ? &ships_[size_t(craft)][size_t(ship)] : nullptr;
}

// sound_play_ui: the voice starts at the sound's MAXVOL, a single-file one also on its 2D speaker voices
// (mix::speakerShare; a positional one gets them with its direction each update, distance rolloff included)
int RaceSfx::start(Playing playing, Start how)
{
    const SoundClip *clip = clipOf(playing.key, playing.craft);
    if (!clip || !clip->pcm)
    {
        return -1;
    }
    playing.options = clip->options;
    if (!inRange(playing) || !startVoice(playing, *clip, how))
    {
        return -1;
    }
    if (!playing.options.positional)
    {
        playing.voice.startRouting(output_.routingWithSpeakers({}, playing.options.speakers));
    }
    playing.handle = nextHandle_++;
    playing.volumeMb = clip->options.volume.maxMb;
    playing.startedAt = clock_;
    applyLevels(playing);
    playing_.push_back(std::move(playing));
    return playing_.back().handle;
}

bool RaceSfx::startVoice(Playing &playing, const SoundClip &clip, Start how)
{
    const Pcm &pcm = *clip.pcm;
    playing.voice.setBus(busOf(clip.options));
    const bool hasLoop = pcm.sampleLoopEnd > pcm.sampleLoopBegin;
    if (how == Start::HoldingLoop && hasLoop)
    {
        return playing.voice.startLooping(output_, clip.pcm, Voice::Region{pcm.sampleLoopBegin, pcm.sampleLoopEnd});
    }
    if (how == Start::Tail && hasLoop)
    {
        return playing.voice.startFrom(output_, clip.pcm, pcm.sampleLoopEnd);
    }
    return playing.voice.start(output_, clip.pcm, clip.options.loop ? Playback::Looped : Playback::Once);
}

// sound_play_ui: the listener filter is on positional sounds only (the record's +0xdd)
Bus RaceSfx::busOf(const SoundOptions &options) const
{
    return options.positional ? Bus::World : Bus::Sfx;
}

// the craft's sound of that key: not released, not finished
bool RaceSfx::holds(const Playing &playing, int key, int craft) const
{
    return playing.key == key && playing.craft == craft && !playing.released && !playing.voice.finished();
}

bool RaceSfx::isPlayingKey(int key, int craft) const
{
    for (const Playing &playing : playing_)
    {
        if (holds(playing, key, craft))
        {
            return true;
        }
    }
    return false;
}

void RaceSfx::stopKey(int key, int craft)
{
    for (auto it = playing_.begin(); it != playing_.end();)
    {
        it = holds(*it, key, craft) ? playing_.erase(it) : it + 1;
    }
}

glm::vec3 RaceSfx::positionOf(const Playing &playing) const
{
    if (playing.atPoint)
    {
        return playing.point;
    }
    if (playing.craft >= 0 && size_t(playing.craft) < craftPositions_.size())
    {
        return craftPositions_[size_t(playing.craft)];
    }
    return listener_.position;
}

// 0xc3bb0: a positional sound starts only within its MAXDIST of the listener
bool RaceSfx::inRange(const Playing &playing) const
{
    if (!playing.options.positional || centred_)
    {
        return true;
    }
    return glm::length(positionOf(playing) - listener_.position) <= playing.options.maxDist;
}

void RaceSfx::applyLevels(Playing &playing) const
{
    const SoundOptions &options = playing.options;
    const float gain = linearGain(playing.volumeMb) * sfxVolume_;
    playing.voice.setVolume(1.f);
    if (!options.positional || centred_)
    {
        playing.voice.setGains(gain, gain);
        return;
    }
    const glm::vec3 at = positionOf(playing);
    const float faded = gain * sound::rolloffGain(glm::length(at - listener_.position), options.minDist, options.maxDist);
    playing.voice.setGains(faded, faded);
    playing.voice.setRouting(output_.routingWithSpeakers(hrtf::directionOf(spatial::headOf(listener_), at), options.speakers));
}

RaceSfx::Playing *RaceSfx::find(int handle)
{
    for (Playing &playing : playing_)
    {
        if (playing.handle == handle)
        {
            return &playing;
        }
    }
    return nullptr;
}

void RaceSfx::dropFinished()
{
    for (auto it = playing_.begin(); it != playing_.end();)
    {
        it = it->voice.finished() ? playing_.erase(it) : it + 1;
    }
}

}  // namespace argon
