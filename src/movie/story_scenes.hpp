// Which story scene plays when (docs\sequences\scenes.md "Story scenes"): the tournament's scene before each race (loader_run
// 0x80090, the table 0x1b4158) and the one after its last race (the race page's leave 0x7fa50, the tables 0x1b8958 /
// 0x1b8dd8). Pure rules; the scenes themselves play in movie\story_scene.cpp.
#pragma once
#include <optional>
#include <string>

namespace argon::story
{

constexpr int kPilots = 16, kRouteTracks = 16;   // the tracks 0..15 have story scenes; Mars (16 / 17) none

// the story name of a pilot in the scene file names (Zaki, Ami, Pharaoh, Kiri, Max, Kahekili for the folders darius,
// abi, zafir, kaitlyn, lukas, kukailimoku)
std::string storyName(int pilot);

// 0x80354: before every tournament race, data\movies\<folder>\<folder>_<pilot>.nms (0x1b4158 + (track x 16 + pilot) x
// 64); many are missing on the disc (the scene then does not play)
std::string preRaceScene(const std::string &trackFolder, int pilot);

// 0x7fac4: leaving the race page after winning a tournament's last race (progress -1): Amateur, Expert and Master
// (levels 1..3) end on the nemesis race, <folder>_<winner>_beatnemesis.nms (0x1b8958 + track x 64: the winner is the
// track's other route's owner); Redshift (4) on the home track, <folder>_<pilot>_winning.nms (0x1b8dd8); Novice none
std::optional<std::string> raceEndScene(int level, int track, const std::string &trackFolder);

// 0x9b170: the pilot of a character folder (its first five bytes, a name's NUL included, against the folder names), -1 none
int pilotOfFolder(const std::string &folder);

} // namespace argon::story
