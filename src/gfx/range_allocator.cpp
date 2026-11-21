// Argon: byte ranges of one block (gfx\range_allocator.hpp).
#include "gfx/range_allocator.hpp"

#include <algorithm>

namespace argon::gfx
{

RangeAllocator::RangeAllocator(uint64_t size) : size_(size), free_{Range{0, size}}
{
}

std::optional<uint64_t> RangeAllocator::allocate(uint64_t bytes, uint64_t alignment)
{
    for (size_t i = 0; i < free_.size(); ++i)
    {
        const Range range = free_[i];
        const uint64_t start = (range.offset + alignment - 1) & ~(alignment - 1);
        const uint64_t end = range.offset + range.bytes;
        if (start + bytes > end)
        {
            continue;
        }
        free_.erase(free_.begin() + std::ptrdiff_t(i));
        if (start > range.offset)
        {
            insertFree(Range{range.offset, start - range.offset});
        }
        if (start + bytes < end)
        {
            insertFree(Range{start + bytes, end - start - bytes});
        }
        return start;
    }
    return std::nullopt;
}

void RangeAllocator::release(uint64_t offset, uint64_t bytes)
{
    insertFree(Range{offset, bytes});
}

bool RangeAllocator::empty() const
{
    return free_.size() == 1 && free_.front().offset == 0 && free_.front().bytes == size_;
}

// in offset order, joined with the free range just before and just after it
void RangeAllocator::insertFree(Range range)
{
    auto after = std::lower_bound(free_.begin(), free_.end(), range, startsBefore);
    if (after != free_.end() && range.offset + range.bytes == after->offset)
    {
        range.bytes += after->bytes;
        after = free_.erase(after);
    }
    if (after != free_.begin())
    {
        Range &before = *(after - 1);
        if (before.offset + before.bytes == range.offset)
        {
            before.bytes += range.bytes;
            return;
        }
    }
    free_.insert(after, range);
}

}  // namespace argon::gfx
