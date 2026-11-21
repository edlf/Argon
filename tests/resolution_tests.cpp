// Argon: tests for Argon Options' Resolution (window\resolution.hpp): reading it and keeping it to the sizes the monitor
// offers. No window or GPU: run with test.bat (or ctest in build\).
#include "window/resolution.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::Resolution;

const std::vector<Resolution> kOffered = {{800, 600}, {1280, 720}, {1600, 900}, {1920, 1080}};

void testParsesWidthByHeight()
{
    CHECK((argon::parseResolution("1280x720") == Resolution{1280, 720}));
}

void testRejectsTextAfterTheHeight()
{
    CHECK(!argon::parseResolution("1280x720p"));
}

void testRejectsAZeroSize()
{
    CHECK(!argon::parseResolution("0x720"));
}

void testTextRoundTrips()
{
    CHECK(argon::parseResolution(Resolution{2560, 1440}.text()) == (Resolution{2560, 1440}));
}

void testAnOfferedSizeIsKept()
{
    CHECK((argon::validResolution(Resolution{1280, 720}, kOffered) == Resolution{1280, 720}));
}

void testASizeTooBigForTheMonitorTakesTheLargestOffered()
{
    CHECK((argon::validResolution(Resolution{3840, 2160}, kOffered) == Resolution{1920, 1080}));
}

void testASizeNotOfferedTakesTheLargest()
{
    CHECK((argon::validResolution(Resolution{1700, 1000}, kOffered) == Resolution{1920, 1080}));
}

void testNoneStoredTakesTheLargest()
{
    CHECK((argon::validResolution(std::nullopt, kOffered) == Resolution{1920, 1080}));
}

void testTheLargestHasTheMostPixelsNotTheMostWidth()
{
    CHECK((argon::largestResolution({{2560, 1080}, {2048, 1536}}) == Resolution{2048, 1536}));
}

void testNothingOfferedKeepsTheSize()
{
    CHECK((argon::validResolution(Resolution{1234, 567}, {}) == Resolution{1234, 567}));
}

void testNothingOfferedAndNoneStoredIsTheDefault()
{
    CHECK(argon::validResolution(std::nullopt, {}) == Resolution());
}
}  // namespace

int main()
{
    testParsesWidthByHeight();
    testRejectsTextAfterTheHeight();
    testRejectsAZeroSize();
    testTextRoundTrips();
    testAnOfferedSizeIsKept();
    testASizeTooBigForTheMonitorTakesTheLargestOffered();
    testASizeNotOfferedTakesTheLargest();
    testNoneStoredTakesTheLargest();
    testTheLargestHasTheMostPixelsNotTheMostWidth();
    testNothingOfferedKeepsTheSize();
    testNothingOfferedAndNoneStoredIsTheDefault();
    std::printf("resolution_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
