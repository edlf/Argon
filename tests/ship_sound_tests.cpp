// Argon: tests for a craft's ship sounds: the pilot's sound list (audio/ship_sound_list.hpp) and when the
// engine power down / up sounds play (logic/engine_power_sound.hpp). No window, GPU or sound card: run with test.bat
// / test.sh (or ctest in build).
#include "audio/ship_sound_list.hpp"
#include "check.hpp"
#include "logic/engine_power_sound.hpp"

#include <cstdio>
#include <sstream>

namespace
{
using argon::test::checks;
using argon::test::failures;

std::map<std::string, std::string> listOf(const char *text)
{
    std::istringstream list(text);
    return argon::readShipSoundList(list);
}

void testTabSeparatedEntryIsRead()
{
    CHECK(listOf("SF_SHIP_TURBOFIRE\tdo_turbo_comp\r\n").at("SF_SHIP_TURBOFIRE") == "do_turbo_comp");
}

void testSpaceSeparatedEntryIsRead()
{
    CHECK(listOf("SF_SHIP_MEGAFIRE gn_mlaunch_03\n").at("SF_SHIP_MEGAFIRE") == "gn_mlaunch_03");
}

void testLinesWithoutAFolderAreSkipped()
{
    CHECK(listOf("\nSF_SHIP_ENGINEPOWERUP\n").empty());
}

void testFirstEngineOffHitSinceBootIsSilent()
{
    argon::EnginePowerSound sound;
    CHECK(!sound.powerDown());
}

void testEngineRestartPlaysPowerUp()
{
    argon::EnginePowerSound sound;
    sound.powerDown();
    CHECK(sound.powerUp());
}

void testHitAfterARestartPlaysPowerDown()
{
    argon::EnginePowerSound sound;
    sound.powerUp();
    CHECK(sound.powerDown());
}

void testSecondHitWhileOffIsSilent()
{
    argon::EnginePowerSound sound;
    sound.powerUp();
    sound.powerDown();
    CHECK(!sound.powerDown());
}
}  // namespace

int main()
{
    testTabSeparatedEntryIsRead();
    testSpaceSeparatedEntryIsRead();
    testLinesWithoutAFolderAreSkipped();
    testFirstEngineOffHitSinceBootIsSilent();
    testEngineRestartPlaysPowerUp();
    testHitAfterARestartPlaysPowerDown();
    testSecondHitWhileOffIsSilent();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
