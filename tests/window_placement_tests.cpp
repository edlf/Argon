// Argon: tests for the monitor borderless fullscreen covers (window_placement.hpp). No window or GPU: run with test.bat
// (or ctest in build\).
#include "window/window_placement.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::ScreenRect;

const std::vector<ScreenRect> kTwoMonitors = {{0, 0, 1920, 1080}, {1920, 0, 2560, 1440}};

void testAWindowOnTheSecondMonitorPicksIt()
{
    CHECK(argon::monitorUnderWindow({2200, 100, 1280, 720}, kTwoMonitors) == 1);
}

void testAWindowAcrossBothPicksTheOneWithMoreOfIt()
{
    CHECK(argon::monitorUnderWindow({1500, 100, 1280, 720}, kTwoMonitors) == 1);
}

void testAWindowOffEveryMonitorPicksTheFirst()
{
    CHECK(argon::monitorUnderWindow({-5000, -5000, 800, 600}, kTwoMonitors) == 0);
}

void testRectsThatOnlyTouchDoNotOverlap()
{
    CHECK(argon::overlapArea({0, 0, 100, 100}, {100, 0, 100, 100}) == 0);
}
}  // namespace

int main()
{
    testAWindowOnTheSecondMonitorPicksIt();
    testAWindowAcrossBothPicksTheOneWithMoreOfIt();
    testAWindowOffEveryMonitorPicksTheFirst();
    testRectsThatOnlyTouchDoNotOverlap();
    std::printf("window_placement_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
