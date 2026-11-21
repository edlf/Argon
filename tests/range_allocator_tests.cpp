// Argon: tests for the shared buffer blocks' byte ranges (gfx\range_allocator.hpp). Run with test.bat / test.sh.
#include "check.hpp"
#include "gfx/range_allocator.hpp"

#include <cstdio>

using argon::gfx::RangeAllocator;

namespace
{
using argon::test::checks;
using argon::test::failures;

void rangesFollowOneAnother()
{
    RangeAllocator ranges(1024);
    const auto first = ranges.allocate(256, 256), second = ranges.allocate(256, 256);
    CHECK(first == 0u && second == 256u);
}

void startIsRoundedUpToTheAlignment()
{
    RangeAllocator ranges(1024);
    ranges.allocate(10, 1);
    CHECK(ranges.allocate(16, 256) == 256u);
}

void fullBlockGivesNothing()
{
    RangeAllocator ranges(512);
    ranges.allocate(512, 256);
    CHECK(!ranges.allocate(1, 1));
}

void releasedRangeIsGivenAgain()
{
    RangeAllocator ranges(768);
    ranges.allocate(256, 256);
    const auto middle = ranges.allocate(256, 256);
    ranges.allocate(256, 256);
    ranges.release(*middle, 256);
    CHECK(ranges.allocate(256, 256) == middle);
}

void neighbouringReleasesMergeIntoOneRange()
{
    RangeAllocator ranges(768);
    const auto a = ranges.allocate(256, 256), b = ranges.allocate(256, 256), c = ranges.allocate(256, 256);
    ranges.release(*a, 256);
    ranges.release(*c, 256);
    ranges.release(*b, 256);
    CHECK(ranges.empty() && ranges.allocate(768, 256) == 0u);
}

void alignmentGapStaysFree()
{
    RangeAllocator ranges(1024);
    ranges.allocate(10, 1);
    ranges.allocate(16, 256);
    CHECK(ranges.allocate(200, 1) == 10u);
}

}  // namespace

int main()
{
    rangesFollowOneAnother();
    startIsRoundedUpToTheAlignment();
    fullBlockGivesNothing();
    releasedRangeIsGivenAgain();
    neighbouringReleasesMergeIntoOneRange();
    alignmentGapStaysFree();
    std::printf("range_allocator_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
