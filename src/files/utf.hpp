// Argon: UTF-16 (the game's strings, the profile names) <-> UTF-8 (the viewer's files). The BMP only, as the
// game's fonts.
#pragma once
#include <string>

inline std::string utf8(const std::u16string &text)
{
    std::string utf8;
    for (char16_t unit : text)
    {
        if (unit < 0x80)
        {
            utf8 += char(unit);
        }
        else if (unit < 0x800)
        {
            utf8 += char(0xc0 | (unit >> 6));
            utf8 += char(0x80 | (unit & 63));
        }
        else
        {
            utf8 += char(0xe0 | (unit >> 12));
            utf8 += char(0x80 | ((unit >> 6) & 63));
            utf8 += char(0x80 | (unit & 63));
        }
    }
    return utf8;
}

inline std::u16string utf16(const std::string &utf8)
{
    std::u16string text;
    for (size_t i = 0; i < utf8.size();)
    {
        unsigned char byte = (unsigned char)utf8[i];
        if (byte < 0x80)
        {
            text += char16_t(byte);
            i += 1;
        }
        else if ((byte >> 5) == 6 && i + 1 < utf8.size())
        {
            text += char16_t(((byte & 31) << 6) | (utf8[i + 1] & 63));
            i += 2;
        }
        else if ((byte >> 4) == 14 && i + 2 < utf8.size())
        {
            text += char16_t(((byte & 15) << 12) | ((utf8[i + 1] & 63) << 6) | (utf8[i + 2] & 63));
            i += 3;
        }
        else
        {
            i += 1;
        }
    }
    return text;
}
