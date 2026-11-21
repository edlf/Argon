#include "audio/ship_sound_list.hpp"

#include <sstream>

namespace argon
{

std::map<std::string, std::string> readShipSoundList(std::istream &list)
{
    std::map<std::string, std::string> folders;
    std::string line;
    while (std::getline(list, line))
    {
        std::istringstream words(line);
        std::string event, folder;
        if (words >> event >> folder)
        {
            folders[event] = folder;
        }
    }
    return folders;
}

}  // namespace argon
