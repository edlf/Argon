// Argon: song and sound files: custom music's songs (docs\frontend\options.md "Custom music"), the game's converted sounds'
// comments (tools\assets\convert_assets.py; their names: game_sound_files.hpp) and the MP3 / Ogg Vorbis decodes (dr_mp3 and stb_vorbis, public domain, from the CMake
// downloads; their implementations are in codecs.c).
#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "audio/wav.hpp"

struct stb_vorbis;

namespace argon
{

// an open Ogg Vorbis stream's comments ("KEY=value")
std::vector<std::string> vorbisComments(stb_vorbis *stream);

// .wav / .flac / .ogg / .mp3, any case
bool isSongFile(const std::filesystem::path &path);
bool isMp3File(const std::filesystem::path &path);

// a loop region [begin, end) in frames; both 0: none
struct SampleLoop
{
    size_t begin = 0, end = 0;
};

// the loop of a converted sound effect from its Vorbis comments, LOOPSTART=<first frame> and LOOPLENGTH=<frames>
// (keys in any case; the .wav's `smpl` loop); none unless both are whole numbers and the length is above 0
SampleLoop sampleLoopFromComments(const std::vector<std::string> &comments);

// a converted sound effect's one-shot length from its Vorbis comment PLAYLENGTH=<frames> (key in any case; the .wav's
// `fact` count, the Ogg holding every ADPCM block whole as loadWav keeps them): Pcm::playFrames, 0 (all) when there is
// none or it is not below the sound's `frames`
size_t playFramesFromComments(const std::vector<std::string> &comments, size_t frames);

// the whole file to 16-bit PCM; false when it holds no MP3 frames
bool decodeMp3(const std::vector<uint8_t> &file, Pcm &out);

// an Ogg Vorbis file (one stream) to 16-bit PCM, its LOOPSTART / LOOPLENGTH comments to the sample loop and PLAYLENGTH
// to playFrames; false when it is not one
bool decodeOggVorbis(const std::vector<uint8_t> &file, Pcm &out);

}  // namespace argon
