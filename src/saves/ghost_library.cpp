// Argon: Time Attack's saved ghosts (ghost_library.hpp).
#include "saves/ghost_library.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace
{
bool byTrackThenPilot(const SavedGhost &first, const SavedGhost &second)
{
    return first.track != second.track ? first.track < second.track : first.pilot < second.pilot;
}

SavedGhost savedGhostOf(const std::filesystem::path &file, const ghost::Lap &lap)
{
    return SavedGhost{file, lap.track, lap.pilot, lap.speed, lap.time};
}

// written beside it first, so a failed write keeps the old file
bool writeFileWhole(const std::filesystem::path &file, const std::vector<uint8_t> &bytes)
{
    std::filesystem::path partial = file;
    partial += ".tmp";
    {
        std::ofstream out(partial, std::ios::binary | std::ios::trunc);
        if (!out || !out.write(reinterpret_cast<const char *>(bytes.data()), std::streamsize(bytes.size())))
        {
            return false;
        }
    }
    std::error_code error;
    std::filesystem::rename(partial, file, error);
    return !error;
}
}  // namespace

void GhostLibrary::setFolder(const std::filesystem::path &folder)
{
    folder_ = folder;
    ghosts_.clear();
}

void GhostLibrary::scan()
{
    ghosts_.clear();
    std::error_code error;
    if (folder_.empty() || !std::filesystem::exists(folder_, error))
    {
        return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(folder_, error))
    {
        if (entry.path().extension() != ".ghost")
        {
            continue;
        }
        ghost::Lap lap;
        if (read(entry.path(), lap))
        {
            ghosts_.push_back(savedGhostOf(entry.path(), lap));
        }
    }
    std::sort(ghosts_.begin(), ghosts_.end(), byTrackThenPilot);
}

const std::vector<SavedGhost> &GhostLibrary::all() const
{
    return ghosts_;
}

std::vector<int> GhostLibrary::ofTrack(int track) const
{
    std::vector<int> indices;
    for (size_t index = 0; index < ghosts_.size(); ++index)
    {
        if (ghosts_[index].track == track)
        {
            indices.push_back(int(index));
        }
    }
    return indices;
}

std::optional<size_t> GhostLibrary::find(int track, int pilot) const
{
    for (size_t index = 0; index < ghosts_.size(); ++index)
    {
        if (ghosts_[index].track == track && ghosts_[index].pilot == pilot)
        {
            return index;
        }
    }
    return std::nullopt;
}

bool GhostLibrary::save(const ghost::Lap &lap, const std::string &fileName)
{
    if (folder_.empty() || !lap.valid() || lap.track < 0 || lap.pilot < 0)
    {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(folder_, error);
    if (!writeFileWhole(folder_ / fileName, ghost::encode(lap)))
    {
        return false;
    }
    scan();
    return true;
}

void GhostLibrary::remove(size_t index)
{
    if (index >= ghosts_.size())
    {
        return;
    }
    std::error_code error;
    std::filesystem::remove(ghosts_[index].file, error);
    scan();
}

bool GhostLibrary::read(const std::filesystem::path &file, ghost::Lap &lap)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
    {
        return false;
    }
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return ghost::decode(bytes, lap) && lap.valid();
}
