// Argon: tests for Time Attack's saved ghosts (saves\ghost_library.hpp), in a folder of their own under the system's
// temporary folder. No window or GPU: run with test.bat (or ctest in build\).
#include "check.hpp"
#include "saves/ghost_library.hpp"

#include <cstdio>
#include <fstream>
#include <string>

namespace
{
using argon::test::checks;
using argon::test::failures;

// an empty folder for one test, removed when it ends
class ScratchFolder
{
  public:
    explicit ScratchFolder(const std::string &name) : path_(std::filesystem::temp_directory_path() / ("argon_ghost_library_" + name))
    {
        std::filesystem::remove_all(path_);
    }
    ~ScratchFolder()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }
    const std::filesystem::path &path() const
    {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

ghost::Lap lapOf(int track, int pilot, float time)
{
    ghost::Lap lap;
    lap.samples.resize(2);
    lap.track = track;
    lap.pilot = pilot;
    lap.time = time;
    return lap;
}

GhostLibrary libraryIn(const ScratchFolder &folder)
{
    GhostLibrary library;
    library.setFolder(folder.path());
    return library;
}

void testSavedGhostIsListed()
{
    const ScratchFolder folder("listed");
    GhostLibrary library = libraryIn(folder);
    library.save(lapOf(3, 7, 61.5f), "flames_kumiko.ghost");
    CHECK(library.all().size() == 1 && library.all()[0].pilot == 7 && library.all()[0].time == 61.5f);
}

void testUnfinishedLapIsNotSaved()
{
    const ScratchFolder folder("unfinished");
    GhostLibrary library = libraryIn(folder);
    CHECK(!library.save(lapOf(3, 7, 0.f), "flames_kumiko.ghost"));
}

void testTrackGhostsComeByCharacter()
{
    const ScratchFolder folder("by_character");
    GhostLibrary library = libraryIn(folder);
    library.save(lapOf(1, 9, 70.f), "b.ghost");
    library.save(lapOf(2, 0, 70.f), "c.ghost");
    library.save(lapOf(1, 4, 70.f), "a.ghost");
    const std::vector<int> rows = library.ofTrack(1);
    CHECK(rows.size() == 2 && library.all()[size_t(rows[0])].pilot == 4 && library.all()[size_t(rows[1])].pilot == 9);
}

void testMissingGhostIsNotFound()
{
    const ScratchFolder folder("missing");
    GhostLibrary library = libraryIn(folder);
    library.save(lapOf(1, 4, 70.f), "a.ghost");
    CHECK(!library.find(1, 5).has_value());
}

void testRemovedGhostIsGoneFromTheFolder()
{
    const ScratchFolder folder("removed");
    GhostLibrary library = libraryIn(folder);
    library.save(lapOf(1, 4, 70.f), "a.ghost");
    library.remove(0);
    library.scan();
    CHECK(library.all().empty() && !std::filesystem::exists(folder.path() / "a.ghost"));
}

void testFileThatIsNoGhostDoesNotRead()
{
    const ScratchFolder folder("no_ghost");
    std::filesystem::create_directories(folder.path());
    std::ofstream(folder.path() / "junk.ghost") << "not a ghost";
    ghost::Lap lap;
    CHECK(!GhostLibrary::read(folder.path() / "junk.ghost", lap));
}
}  // namespace

int main()
{
    testSavedGhostIsListed();
    testUnfinishedLapIsNotSaved();
    testTrackGhostsComeByCharacter();
    testMissingGhostIsNotFound();
    testRemovedGhostIsGoneFromTheFolder();
    testFileThatIsNoGhostDoesNotRead();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
