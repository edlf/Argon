// Argon: the language's string tables read from its locale folder (ui\game_strings.hpp readGameStrings).
#include "ui/game_strings.hpp"

#include <utility>

#include "files/read_bytes.hpp"
#include "files/string_table.hpp"

GameStrings readGameStrings(const std::string &localeFolder, Language language)
{
    GameStringTables tables;
    tables.race = argon::parseStringTable(readBytes(localeFolder + "race_strings.str"));
    tables.frontEnd = argon::parseStringTable(readBytes(localeFolder + "fe_strings.str"));
    tables.info = argon::parseStringTable(readBytes(localeFolder + "infotext.str"));
    tables.general = argon::parseStringTable(readBytes(localeFolder + "strings.str"));
    return GameStrings(std::move(tables), language);
}
