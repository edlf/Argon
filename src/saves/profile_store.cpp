// Argon: the profiles in <saves>\profiles\*.json (saves\profile_store.hpp).
#include "saves/profile_store.hpp"
#include "files/utf.hpp"
#include "saves/profile_fields.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>

namespace argon
{
namespace
{
namespace fs = std::filesystem;

bool byName(const Profile &first, const Profile &second)
{
    return first.name < second.name;
}
}  // namespace

void ProfileStore::setFolder(std::string folder)
{
    folder_ = std::move(folder);
}

void ProfileStore::load()
{
    profiles_.clear();
    std::error_code ec;
    if (fs::exists(folder_, ec))
    {
        for (const auto &entry : fs::directory_iterator(folder_, ec))
        {
            if (entry.path().extension() != ".json")
            {
                continue;
            }
            Profile profile = readProfileFile(entry.path());
            profile.percent = progressOf(profile).percentComplete();
            profiles_.push_back(profile);
        }
    }
    std::sort(profiles_.begin(), profiles_.end(), byName);
}

// the name's letters and digits (others as _), "profile" when none; _2, _3 .. while the file exists
std::string ProfileStore::fileFor(const Profile &profile) const
{
    std::string base;
    for (const char character : utf8(profile.name))
    {
        base += std::isalnum((unsigned char)character) ? character : '_';
    }
    if (base.empty())
    {
        base = "profile";
    }
    std::error_code ec;
    std::string file = folder_ + "/" + base + ".json";
    for (int n = 2; fs::exists(file, ec); ++n)
    {
        file = folder_ + "/" + base + "_" + std::to_string(n) + ".json";
    }
    return file;
}

bool ProfileStore::save(Profile &profile)
{
    std::error_code ec;
    fs::create_directories(folder_, ec);
    if (profile.file.empty())
    {
        profile.file = fileFor(profile);
    }
    profile.percent = progressOf(profile).percentComplete();
    try
    {
        writeProfileFile(profile);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        return false;
    }
    ++writes_;
    return true;
}

void ProfileStore::remove(size_t index)
{
    if (index >= profiles_.size())
    {
        return;
    }
    std::error_code ec;
    fs::remove(profiles_[index].file, ec);
    load();
}

const std::vector<Profile> &ProfileStore::all() const
{
    return profiles_;
}

void ProfileStore::use(const std::u16string &name)
{
    currentName_ = name;
}

const std::u16string &ProfileStore::currentName() const
{
    return currentName_;
}

bool ProfileStore::hasCurrent() const
{
    return !currentName_.empty();
}

std::optional<std::reference_wrapper<Profile>> ProfileStore::current()
{
    if (!hasCurrent())
    {
        return std::nullopt;
    }
    for (Profile &profile : profiles_)
    {
        if (profile.name == currentName_)
        {
            return profile;
        }
    }
    return std::nullopt;
}

void ProfileStore::saveCurrent()
{
    if (const auto profile = current())
    {
        save(*profile);
    }
}

int ProfileStore::writes() const
{
    return writes_;
}

}  // namespace argon
