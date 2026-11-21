// Argon: the crafts' stats file (0x72390) and the pilots' names (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
#include "craft/craftsim.hpp"
#include "files/game_path.hpp"
#include "logic/blast_push.hpp"
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

namespace argon
{

// ------------------------------------------------------------------ stats (0x72390)
void CraftStats::load(const std::string &path)
{
    std::ifstream f(findGamePath(path));
    if (!f)
    {
        throw std::runtime_error("can't open " + path);
    }
    v_.clear();
    std::memset(gears_, 0, sizeof gears_);
    std::string line;
    int gearset = 0;
    while (std::getline(f, line))
    {
        std::vector<std::string> tokens;
        std::string token;
        for (char character : line)
        {
            if (character == '\t' || character == '\r')
            {
                tokens.push_back(token);
                token.clear();
            }
            else
            {
                token += character;
            }
        }
        tokens.push_back(token);
        std::string key;
        std::vector<float> vals;
        for (auto &cell : tokens)
        {
            size_t first = cell.find_first_not_of(' '), last = cell.find_last_not_of(' ');
            if (first == std::string::npos)
            {
                continue;
            }
            std::string field = cell.substr(first, last - first + 1);
            char *end = nullptr;
            float x = std::strtof(field.c_str(), &end);
            if (end && *end == 0 && end != field.c_str())
            {
                if (!key.empty())
                {
                    vals.push_back(x);
                }
            }
            else if (key.empty())
            {
                key = field;
            }
        }
        if (key.empty() || vals.empty())
        {
            continue;
        }
        if (key == "gears_gearset")
        {
            gearset = std::clamp(int(vals[0]) - 1, 0, 4);
            continue;
        }
        if (key.rfind("gears_gear", 0) == 0 && key != "gears_gearset")
        {
            int gear = std::atoi(key.c_str() + 10) - 1;
            if (gear >= 0 && gear < 10)
            {
                for (int p = 0; p < 16; ++p)
                {
                    gears_[gearset][gear][p] = vals[std::min<size_t>(p, vals.size() - 1)];
                }
            }
            continue;
        }
        if (!v_.count(key))
        {
            v_[key] = vals;      // first occurrence wins (as the parser's fixed slots)
        }
    }
    if (v_.empty())
    {
        throw std::runtime_error("no values in " + path);
    }
}

float CraftStats::get(const char *key, int pilot) const
{
    auto it = v_.find(key);
    if (it == v_.end() || it->second.empty())
    {
        return 0.f;
    }
    return it->second[std::min<size_t>(size_t(std::max(pilot, 0)), it->second.size() - 1)];
}


static const char *kPilots[16] = {
    "tara", "yuri", "kumiko", "darius", "jordan", "marcus", "leah", "kukai", "alicia", "vladimir", "abi", "zafir", "kaitlyn", "jaya", "lukas", "kutoo"};
static const char *kPilotNames[16] = {
    "Tara", "Yuri", "Kumiko", "Darius", "Jordan", "Marcus", "Leah", "Kukailimoku", "Alicia", "Vladimir", "Abi", "Pharaoh", "Kaitlyn", "Jaya", "Lukas", "Kutoo"};
int pilotIndex(const std::string &folder)
{
    for (int i = 0; i < 16; ++i)
    {
        if (folder == kPilots[i])
        {
            return i;
        }
    }
    return 0;
}
const char *pilotName(int pilot)
{
    return kPilotNames[std::clamp(pilot, 0, 15)];
}
const char *pilotFolder(int pilot)
{
    return kPilots[std::clamp(pilot, 0, 15)];
}

} // namespace argon
