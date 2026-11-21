// Argon: what audio.cpp and sound_reading.cpp share for reading sounds: the decoding cache and the folder helpers.
#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "audio/audio.hpp"
#include "audio/song_feed.hpp"
#include "audio/mix_paths.hpp"

namespace argon
{

// decodes each file once; a failed file is a warning and a null sound
struct SoundReader
{
    std::map<std::string, std::shared_ptr<Pcm>> cache;
    std::vector<std::string> *warnings = nullptr;

    std::shared_ptr<Pcm> load(const std::filesystem::path &file);
    std::shared_ptr<Pcm> loadLoop(const std::filesystem::path &file);   // load, then makeLoopable
};

std::string lower(std::string text);
std::string pathKey(const std::filesystem::path &p);   // a path's case-insensitive cache key
std::string readTextFile(const std::filesystem::path &path);
std::filesystem::path findChild(const std::filesystem::path &dir, const std::string &name);   // case-insensitive
// a file and the speaker mixbins it plays on (mix::speakerFileGains)
using SpeakerPart = std::pair<std::shared_ptr<Pcm>, mix::SpeakerGains>;
// one file as a stereo buffer, several as the six speaker mixbins (normalised so it can't clip)
std::shared_ptr<Pcm> mixSpeakerFiles(const std::vector<SpeakerPart> &parts, bool normalise);
std::shared_ptr<SongFeed> openSong(const std::filesystem::path &file);   // one song to decode as it plays, any thread
// audio\frontend ui_amb3 (mode 1, the menus) / ui_levelload (the loading screen) as one stereo loop, any thread
std::shared_ptr<Pcm> readFrontEndAmbient(const std::string &game, int mode);

} // namespace argon
