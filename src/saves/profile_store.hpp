// Argon: the profiles in <saves>\profiles\*.json (docs\frontend\profiles.md "Profiles"): the list FE CHOOSESAVE shows, the one in
// use, writing them (a new profile gets a file named after it) and how many writes there have been (FE OPTIONS shows
// "Saving" when it changed one).
#pragma once
#include "saves/profile_file.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace argon
{

class ProfileStore
{
  public:
    void setFolder(std::string folder);
    // every profile file in the folder, by name, each with its Percentage Complete
    void load();
    // writes the profile (naming its file when it has none); false, with the reason on stderr, when it can't be written
    bool save(Profile &profile);
    // deletes that profile's file, then loads the list again
    void remove(size_t index);
    const std::vector<Profile> &all() const;

    // the profile in use: its name, empty for none (the game without a save)
    void use(const std::u16string &name);
    const std::u16string &currentName() const;
    bool hasCurrent() const;
    // the loaded profile in use, if the list holds it
    std::optional<std::reference_wrapper<Profile>> current();
    // writes the profile in use (nothing when none is loaded)
    void saveCurrent();
    int writes() const;

  private:
    std::string fileFor(const Profile &profile) const;

    std::string folder_;
    std::vector<Profile> profiles_;
    std::u16string currentName_;
    int writes_ = 0;
};

}  // namespace argon
