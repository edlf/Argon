// A string table, Locale\<language>\*.str (original\text_records.hpp; string_table_open 0x79030): the UTF-16 strings of
// its first block by id.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace argon
{

// empty for a file too short for its header; the entries up to the end of the file for a truncated one
std::map<int, std::u16string> parseStringTable(const std::vector<uint8_t> &bytes);

// the USA disc's French, German, Italian and Spanish tables: the English text pseudo-localised, most strings between
// two "x" and padded with "öäßü" ("xExìtöäßüöäßüöäx"; docs\frontend\intro-title.md). An empty table is one too.
bool isPlaceholderTranslation(const std::map<int, std::u16string> &strings);

// the viewer's own ASCII / Latin-1 text as the tables' UTF-16 (the fallbacks of a missing string)
std::u16string widenLatin1(const char *text);

} // namespace argon
