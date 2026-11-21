// Argon: a file's bytes, none when it cannot be read (the callers treat a missing file as empty: no string table,
// no sequence, no weapon book).
#pragma once
#include "model/cmm.hpp"

#include <cstdint>
#include <string>
#include <vector>

inline std::vector<uint8_t> readBytes(const std::string &path)
{
    std::vector<uint8_t> bytes;
    if (!argon::readFile(path, bytes))
    {
        bytes.clear();
    }
    return bytes;
}
