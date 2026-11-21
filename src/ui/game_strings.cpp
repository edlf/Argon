// Argon: the game's text in the chosen language (game_strings.hpp).
#include "ui/game_strings.hpp"

#include "files/string_table.hpp"
#include "ui/viewer_text.hpp"

#include <utility>

namespace
{
std::u16string stringOr(const std::map<int, std::u16string> &table, int id, const char *fallback)
{
    const auto found = table.find(id);
    return found != table.end() ? found->second : argon::widenLatin1(fallback);
}
}  // namespace

GameStrings::GameStrings(GameStringTables tables, Language language) : tables_(std::move(tables)), language_(language)
{
}

std::u16string GameStrings::frontEnd(int id, const char *fallback) const
{
    return stringOr(tables_.frontEnd, id, fallback);
}

std::u16string GameStrings::frontEnd(int id, const ViewerText &fallback) const
{
    const auto found = tables_.frontEnd.find(id);
    return found != tables_.frontEnd.end() ? found->second : viewer(fallback);
}

std::u16string GameStrings::race(int id, const char *fallback) const
{
    return stringOr(tables_.race, id, fallback);
}

std::u16string GameStrings::info(int id, const char *fallback) const
{
    return stringOr(tables_.info, id, fallback);
}

std::u16string GameStrings::general(int id, const char *fallback) const
{
    return stringOr(tables_.general, id, fallback);
}

std::u16string GameStrings::viewer(const ViewerText &text) const
{
    return text.in(language_);
}
