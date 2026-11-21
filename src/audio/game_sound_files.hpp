// Argon: the game's sound files by their .wav names. tools\assets\convert_assets.py writes <name>.flac (the music) and
// <name>.ogg (the sound effects) next to each game .wav, and tools\assets\delete_converted_originals.py may then delete the
// .wav: the viewer still lists a folder's sounds and names each one by its .wav (audio.cpp loadGameSound loads the
// converted copy in its place), so the pairing (*_L / *_R, Amb_1.L.wav) and the order stay the game's.
#pragma once
#include <filesystem>
#include <vector>

namespace argon
{

// the converted copies tools\assets\convert_assets.py writes next to a game .wav (any case), in the order to prefer them:
// <name>.flac (the music, lossless) and <name>.ogg (the sound effects); none for other files
std::vector<std::filesystem::path> convertedSoundCandidates(const std::filesystem::path &wav);

// a folder's sounds from its files: one <name>.wav for each name that has a .wav, .flac or .ogg file (extensions in any
// case; the .wav's own spelling when it is there), sorted; other files are left out
std::vector<std::filesystem::path> gameSoundNames(const std::vector<std::filesystem::path> &files);

// gameSoundNames of the files in `folder` (none when it is missing)
std::vector<std::filesystem::path> listGameSounds(const std::filesystem::path &folder);

}  // namespace argon
