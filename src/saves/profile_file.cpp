// Argon: a profile's JSON file, see profile_file.hpp.
#include "saves/profile_file.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "files/utf.hpp"

using nlohmann::json;

namespace argon
{
namespace
{
// a bit mask: written as a "0x..." string; read from such a string, a plain number or a decimal string
uint32_t readMask(const json &root, const char *key, uint32_t fallback)
{
    const auto value = root.find(key);
    if (value == root.end())
    {
        return fallback;
    }
    if (value->is_number_unsigned())
    {
        return value->get<uint32_t>();
    }
    if (value->is_string())
    {
        return uint32_t(std::stoul(value->get<std::string>(), nullptr, 0));
    }
    return fallback;
}

std::string maskText(uint32_t mask, int digits)
{
    char text[16];
    std::snprintf(text, sizeof text, "0x%0*x", digits, mask);
    return text;
}

json parseFile(const std::filesystem::path &file)
{
    std::ifstream in(file);
    return json::parse(in);
}

Profile profileFrom(const json &root)
{
    Profile profile;
    profile.name = utf16(root.value("name", std::string()));
    profile.valid = !profile.name.empty();
    profile.characters = uint16_t(readMask(root, "characters", profile.characters));
    profile.tracks = readMask(root, "tracks", profile.tracks);
    profile.mars = root.value("mars", profile.mars);
    profile.speeds = root.value("speeds", profile.speeds);
    profile.redshiftWins = readMask(root, "redshiftWins", profile.redshiftWins);
    profile.tourneyLevel = root.value("tourneyLevel", profile.tourneyLevel);
    profile.options = root.value("options", profile.options);
    profile.eggs = readMask(root, "eggs", profile.eggs);
    profile.eggsOn = readMask(root, "eggsOn", profile.eggsOn);
    profile.cheats = readMask(root, "cheats", profile.cheats);
    profile.cheatsOn = readMask(root, "cheatsOn", profile.cheatsOn);
    profile.cheatMenu = root.value("cheatMenu", profile.cheatMenu);
    profile.musicCustom = root.value("musicCustom", profile.musicCustom);
    profile.playlist = root.value("playlist", profile.playlist);
    profile.tourney = root.value("tourney", profile.tourney);
    profile.trophies = root.value("trophies", profile.trophies);
    profile.cash = root.value("cash", profile.cash);
    profile.upgrades = root.value("upgrades", profile.upgrades);
    profile.bestLaps = root.value("bestLaps", profile.bestLaps);
    profile.hiTourney = root.value("hiTourney", profile.hiTourney);
    profile.hiQuick = root.value("hiQuick", profile.hiQuick);
    return profile;
}

// in the order the viewer has always written them (ordered_json keeps it)
nlohmann::ordered_json jsonFrom(const Profile &profile)
{
    return nlohmann::ordered_json{{"name", utf8(profile.name)},
                                  {"characters", maskText(profile.characters, 4)},
                                  {"tracks", maskText(profile.tracks, 8)},
                                  {"mars", profile.mars},
                                  {"speeds", profile.speeds},
                                  {"percent", profile.percent},
                                  {"options", profile.options},
                                  {"eggs", maskText(profile.eggs, 2)},
                                  {"eggsOn", maskText(profile.eggsOn, 2)},
                                  {"cheats", maskText(profile.cheats, 2)},
                                  {"cheatsOn", maskText(profile.cheatsOn, 2)},
                                  {"cheatMenu", profile.cheatMenu},
                                  {"musicCustom", profile.musicCustom},
                                  {"playlist", profile.playlist},
                                  {"tourney", profile.tourney},
                                  {"trophies", profile.trophies},
                                  {"cash", profile.cash},
                                  {"upgrades", profile.upgrades},
                                  {"hiTourney", profile.hiTourney},
                                  {"hiQuick", profile.hiQuick},
                                  {"redshiftWins", maskText(profile.redshiftWins, 1)},
                                  {"tourneyLevel", profile.tourneyLevel},
                                  {"bestLaps", profile.bestLaps}};
}
}  // namespace

Profile readProfileFile(const std::filesystem::path &file)
{
    Profile profile;
    try
    {
        profile = profileFrom(parseFile(file));
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "profile %s: %s\n", file.string().c_str(), e.what());
        profile.valid = false;
    }
    profile.file = file.string();
    return profile;
}

void writeProfileFile(const Profile &profile)
{
    std::ofstream out(std::filesystem::path(profile.file), std::ios::binary);
    out << jsonFrom(profile).dump(2) << '\n';
    if (!out)
    {
        throw std::runtime_error("can't write the profile " + profile.file);
    }
}

}  // namespace argon
