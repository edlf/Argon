// Argon: hands out byte ranges of one fixed-size block (a GPU buffer several small buffers share, gfx\d3d12): first
// fit, freed ranges merged with their free neighbours.
#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace argon::gfx
{

class RangeAllocator
{
  public:
    explicit RangeAllocator(uint64_t size);

    // the offset of `bytes` free bytes starting at a multiple of alignment (a power of two); none when nothing fits
    std::optional<uint64_t> allocate(uint64_t bytes, uint64_t alignment);
    // a range allocate gave, with the same byte count
    void release(uint64_t offset, uint64_t bytes);
    bool empty() const;   // nothing allocated

  private:
    struct Range
    {
        uint64_t offset, bytes;
    };
    static bool startsBefore(const Range &first, const Range &second)
    {
        return first.offset < second.offset;
    }
    void insertFree(Range range);

    uint64_t size_;
    std::vector<Range> free_;   // by offset, none touching another
};

}  // namespace argon::gfx
