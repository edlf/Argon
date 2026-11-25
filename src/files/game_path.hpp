// Argon: finding the game's files whatever the case. The Xbox ignores case and the game's names mix it
// (Locale\English, audio\generic\bronze\Bronze_L.wav), and the game's text files use '\' between folders. Windows finds
// such a path as written; Linux needs every folder level matched by name, ignoring case.
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace argon
{

// the file or folder on disk that `path` names ('\' or '/' between folders, any case); `path` itself, with '/' between
// folders, when nothing matches
std::filesystem::path findGamePath(const std::filesystem::path &path);

bool gamePathExists(const std::filesystem::path &path);

// the whole file at findGamePath(path); false when it cannot be read
bool readFile(const std::string &path, std::vector<uint8_t> &out);

}  // namespace argon
