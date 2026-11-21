// The song files read a chunk at a time for a SongFeed (audio\song_feed.hpp): FLAC (dr_flac), MP3 (dr_mp3) and Ogg
// Vorbis (stb_vorbis) decoded from the file's bytes kept in memory (a few MB, not the ~40 MB a decoded song takes);
// a WAV (Xbox ADPCM or PCM16) is decoded whole by loadSound. More than two channels: the front pair.
#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "audio/song_feed.hpp"

namespace argon
{

// the file's bytes as FLAC / Ogg Vorbis (by their magic) or MP3 (by the name); null for anything else or a bad file
std::unique_ptr<SongFrames> compressedSongFrames(const std::filesystem::path &name, std::vector<uint8_t> bytes);

}  // namespace argon
