// Reads the game's little-endian file records (src\original\*_records.hpp) out of a loaded file.
#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace argon::original
{

// The value at offset; zero-filled when it would run past the end of the file.
template <class T> T readAt(const std::vector<uint8_t> &bytes, size_t offset)
{
    static_assert(std::is_trivially_copyable_v<T>, "file records are plain bytes");
    T value{};
    if (offset + sizeof(T) <= bytes.size())
    {
        std::memcpy(&value, bytes.data() + offset, sizeof(T));
    }
    return value;
}

} // namespace argon::original
