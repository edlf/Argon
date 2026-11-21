// Argon: Time Attack's saved ghosts. The game keeps them on the save device (FE GHOSTSELECT 0x241b0 checks for
// 0x100000 bytes free; the list 0x459fbc: name, character +0x208, track +0x20c), one per character and track (fe 0x1a5
// "A ghost already exists for this character and track."). The viewer keeps one file each in
// <saves>\ghosts\<track>_<pilot>.ghost (logic\ghost.hpp encode / decode), listed by track then character. The pages:
// ui\ghostselect.cpp, ui\ghostsave.cpp and FE GHOSTMANAGE in ui\records.cpp.
#pragma once
#include "logic/ghost.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct SavedGhost
{
    std::filesystem::path file;
    int track = -1, pilot = -1, speed = 0;
    float time = 0;
};

class GhostLibrary
{
  public:
    void setFolder(const std::filesystem::path &folder);
    void scan();   // every readable ghost file in the folder

    const std::vector<SavedGhost> &all() const;
    std::vector<int> ofTrack(int track) const;   // indices into all()
    std::optional<size_t> find(int track, int pilot) const;

    // the lap as its character's ghost on its track, as fileName in the folder (replacing the old one); false when
    // the lap is no ghost or the file could not be written
    bool save(const ghost::Lap &lap, const std::string &fileName);
    void remove(size_t index);

    static bool read(const std::filesystem::path &file, ghost::Lap &lap);   // false for a file that is no ghost

  private:
    std::filesystem::path folder_;
    std::vector<SavedGhost> ghosts_;
};
