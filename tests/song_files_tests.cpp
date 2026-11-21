// Argon: tests for the song and sound files (audio\song_files.hpp): which files are songs, the converted game sounds'
// names (audio\game_sound_files.hpp: also a folder listed by its .wav names once the originals are gone), their loop
// comments and the MP3 / Ogg Vorbis decodes.
// No window or sound card: run with test.bat (or ctest in build\).
#include "audio/song_files.hpp"
#include "check.hpp"
#include "audio/game_sound_files.hpp"

#include <algorithm>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

// MPEG-1 Layer III, 128 kbit/s, 44.1 kHz, mono, no padding: 144 x 128000 / 44100 = 417 bytes a frame. All-zero side
// information and main data decode to silence.
constexpr size_t kSilentFrameBytes = 417;
constexpr int kSilentFrameRate = 44100;

std::vector<uint8_t> silentMp3(int frameCount)
{
    std::vector<uint8_t> file;
    for (int i = 0; i < frameCount; ++i)
    {
        const size_t at = file.size();
        file.resize(at + kSilentFrameBytes, 0);
        file[at] = 0xff;
        file[at + 1] = 0xfb;   // MPEG-1, Layer III, no CRC
        file[at + 2] = 0x90;   // 128 kbit/s, 44.1 kHz
        file[at + 3] = 0xc0;   // mono
    }
    return file;
}

void testSongExtensionsAnyCase()
{
    // Arrange / Act / Assert
    CHECK(argon::isSongFile("a.wav"));
    CHECK(argon::isSongFile("a.FLAC"));
    CHECK(argon::isSongFile("a.Ogg"));
    CHECK(argon::isSongFile("a.MP3"));
}

void testOtherFilesAreNotSongs()
{
    // Arrange / Act / Assert
    CHECK(!argon::isSongFile("a.wma"));
    CHECK(!argon::isSongFile("a.txt"));
    CHECK(!argon::isSongFile("mp3"));
}

void testMp3FileByExtension()
{
    // Arrange / Act / Assert
    CHECK(argon::isMp3File("song.Mp3"));
    CHECK(!argon::isMp3File("song.ogg"));
}

void testMp3DecodesFormat()
{
    // Arrange
    argon::Pcm pcm;

    // Act
    const bool decoded = argon::decodeMp3(silentMp3(20), pcm);

    // Assert
    CHECK(decoded);
    CHECK(pcm.channels == 1);
    CHECK(pcm.rate == kSilentFrameRate);
}

void testMp3DecodesSilentSamples()
{
    // Arrange
    argon::Pcm pcm;

    // Act
    argon::decodeMp3(silentMp3(20), pcm);

    // Assert
    CHECK(!pcm.samples.empty());
    CHECK(std::count(pcm.samples.begin(), pcm.samples.end(), int16_t(0)) == std::ptrdiff_t(pcm.samples.size()));
}

void testNonMp3BytesFail()
{
    // Arrange
    const std::vector<uint8_t> text(2000, 'x');
    argon::Pcm pcm;

    // Act
    const bool decoded = argon::decodeMp3(text, pcm);

    // Assert
    CHECK(!decoded);
}

void testEmptyFileFails()
{
    // Arrange
    argon::Pcm pcm;

    // Act / Assert
    CHECK(!argon::decodeMp3({}, pcm));
}

void testConvertedCandidatesPreferFlacThenOgg()
{
    // Arrange
    const std::filesystem::path wav = "audio/generic/bronze/Bronze_L.wav";

    // Act
    const std::vector<std::filesystem::path> candidates = argon::convertedSoundCandidates(wav);

    // Assert
    CHECK(candidates.size() == 2);
    CHECK(candidates[0] == std::filesystem::path("audio/generic/bronze/Bronze_L.flac"));
    CHECK(candidates[1] == std::filesystem::path("audio/generic/bronze/Bronze_L.ogg"));
}

void testConvertedCandidatesForUpperCaseWav()
{
    // Arrange / Act
    const std::vector<std::filesystem::path> candidates = argon::convertedSoundCandidates("music/On_Start.WAV");

    // Assert
    CHECK(candidates.size() == 2);
    CHECK(candidates[1] == std::filesystem::path("music/On_Start.ogg"));
}

void testConvertedCandidatesKeepDotsInName()
{
    // Arrange / Act
    const std::vector<std::filesystem::path> candidates = argon::convertedSoundCandidates("cut_ambient/Amb_1.L.wav");

    // Assert
    CHECK(candidates.size() == 2);
    CHECK(candidates[0] == std::filesystem::path("cut_ambient/Amb_1.L.flac"));
}

void testNoConvertedCandidatesForOtherFiles()
{
    // Arrange / Act / Assert
    CHECK(argon::convertedSoundCandidates("song.ogg").empty());
    CHECK(argon::convertedSoundCandidates("song.mp3").empty());
    CHECK(argon::convertedSoundCandidates("wav").empty());
}

using Paths = std::vector<std::filesystem::path>;

void testSoundNamesKeepTheWavFiles()
{
    // Arrange
    const Paths files = {"bronze/Bronze_R.wav", "bronze/Bronze_L.wav"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds == Paths({"bronze/Bronze_L.wav", "bronze/Bronze_R.wav"}));
}

void testSoundNamesOfConvertedFilesWithoutWav()
{
    // Arrange
    const Paths files = {"music/on_start.flac", "bronze/Bronze_L.ogg"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds == Paths({"bronze/Bronze_L.wav", "music/on_start.wav"}));
}

void testSoundNameListedOnceWithItsConvertedCopies()
{
    // Arrange
    const Paths files = {"x/Amb_1.L.ogg", "x/Amb_1.L.wav", "x/Amb_1.L.flac"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds == Paths({"x/Amb_1.L.wav"}));
}

void testSoundNameKeepsTheWavSpelling()
{
    // Arrange
    const Paths files = {"x/bronze_l.ogg", "x/Bronze_L.WAV"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds == Paths({"x/Bronze_L.WAV"}));
}

void testSoundNamesOfUpperCaseConvertedExtensions()
{
    // Arrange
    const Paths files = {"x/Hit.OGG"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds == Paths({"x/Hit.wav"}));
}

void testSoundNamesLeaveOutOtherFiles()
{
    // Arrange
    const Paths files = {"x/bronze.txt", "x/abi.ini", "x/song.mp3", "x/intro.ogv"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds.empty());
}

void testSoundNamesSortedAsTheWavNames()
{
    // Arrange
    const Paths files = {"x/b_R.ogg", "x/a.wav", "x/b_L.ogg"};

    // Act
    const Paths sounds = argon::gameSoundNames(files);

    // Assert
    CHECK(sounds == Paths({"x/a.wav", "x/b_L.wav", "x/b_R.wav"}));
}

void testMissingFolderListsNoSounds()
{
    // Arrange / Act / Assert
    CHECK(argon::listGameSounds("no such folder/really").empty());
}

void testLoopCommentsGiveTheRegion()
{
    // Arrange
    const std::vector<std::string> comments = {"encoder=Lavf", "LOOPSTART=1200", "LOOPLENGTH=3000"};

    // Act
    const argon::SampleLoop loop = argon::sampleLoopFromComments(comments);

    // Assert
    CHECK(loop.begin == 1200);
    CHECK(loop.end == 4200);
}

void testLoopCommentKeysInAnyCase()
{
    // Arrange
    const std::vector<std::string> comments = {"looplength=64", "LoopStart=0"};

    // Act
    const argon::SampleLoop loop = argon::sampleLoopFromComments(comments);

    // Assert
    CHECK(loop.begin == 0);
    CHECK(loop.end == 64);
}

void testNoLoopWithoutLength()
{
    // Arrange / Act
    const argon::SampleLoop loop = argon::sampleLoopFromComments({"LOOPSTART=100"});

    // Assert
    CHECK(loop.begin == 0);
    CHECK(loop.end == 0);
}

void testNoLoopForZeroLength()
{
    // Arrange / Act
    const argon::SampleLoop loop = argon::sampleLoopFromComments({"LOOPSTART=100", "LOOPLENGTH=0"});

    // Assert
    CHECK(loop.end == 0);
}

void testNoLoopForNonNumericValues()
{
    // Arrange / Act
    const argon::SampleLoop loop = argon::sampleLoopFromComments({"LOOPSTART=-5", "LOOPLENGTH=12a"});

    // Assert
    CHECK(loop.end == 0);
}

void testNoLoopForSimilarKeys()
{
    // Arrange / Act
    const argon::SampleLoop loop = argon::sampleLoopFromComments({"LOOPSTARTX=5", "LOOPLENGTHS=12"});

    // Assert
    CHECK(loop.end == 0);
}

void testPlayLengthCommentGivesPlayFrames()
{
    // Arrange
    const std::vector<std::string> comments = {"LOOPSTART=0", "PlayLength=6250"};

    // Act
    const size_t playFrames = argon::playFramesFromComments(comments, 6272);

    // Assert
    CHECK(playFrames == 6250);
}

void testPlayLengthOfTheWholeSoundIsAll()
{
    // Arrange / Act
    const size_t playFrames = argon::playFramesFromComments({"PLAYLENGTH=6272"}, 6272);

    // Assert
    CHECK(playFrames == 0);
}

void testNoPlayLengthCommentIsAll()
{
    // Arrange / Act
    const size_t playFrames = argon::playFramesFromComments({"encoder=Lavf", "PLAYLENGTH=x"}, 6272);

    // Assert
    CHECK(playFrames == 0);
}

void testNonOggBytesFail()
{
    // Arrange
    const std::vector<uint8_t> text(2000, 'x');
    argon::Pcm pcm;

    // Act / Assert
    CHECK(!argon::decodeOggVorbis(text, pcm));
}
}  // namespace

int main()
{
    testSongExtensionsAnyCase();
    testOtherFilesAreNotSongs();
    testMp3FileByExtension();
    testMp3DecodesFormat();
    testMp3DecodesSilentSamples();
    testNonMp3BytesFail();
    testEmptyFileFails();
    testConvertedCandidatesPreferFlacThenOgg();
    testConvertedCandidatesForUpperCaseWav();
    testConvertedCandidatesKeepDotsInName();
    testNoConvertedCandidatesForOtherFiles();
    testSoundNamesKeepTheWavFiles();
    testSoundNamesOfConvertedFilesWithoutWav();
    testSoundNameListedOnceWithItsConvertedCopies();
    testSoundNameKeepsTheWavSpelling();
    testSoundNamesOfUpperCaseConvertedExtensions();
    testSoundNamesLeaveOutOtherFiles();
    testSoundNamesSortedAsTheWavNames();
    testMissingFolderListsNoSounds();
    testLoopCommentsGiveTheRegion();
    testLoopCommentKeysInAnyCase();
    testNoLoopWithoutLength();
    testNoLoopForZeroLength();
    testNoLoopForNonNumericValues();
    testNoLoopForSimilarKeys();
    testPlayLengthCommentGivesPlayFrames();
    testPlayLengthOfTheWholeSoundIsAll();
    testNoPlayLengthCommentIsAll();
    testNonOggBytesFail();
    std::printf("song_files_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
