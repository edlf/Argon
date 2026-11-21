// Argon: tests for the crafts' start locators (logic\grid_starts.hpp).
// No window or sound card: run with test.bat (or ctest in build\).
#include "logic/grid_starts.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

std::vector<argon::StartSlot> startsNamed(const std::vector<std::string> &names)
{
    std::vector<argon::StartSlot> starts(names.size());
    for (size_t i = 0; i < names.size(); ++i)
    {
        starts[i].name = names[i];
    }
    return starts;
}

void testPlayerStartsLastOfSix()
{
    // Arrange / Act / Assert
    CHECK(argon::grid::startNumberOfCraft(0, 6) == 5);
}

void testLastCraftTakesThePole()
{
    // Arrange / Act / Assert
    CHECK(argon::grid::startNumberOfCraft(5, 6) == 0);
}

void testLoneCraftTakesThePole()
{
    // Arrange / Act / Assert
    CHECK(argon::grid::startNumberOfCraft(0, 1) == 0);
}

void testSlotPastTheRaceKeepsItsNumber()
{
    // Arrange / Act / Assert
    CHECK(argon::grid::startNumberOfCraft(7, 6) == 7);
}

void testStartFoundByName()
{
    // Arrange
    const std::vector<argon::StartSlot> starts = startsNamed({"player_start0", "player_start1", "other", "player_start2"});

    // Act
    const size_t index = argon::grid::startIndexOf(starts, 2);

    // Assert
    CHECK(index == 3);
}

void testUnnamedStartsByPosition()
{
    // Arrange
    const std::vector<argon::StartSlot> starts = startsNamed({"a", "b", "c"});

    // Act
    const size_t index = argon::grid::startIndexOf(starts, 1);

    // Assert
    CHECK(index == 1);
}
}  // namespace

int main()
{
    testPlayerStartsLastOfSix();
    testLastCraftTakesThePole();
    testLoneCraftTakesThePole();
    testSlotPastTheRaceKeepsItsNumber();
    testStartFoundByName();
    testUnnamedStartsByPosition();
    std::printf("grid_starts_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
