// Argon: the ship parts that only show while a weapon or shield is active (weapon_bones.hpp).
#include "model/weapon_bones.hpp"

#include <algorithm>
#include <cctype>

namespace argon
{

namespace
{

const size_t kShortestName = 4;
const int kDeepestBone = 64;

bool nameCharacter(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool endsWith(const std::string &text, const std::string &end)
{
    return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
}

std::string withoutTrailingNumber(std::string name)
{
    while (!name.empty() && (std::isdigit(static_cast<unsigned char>(name.back())) || name.back() == '_'))
    {
        name.pop_back();
    }
    return name;
}

}  // namespace

// the file's zero-terminated names of four characters or more, then the craft loader's own
WeaponBones WeaponBones::fromWeaponFile(const std::vector<uint8_t> &weaponFile)
{
    WeaponBones bones;
    std::string name;
    for (const uint8_t byte : weaponFile)
    {
        const char ch = char(byte);
        if (nameCharacter(ch))
        {
            name += ch;
            continue;
        }
        const bool known = std::find(bones.names_.begin(), bones.names_.end(), name) != bones.names_.end();
        if (ch == 0 && name.size() >= kShortestName && !known)
        {
            bones.names_.push_back(name);
        }
        name.clear();
    }
    for (const char *loaderName : {"SPECIAL", "revol", "SHIELD", "STAR", "SHURIKEN", "RETALIATOR", "PSUN", "SNOVA", "LSTRIKE", "TRAIL"})
    {
        bones.names_.push_back(loaderName);
    }
    return bones;
}

bool WeaponBones::marks(const std::vector<CmmBone> &bones, int bone) const
{
    for (int b = bone, depth = 0; b >= 0 && b < int(bones.size()) && depth < kDeepestBone; b = bones[size_t(b)].parent, ++depth)
    {
        if (namesWeapon(bones[size_t(b)].name))
        {
            return true;
        }
    }
    return false;
}

bool WeaponBones::namesWeapon(const std::string &boneName) const
{
    const std::string stem = withoutTrailingNumber(boneName);
    for (const std::string &name : names_)
    {
        if (endsWith(stem, name))
        {
            return true;
        }
    }
    for (const char *part : {"SPECIAL", "revol", "SHIELD"})
    {
        if (boneName.find(part) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

}  // namespace argon
