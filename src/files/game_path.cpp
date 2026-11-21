#include "files/game_path.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace fs = std::filesystem;

namespace argon
{
namespace
{

std::string lowerUtf8(const fs::path &name)
{
    const std::u8string utf8 = name.u8string();
    std::string lower(utf8.begin(), utf8.end());
    for (auto &character : lower)
    {
        character = char(std::tolower(static_cast<unsigned char>(character)));
    }
    return lower;
}

fs::path withForwardSlashes(const fs::path &path)
{
    std::u8string text = path.u8string();
    std::replace(text.begin(), text.end(), u8'\\', u8'/');
    return fs::path(text);
}

// the entry of `folder` called `name` ignoring case; empty when there is none
fs::path findEntryIgnoringCase(const fs::path &folder, const fs::path &name)
{
    const std::string wanted = lowerUtf8(name);
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(folder.empty() ? fs::path(".") : folder, ec))
    {
        if (lowerUtf8(entry.path().filename()) == wanted)
        {
            return folder / entry.path().filename();
        }
    }
    return {};
}

}  // namespace

fs::path findGamePath(const fs::path &path)
{
    const fs::path requested = withForwardSlashes(path);
    std::error_code ec;
    if (fs::exists(requested, ec))
    {
        return requested;
    }
    fs::path found = requested.root_path();
    for (const fs::path &part : requested.relative_path())
    {
        if (fs::exists(found / part, ec))
        {
            found /= part;
            continue;
        }
        found = findEntryIgnoringCase(found, part);
        if (found.empty())
        {
            return requested;
        }
    }
    return found;
}

bool gamePathExists(const fs::path &path)
{
    std::error_code ec;
    return fs::exists(findGamePath(path), ec);
}

}  // namespace argon
