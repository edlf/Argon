// Argon: one sound file decoded to PCM, whatever it holds (no audio device, no game state: also argon_decode's).
#pragma once
#include <filesystem>
#include <string>

#include "audio/wav.hpp"

namespace argon
{

// a sound by its contents: FLAC ("fLaC", dr_flac) and Ogg Vorbis ("OggS", stb_vorbis) for custom music and the converted
// game sounds, MP3 by its name, else WAV; throws std::runtime_error naming the file and what is wrong
Pcm loadSound(const std::filesystem::path &path);

std::string lower(std::string text);
std::string pathKey(const std::filesystem::path &path);   // a path's case-insensitive cache key

}  // namespace argon
