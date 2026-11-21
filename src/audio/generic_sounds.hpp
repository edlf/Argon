// Argon: the race's sounds read from disk (any thread): the generic ones (audio\generic\generic.txt: "SF_GENERIC_<name>
// <folder>"), a pilot's ship sounds (audio\ships\<pilot>\<pilot>.txt: "SF_SHIP_<name> <folder>"), each folder's options
// (its <folder>.txt: LOOPS, MINVOL / MAXVOL, MINDIST / MAXDIST, NOPOSITION, NOREVERB; the defaults are the sound
// record's, 0xc6a40), and audio\f_settings.txt.
#pragma once
#include <array>
#include <memory>
#include <string>

#include "audio/mix_paths.hpp"
#include "audio/sound_ids.hpp"
#include "audio/wav.hpp"
#include "logic/sound_rules.hpp"

namespace argon
{

struct SoundOptions
{
    bool loop = false;
    bool positional = true;   // NOPOSITION clears it: played on the listener, unfiltered
    sound::VolumeRange volume;
    float minDist = 20.f, maxDist = 200.f;
    mix::SpeakerFlags speakers;   // the 2D speaker voices a single-file sound also plays on
};
SoundOptions parseSoundOptions(const std::string &text);

struct SoundClip
{
    std::shared_ptr<Pcm> pcm;   // the folder's files mixed to stereo; none if the list does not name it
    SoundOptions options;
};
using GenericSoundBank = std::array<SoundClip, kGenericSoundCount>;
using ShipSounds = std::array<SoundClip, kShipSoundCount>;

GenericSoundBank readGenericSounds(const std::string &game);
ShipSounds readShipSounds(const std::string &game, const std::string &pilot);
sound::ImpactSettings readImpactSettings(const std::string &game);

}  // namespace argon
