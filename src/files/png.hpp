#pragma once
#include <cstdint>
#include <string>

namespace argon
{
// Writes an 8-bit RGB PNG (uncompressed deflate; used for --shot screenshots).
bool writePng(const std::string &path, uint32_t w, uint32_t h, const uint8_t *rgb);
} // namespace argon
