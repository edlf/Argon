// Argon: the game's text in the language chosen in settings.json: the string tables of Locale\<language>\
// (string_table.hpp; fe_strings.str, race_strings.str, infotext.str, strings.str) and the viewer's own text
// (viewer_text.hpp). A string missing from its table reads as the fallback the caller gives.
#pragma once
#include "saves/viewer_settings.hpp"

#include <map>
#include <string>

struct ViewerText;

struct GameStringTables
{
    std::map<int, std::u16string> frontEnd;   // fe_strings.str
    std::map<int, std::u16string> race;       // race_strings.str
    std::map<int, std::u16string> info;       // infotext.str
    std::map<int, std::u16string> general;    // strings.str
};

class GameStrings
{
  public:
    GameStrings() = default;
    GameStrings(GameStringTables tables, Language language);

    std::u16string frontEnd(int id, const char *fallback) const;
    std::u16string frontEnd(int id, const ViewerText &fallback) const;   // id -1: the viewer's own text, translated
    std::u16string race(int id, const char *fallback) const;
    std::u16string info(int id, const char *fallback) const;
    std::u16string general(int id, const char *fallback) const;
    std::u16string viewer(const ViewerText &text) const;   // in the tables' language

  private:
    GameStringTables tables_;
    Language language_ = Language::English;
};

// the language's fe, race, info and strings tables from its locale folder (<game>\Locale\<language>\)
GameStrings readGameStrings(const std::string &localeFolder, Language language);
