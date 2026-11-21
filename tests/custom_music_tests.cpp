// Argon: tests for custom music (audio\custom_music.hpp): the soundtracks read from a folder of song files (empty
// files under the system's temporary folder; only their names count), the playlist and its profile string. No window
// or GPU: run with test.bat (or ctest in build\).
#include "audio/custom_music.hpp"
#include "check.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>

namespace
{
using argon::test::checks;
using argon::test::failures;
namespace fs = std::filesystem;

// a custom_music folder for one test: "Alpha" with two songs, "Beta" with one and a text file, a loose song; removed
// when the test ends
class MusicFolder
{
  public:
    explicit MusicFolder(const std::string &name) : path_(fs::temp_directory_path() / ("argon_custom_music_" + name))
    {
        fs::remove_all(path_);
        addFile("Alpha/one.wav");
        addFile("Alpha/two.ogg");
        addFile("Beta/three.mp3");
        addFile("Beta/notes.txt");
        addFile("loose.flac");
    }
    ~MusicFolder()
    {
        std::error_code error;
        fs::remove_all(path_, error);
    }
    void addFile(const std::string &name) const
    {
        fs::create_directories((path_ / name).parent_path());
        std::ofstream(path_ / name).put('\0');
    }
    const fs::path &path() const
    {
        return path_;
    }

  private:
    fs::path path_;
};

CustomMusic musicIn(const MusicFolder &folder)
{
    CustomMusic music;
    music.setFolder(folder.path());
    music.refresh(0.0);
    return music;
}

void testLooseSongsComeFirstThenEachFolder()
{
    const MusicFolder folder("order");
    const CustomMusic music = musicIn(folder);
    const SongRef loose{0, 0}, alphaTwo{1, 1}, betaThree{2, 0};
    CHECK(music.label(loose) == u"custom_music\\loose" && music.label(alphaTwo) == u"Alpha\\two" && music.label(betaThree) == u"Beta\\three");
}

void testFilesThatAreNoSongsAreLeftOut()
{
    const MusicFolder folder("no_songs");
    const CustomMusic music = musicIn(folder);
    CHECK(music.songsNotInPlaylist().size() == 4);
}

void testAddedSongLeavesTheOtherList()
{
    const MusicFolder folder("added");
    CustomMusic music = musicIn(folder);
    const SongRef added{1, 0};
    music.addToPlaylist(added);
    const std::vector<SongRef> others = music.songsNotInPlaylist();
    CHECK(music.playlist().size() == 1 && std::find(others.begin(), others.end(), added) == others.end());
}

void testPlaylistStringFindsTheSongsAgain()
{
    const MusicFolder folder("string");
    CustomMusic written = musicIn(folder);
    written.addToPlaylist({2, 0});
    written.addToPlaylist({1, 1});
    CustomMusic read = musicIn(folder);
    read.readPlaylistString(written.playlistString());
    CHECK(read.playlist() == written.playlist());
}

void testPlaylistFollowsItsSongsWhenTheFolderChanges()
{
    const MusicFolder folder("changed");
    CustomMusic music = musicIn(folder);
    music.addToPlaylist({1, 0});   // Alpha\one
    folder.addFile("Aardvark/zero.wav");
    music.refresh(5.0);
    CHECK(music.label(music.playlist().front()) == u"Alpha\\one");
}

void testFullPlaylistTakesNoMore()
{
    const MusicFolder folder("full");
    CustomMusic music = musicIn(folder);
    for (size_t slot = 0; slot < CustomMusic::kPlaylistSlots; ++slot)
    {
        music.addToPlaylist({1, 0});
    }
    music.addToPlaylist({2, 0});
    CHECK(music.playlistFull() && music.playlist().size() == CustomMusic::kPlaylistSlots);
}

void testRemovedSongClosesTheGap()
{
    const MusicFolder folder("removed");
    CustomMusic music = musicIn(folder);
    const SongRef kept{1, 1};
    music.addToPlaylist({1, 0});
    music.addToPlaylist(kept);
    music.removeFromPlaylist(0);
    CHECK(music.playlist().size() == 1 && music.playlist().front() == kept);
}

void testEmptyFolderHasNoCustomMusic()
{
    CustomMusic music;
    music.setFolder(fs::temp_directory_path() / "argon_custom_music_none");
    music.refresh(0.0);
    CHECK(!music.available());
}
}  // namespace

int main()
{
    testLooseSongsComeFirstThenEachFolder();
    testFilesThatAreNoSongsAreLeftOut();
    testAddedSongLeavesTheOtherList();
    testPlaylistStringFindsTheSongsAgain();
    testPlaylistFollowsItsSongsWhenTheFolderChanges();
    testFullPlaylistTakesNoMore();
    testRemovedSongClosesTheGap();
    testEmptyFolderHasNoCustomMusic();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
