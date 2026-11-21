// Argon: tests for the minimap's map per track (hud/minimap_rules.hpp). Run with test.bat / test.sh.
#include "hud/minimap_rules.hpp"
#include "check.hpp"

#include <cstdio>

using namespace argon::hud;

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr int kSilver = 0, kKalau = 1, kFuusoku = 2, kDraco = 7, kAltSilver = 8, kAltKalau = 9, kAltFuusoku = 0xa;
constexpr int kAltBracken = 0xd, kAltMount = 0xe, kAltDraco = 0xf, kMars = 0x10, kAltMars = 0x11;

void firstRoutesShowTheirOwnMap()
{
    CHECK(minimapName(kSilver) == "silver" && minimapName(kDraco) == "draco");
}

void altSilverBrackenAndMountShowTheirOwnMap()
{
    CHECK(minimapName(kAltSilver) == "alt_silver" && minimapName(kAltBracken) == "alt_bracken" && minimapName(kAltMount) == "alt_mount");
}

void otherSecondRoutesShowTheFirstRoutesMap()
{
    CHECK(minimapName(kAltKalau) == "kalau" && minimapName(kAltFuusoku) == "fuusoku" && minimapName(kAltDraco) == "draco");
}

void bothMarsRoutesShowMars()
{
    CHECK(minimapName(kMars) == "mars" && minimapName(kAltMars) == "mars");
}

void anUnknownTrackShowsMars()
{
    CHECK(minimapName(-1) == "mars");
}

void fuusokuMapsAreDrawnAtElevenPointEightyEight()
{
    CHECK(minimapWorldPerTexel(kFuusoku) == 11.88f && minimapWorldPerTexel(kAltFuusoku) == 11.88f);
}

void otherMapsAreDrawnAtNinePointNine()
{
    CHECK(minimapWorldPerTexel(kKalau) == 9.9f && minimapWorldPerTexel(kAltMars) == 9.9f);
}
}  // namespace

int main()
{
    firstRoutesShowTheirOwnMap();
    altSilverBrackenAndMountShowTheirOwnMap();
    otherSecondRoutesShowTheFirstRoutesMap();
    bothMarsRoutesShowMars();
    anUnknownTrackShowsMars();
    fuusokuMapsAreDrawnAtElevenPointEightyEight();
    otherMapsAreDrawnAtNinePointNine();
    std::printf("minimap_rules_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
