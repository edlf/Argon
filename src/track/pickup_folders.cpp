// Argon: pickup model folders (pickup_folders.hpp)
#include "track/pickup_folders.hpp"

#include <cctype>

std::string pickupFolder(const std::string &type)
{
    std::string file;
    for (char ch : type)
    {
        file += char(std::tolower(unsigned(ch)));
    }
    if (file.rfind("powerup", 0) == 0 && file.size() > 7 && file[7] != '_')
    {
        file.insert(7, "_");
    }
    return file;
}

// Fruity Fruit (pickups_init 0x3e527 reads the egg 0x1bbc16): objects\powerup_red_ee, powerup_red_ee_ns, megaup_ee,
// powerchip_ee, ...
std::string fruityPickupFolder(const std::string &type)
{
    std::string file = pickupFolder(type);
    const bool taken = file.size() > 3 && file.compare(file.size() - 3, 3, "_ns") == 0;
    file.insert(taken ? file.size() - 3 : file.size(), "_ee");
    return file;
}
