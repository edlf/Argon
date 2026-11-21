// Argon: tests for the split screen's views and HUD frames (logic/split_screen.hpp). No window or GPU: run
// with test.bat / test.sh (or ctest in build).
#include "logic/split_screen.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

bool same(const argon::split::ScreenRect &a, const argon::split::ScreenRect &b)
{
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

void testOnePlayerHasTheWholeScreen()
{
    CHECK(same(argon::split::viewOf(1, 0), {0, 0, 640, 480}));
}

void testTwoPlayersShareTopAndBottom()
{
    CHECK(same(argon::split::viewOf(2, 1), {0, 240, 640, 240}));
}

void testThreePlayersTakeThreeQuarters()
{
    CHECK(same(argon::split::viewOf(3, 2), {0, 240, 320, 240}));
}

void testFourthPlayerTakesTheBottomRight()
{
    CHECK(same(argon::split::viewOf(4, 3), {320, 240, 320, 240}));
}

void testSinglePlayerFrameIsTheSafeArea()
{
    CHECK(same(argon::split::hudFrame({0, 0, 640, 480}), {52, 36, 536, 408}));
}

void testTopViewKeepsTwelveFromTheBottomView()
{
    CHECK(same(argon::split::hudFrame(argon::split::viewOf(2, 0)), {52, 36, 536, 192}));
}

void testQuarterFrameUsesTheInnerMarginOnInnerEdges()
{
    CHECK(same(argon::split::hudFrame(argon::split::viewOf(4, 3)), {332, 252, 256, 192}));
}

void testWideScreenKeepsTheMarginInHudUnits()
{
    CHECK(same(argon::split::hudFrame({0, 0, 853, 480}, 853), {52, 36, 749, 408}));
}
}  // namespace

int main()
{
    testOnePlayerHasTheWholeScreen();
    testTwoPlayersShareTopAndBottom();
    testThreePlayersTakeThreeQuarters();
    testFourthPlayerTakesTheBottomRight();
    testSinglePlayerFrameIsTheSafeArea();
    testTopViewKeepsTwelveFromTheBottomView();
    testQuarterFrameUsesTheInnerMarginOnInnerEdges();
    testWideScreenKeepsTheMarginInHudUnits();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
