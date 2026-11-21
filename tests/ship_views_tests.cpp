// Argon: tests for the ship views the Black button cycles (logic\ship_views.hpp).
// No window or sound card: run with test.bat (or ctest in build\).
#include "logic/ship_views.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr int kCockpit = 0, kClose = 1, kMid = 2, kFar = 3, kTv = 4;

void testCloseGoesToMid()
{
    // Arrange / Act / Assert
    CHECK(argon::views::nextShipView(kClose) == kMid);
}

void testFarGoesToCockpit()
{
    // Arrange / Act / Assert
    CHECK(argon::views::nextShipView(kFar) == kCockpit);
}

void testCockpitGoesToClose()
{
    // Arrange / Act / Assert
    CHECK(argon::views::nextShipView(kCockpit) == kClose);
}

void testFourPressesComeBack()
{
    // Arrange
    int view = kMid;

    // Act
    for (int press = 0; press < 4; ++press)
    {
        view = argon::views::nextShipView(view);
    }

    // Assert
    CHECK(view == kMid);
}

void testTvViewIsNotAShipView()
{
    // Arrange / Act / Assert
    CHECK(argon::views::isShipView(kFar));
    CHECK(!argon::views::isShipView(kTv));
}
}  // namespace

int main()
{
    testCloseGoesToMid();
    testFarGoesToCockpit();
    testCockpitGoesToClose();
    testFourPressesComeBack();
    testTvViewIsNotAShipView();
    std::printf("ship_views_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
