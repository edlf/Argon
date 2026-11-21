// Argon: tests for finding the game's files whatever their case (game_path.hpp). No window or GPU: run
// with test.bat / test.sh (or ctest in build). Each test works in its own folder under the system temp folder. On
// Windows the file system ignores case itself, so the case tests prove most on Linux.
#include "files/game_path.hpp"
#include "check.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace
{
using argon::test::checks;
using argon::test::failures;

// <temp>/argon_tests/<name>/Data/Tracks/Track.TXT
fs::path freshGameFolder(const char *name)
{
    const fs::path folder = fs::temp_directory_path() / "argon_tests" / name;
    fs::remove_all(folder);
    fs::create_directories(folder / "Data" / "Tracks");
    std::ofstream(folder / "Data" / "Tracks" / "Track.TXT") << "track";
    return folder;
}

std::string firstWord(const fs::path &file)
{
    std::string word;
    std::ifstream(file) >> word;
    return word;
}

void testExactPathIsFound()
{
    const fs::path game = freshGameFolder("exact");
    CHECK(firstWord(argon::findGamePath(game / "Data" / "Tracks" / "Track.TXT")) == "track");
}

void testOtherCaseIsFound()
{
    const fs::path game = freshGameFolder("other_case");
    CHECK(firstWord(argon::findGamePath(game / "data" / "tracks" / "track.txt")) == "track");
}

void testBackslashesAreFolders()
{
    const fs::path game = freshGameFolder("backslashes");
    CHECK(firstWord(argon::findGamePath(game.generic_string() + "/data\\TRACKS\\track.txt")) == "track");
}

void testMissingFileIsNotFound()
{
    const fs::path game = freshGameFolder("missing");
    CHECK(!argon::gamePathExists(game / "data" / "tracks" / "other.txt"));
}

void testMissingFileKeepsRequestedName()
{
    const fs::path game = freshGameFolder("missing_name");
    CHECK(argon::findGamePath(game / "data" / "nothing" / "other.txt").filename() == "other.txt");
}

void testFolderInOtherCaseExists()
{
    const fs::path game = freshGameFolder("folder");
    CHECK(argon::gamePathExists(game / "DATA" / "tracks"));
}
}  // namespace

int main()
{
    testExactPathIsFound();
    testOtherCaseIsFound();
    testBackslashesAreFolders();
    testMissingFileIsNotFound();
    testMissingFileKeepsRequestedName();
    testFolderInOtherCaseExists();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
