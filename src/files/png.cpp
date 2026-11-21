#include "files/png.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace argon
{

static std::array<uint32_t, 256> makeCrcTable()
{
    std::array<uint32_t, 256> table{};
    for (uint32_t byte = 0; byte < 256; ++byte)
    {
        uint32_t crc = byte;
        for (int bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
        }
        table[byte] = crc;
    }
    return table;
}

static uint32_t crc(const uint8_t *bytes, size_t size, uint32_t running = 0xffffffffu)
{
    static const std::array<uint32_t, 256> table = makeCrcTable();
    for (size_t i = 0; i < size; ++i)
    {
        running = table[(running ^ bytes[i]) & 0xff] ^ (running >> 8);
    }
    return running;
}

// zlib's Adler-32; the sums are reduced once per 5552 bytes, the most that cannot overflow 32 bits
static uint32_t adler32(const std::vector<uint8_t> &bytes)
{
    constexpr uint32_t kModulus = 65521;
    constexpr size_t kRun = 5552;
    uint32_t a = 1, b = 0;
    for (size_t start = 0; start < bytes.size(); start += kRun)
    {
        const size_t end = std::min(bytes.size(), start + kRun);
        for (size_t i = start; i < end; ++i)
        {
            a += bytes[i];
            b += a;
        }
        a %= kModulus;
        b %= kModulus;
    }
    return (b << 16) | a;
}

static void be32(std::vector<uint8_t> &v, uint32_t x)
{
    for (int s = 24; s >= 0; s -= 8)
    {
        v.push_back(uint8_t(x >> s));
    }
}

static void chunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &body)
{
    be32(out, uint32_t(body.size()));
    size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), body.begin(), body.end());
    be32(out, crc(out.data() + start, out.size() - start) ^ 0xffffffffu);
}

bool writePng(const std::string &path, uint32_t w, uint32_t h, const uint8_t *rgb)
{
    std::vector<uint8_t> raw;
    raw.reserve(size_t(h) * (w * 3 + 1));
    for (uint32_t y = 0; y < h; ++y)
    {
        raw.push_back(0);
        raw.insert(raw.end(), rgb + size_t(y) * w * 3, rgb + size_t(y + 1) * w * 3);
    }
    std::vector<uint8_t> z = {0x78, 0x01};   // zlib, stored blocks
    z.reserve(raw.size() + raw.size() / 65535 * 5 + 16);
    for (size_t o = 0; o < raw.size(); o += 65535)
    {
        size_t size = std::min<size_t>(65535, raw.size() - o);
        z.push_back(o + size == raw.size() ? 1 : 0);
        z.push_back(uint8_t(size));
        z.push_back(uint8_t(size >> 8));
        z.push_back(uint8_t(~size));
        z.push_back(uint8_t(~size >> 8));
        z.insert(z.end(), raw.begin() + o, raw.begin() + o + size);
    }
    be32(z, adler32(raw));

    std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    std::vector<uint8_t> ihdr;
    be32(ihdr, w);
    be32(ihdr, h);
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});
    FILE *file = std::fopen(path.c_str(), "wb");
    if (!file)
    {
        return false;
    }
    bool ok = std::fwrite(out.data(), 1, out.size(), file) == out.size();
    std::fclose(file);
    return ok;
}

} // namespace argon
