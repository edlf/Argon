// Argon: tests for reading the string tables (string_table.hpp, Locale\<language>\*.str). No window or GPU: run with
// test.bat (or ctest in build\).
#include "original/text_records.hpp"
#include "check.hpp"
#include "files/string_table.hpp"
#include "ui/game_strings.hpp"
#include "ui/viewer_text.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

template <class Record> void append(std::vector<uint8_t> &bytes, const Record &record)
{
    const size_t at = bytes.size();
    bytes.resize(at + sizeof record);
    std::memcpy(bytes.data() + at, &record, sizeof record);
}

void appendString(std::vector<uint8_t> &bytes, const std::u16string &text)
{
    for (char16_t c : text + u'\0')
    {
        append(bytes, c);
    }
}

// one block of (id, text) strings after a header naming two blocks, as the Japanese tables have
std::vector<uint8_t> tableOf(const std::vector<std::pair<uint16_t, std::u16string>> &strings)
{
    std::vector<uint8_t> bytes;
    const uint32_t headerSize = sizeof(argon::original::StringTableHeader) + 4;
    argon::original::StringTableHeader header{{'L', 'S', 'B', 'L'}, headerSize, 2, headerSize};
    append(bytes, header);
    append(bytes, uint32_t(0));
    append(bytes, argon::original::StringBlockHeader{0, uint32_t(strings.size())});
    uint32_t offset = 0;
    for (const auto &[id, text] : strings)
    {
        append(bytes, argon::original::StringEntry{id, offset});
        offset += uint32_t(text.size() + 1);
    }
    append(bytes, argon::original::StringEntry{0xffff, offset});
    for (const auto &entry : strings)
    {
        appendString(bytes, entry.second);
    }
    return bytes;
}

void testStringsAreFoundById()
{
    const auto strings = argon::parseStringTable(tableOf({{600, u"Turbo"}, {601, u"Shield"}}));
    CHECK(strings.size() == 2 && strings.at(600) == u"Turbo" && strings.at(601) == u"Shield");
}

void testTruncatedStringEndsAtTheFileEnd()
{
    std::vector<uint8_t> bytes = tableOf({{605, u"Lap"}});
    bytes.resize(bytes.size() - 4);
    CHECK(argon::parseStringTable(bytes).at(605) == u"La");
}

void testFileShorterThanItsHeaderHasNoStrings()
{
    std::vector<uint8_t> bytes = tableOf({{605, u"Lap"}});
    bytes.resize(sizeof(argon::original::StringTableHeader) - 1);
    CHECK(argon::parseStringTable(bytes).empty());
}

void testBlockPastTheFileEndHasNoStrings()
{
    std::vector<uint8_t> bytes = tableOf({{605, u"Lap"}});
    bytes.resize(sizeof(argon::original::StringTableHeader) + 4);
    CHECK(argon::parseStringTable(bytes).empty());
}
} // namespace

void testPseudoLocalisedTableIsAPlaceholder()
{
    const auto strings = argon::parseStringTable(tableOf({{5, u"xÓñöäßüöäßüöäx"}, {8, u"xExìtöäßüöäßüöäx"}}));
    CHECK(argon::isPlaceholderTranslation(strings));
}

void testTranslatedTableIsNoPlaceholder()
{
    const auto strings = argon::parseStringTable(tableOf({{5, u"Ein"}, {8, u"Beenden"}, {0x330, u"Kopfhörer"}}));
    CHECK(!argon::isPlaceholderTranslation(strings));
}

void testEmptyTableIsAPlaceholder()
{
    CHECK(argon::isPlaceholderTranslation({}));
}

void testGameStringIsFoundInItsTable()
{
    GameStringTables tables;
    tables.race[624] = u"mph";
    const GameStrings strings(std::move(tables), Language::English);
    CHECK(strings.race(624, "MPH") == u"mph");
}

void testMissingGameStringIsTheFallback()
{
    const GameStrings strings(GameStringTables{}, Language::English);
    CHECK(strings.frontEnd(8, "Exit") == u"Exit");
}

void testMissingFrontEndStringIsTheViewerTextInTheTablesLanguage()
{
    const ViewerText save{u"Save", u"Enregistrer"};
    const GameStrings strings(GameStringTables{}, Language::French);
    CHECK(strings.frontEnd(-1, save) == u"Enregistrer");
}

int main()
{
    testStringsAreFoundById();
    testTruncatedStringEndsAtTheFileEnd();
    testFileShorterThanItsHeaderHasNoStrings();
    testBlockPastTheFileEndHasNoStrings();
    testPseudoLocalisedTableIsAPlaceholder();
    testTranslatedTableIsNoPlaceholder();
    testEmptyTableIsAPlaceholder();
    testGameStringIsFoundInItsTable();
    testMissingGameStringIsTheFallback();
    testMissingFrontEndStringIsTheViewerTextInTheTablesLanguage();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
