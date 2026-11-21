// Argon: reading a string table (string_table.hpp).
#include "files/string_table.hpp"

#include "original/record_read.hpp"
#include "original/text_records.hpp"

namespace argon
{

namespace
{

using original::readAt;
using original::StringBlockHeader;
using original::StringEntry;

// the 0-terminated UTF-16 string at `at`, up to the end of the file
std::u16string stringAt(const std::vector<uint8_t> &bytes, size_t at)
{
    std::u16string text;
    for (size_t p = at; p + 1 < bytes.size(); p += 2)
    {
        const char16_t unit = char16_t(bytes[p] | (bytes[p + 1] << 8));
        if (!unit)
        {
            break;
        }
        text += unit;
    }
    return text;
}

} // namespace

std::map<int, std::u16string> parseStringTable(const std::vector<uint8_t> &bytes)
{
    std::map<int, std::u16string> strings;
    if (bytes.size() < sizeof(original::StringTableHeader))
    {
        return strings;
    }
    const size_t block = readAt<original::StringTableHeader>(bytes, 0).firstBlock;
    const size_t firstEntry = block + sizeof(StringBlockHeader);
    if (firstEntry > bytes.size())
    {
        return strings;
    }
    const uint32_t count = readAt<StringBlockHeader>(bytes, block).count;
    const size_t stringsStart = firstEntry + (size_t(count) + 1) * sizeof(StringEntry);
    for (uint32_t i = 0; i < count && firstEntry + (i + 1) * sizeof(StringEntry) <= bytes.size(); ++i)
    {
        const auto entry = readAt<StringEntry>(bytes, firstEntry + i * sizeof(StringEntry));
        strings[entry.id] = stringAt(bytes, stringsStart + size_t(entry.offset) * 2);
    }
    return strings;
}

namespace
{
bool looksPseudoLocalised(const std::u16string &text)
{
    return text.size() > 2 && text.front() == u'x' && text.back() == u'x' && text.find(u"öäßü") != std::u16string::npos;
}
} // namespace

bool isPlaceholderTranslation(const std::map<int, std::u16string> &strings)
{
    size_t padded = 0;
    for (const auto &[id, text] : strings)
    {
        padded += looksPseudoLocalised(text) ? 1 : 0;
    }
    return strings.empty() || padded * 2 > strings.size();
}

std::u16string widenLatin1(const char *text)
{
    std::u16string wide;
    for (; *text; ++text)
    {
        wide += char16_t(static_cast<unsigned char>(*text));
    }
    return wide;
}

} // namespace argon
