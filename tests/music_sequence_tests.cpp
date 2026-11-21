// Argon: tests for the music played back to back (audio/music_sequence.hpp, audio/song_stream.hpp, the custom music's
// RandomPickOrder in audio/play_order.hpp), mixed
// without a sound card, at the songs' own rate (exact samples) and at 48 kHz (resampled): two songs of constant level,
// so a gap shows as zeros and an overlap as their sum. Run with test.bat / test.sh (or ctest in build).
#include "audio/audio_output.hpp"
#include "check.hpp"
#include "audio/music_sequence.hpp"

#include <cmath>
#include <cstdio>
#include <future>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
using argon::test::checks;
using argon::test::failures;

const int kRate = 44100;
const int kChunk = 441;            // frames mixed per update, as a 100 Hz frame loop would
const size_t kSongA = 44100;       // 1 s at +0.5
const size_t kSongB = 22050;       // 0.5 s at -0.25
const float kLevelA = 0.5f, kLevelB = -0.25f;

std::shared_ptr<argon::Pcm> constantSong(size_t frames, float level)
{
    auto pcm = std::make_shared<argon::Pcm>();
    pcm->channels = 2;
    pcm->rate = kRate;
    pcm->samples.assign(frames * 2, int16_t(level * 32768.f));
    return pcm;
}

std::shared_ptr<argon::SongFeed> readTestSong(const fs::path &file)
{
    std::shared_ptr<argon::Pcm> song = file == "a" ? constantSong(kSongA, kLevelA) : constantSong(kSongB, kLevelB);
    return std::make_shared<argon::SongFeed>(std::make_unique<argon::PcmFrames>(std::move(song)));
}

// the left channel of the mix while the sequence is updated once per chunk (the worker gets a moment to decode)
class Mixdown
{
  public:
    explicit Mixdown(argon::MusicSequence &sequence, argon::AudioOutput &output) : sequence_(sequence), output_(output)
    {
    }

    void run(int chunks)
    {
        std::vector<float> stereo(size_t(kChunk) * 2);
        for (int i = 0; i < chunks; ++i)
        {
            sequence_.update();
            output_.renderFrames(stereo.data(), kChunk);
            for (int f = 0; f < kChunk; ++f)
            {
                left.push_back(stereo[size_t(f) * 2]);
            }
        }
    }

    std::vector<float> left;

  private:
    argon::MusicSequence &sequence_;
    argon::AudioOutput &output_;
};

bool near(float value, float expected)
{
    return std::abs(value - expected) < 1e-3f;
}

size_t firstSound(const std::vector<float> &samples, size_t from = 0)
{
    for (size_t i = from; i < samples.size(); ++i)
    {
        if (std::abs(samples[i]) > 0.01f)
        {
            return i;
        }
    }
    return samples.size();
}

size_t firstNear(const std::vector<float> &samples, float level, size_t from)
{
    for (size_t i = from; i < samples.size(); ++i)
    {
        if (near(samples[i], level))
        {
            return i;
        }
    }
    return samples.size();
}

size_t countNear(const std::vector<float> &samples, float level, size_t from, size_t to)
{
    size_t n = 0;
    for (size_t i = from; i < to && i < samples.size(); ++i)
    {
        n += near(samples[i], level) ? 1 : 0;
    }
    return n;
}

struct Fixture
{
    argon::AudioOutput output;
    argon::MusicSequence sequence{output, readTestSong, std::launch::deferred};
    Mixdown mix{sequence, output};

    explicit Fixture(int mixRate = kRate)
    {
        output.openWithoutDevice(mixRate, argon::mix::SpeakerLayout::Stereo);
        sequence.setPlaylist({"a", "b"});
        sequence.start();
    }
};

void testSongsFollowWithoutGap()
{
    Fixture f;
    f.mix.run(300);
    const size_t start = firstSound(f.mix.left);
    CHECK(countNear(f.mix.left, kLevelA, start, start + kSongA) == kSongA);
}

// mixed at 48 kHz (as a sound card may run): resampled, still no silent sample from the start through the junction
void testResampledSongsFollowWithoutGap()
{
    Fixture f(48000);
    f.mix.run(300);
    const size_t start = firstSound(f.mix.left);
    const size_t junction = firstNear(f.mix.left, kLevelB, start);
    CHECK(firstSound(f.mix.left, start) == start && junction < f.mix.left.size() && countNear(f.mix.left, 0.f, start, junction + 1000) == 0);
}

void testResampledSongKeepsItsLength()
{
    Fixture f(48000);
    f.mix.run(300);
    const size_t start = firstSound(f.mix.left);
    const size_t played = firstNear(f.mix.left, kLevelB, start) - start;
    CHECK(played + 2 >= 48000 && played <= 48000 + 2);
}

void testNextSongStartsOnTheFrameAfter()
{
    Fixture f;
    f.mix.run(300);
    const size_t start = firstSound(f.mix.left);
    CHECK(near(f.mix.left[start + kSongA - 1], kLevelA) && near(f.mix.left[start + kSongA], kLevelB));
}

void testPlaylistLoopsWithoutGap()
{
    Fixture f;
    f.mix.run(300);
    const size_t loop = firstSound(f.mix.left) + kSongA + kSongB;
    CHECK(near(f.mix.left[loop - 1], kLevelB) && near(f.mix.left[loop], kLevelA));
}

void testPauseKeepsTheJunction()
{
    Fixture f;
    f.mix.run(50);
    f.sequence.pause();
    f.mix.run(20);
    f.sequence.resume();
    f.mix.run(300);
    const size_t start = firstSound(f.mix.left);
    const size_t junction = firstNear(f.mix.left, kLevelB, start);
    const bool wholeSongBefore = countNear(f.mix.left, kLevelA, start, junction) == kSongA;
    CHECK(wholeSongBefore && near(f.mix.left[junction - 1], kLevelA));
}

// plays song 0 first, then whatever `choice` says
class ChosenOrder : public argon::PlayOrder
{
  public:
    void restart(size_t) override
    {
        choice = 0;
    }
    size_t next() const override
    {
        return choice;
    }
    void started(size_t) override
    {
    }
    size_t choice = 0;
};

void testChangedChoiceReplacesTheQueuedSong()
{
    argon::AudioOutput output;
    output.openWithoutDevice(kRate, argon::mix::SpeakerLayout::Stereo);
    argon::MusicSequence sequence{output, readTestSong, std::launch::deferred};
    ChosenOrder order;
    sequence.setPlaylist({"a", "b"}, order);
    sequence.start();
    Mixdown mix{sequence, output};
    mix.run(10);
    order.choice = 1;
    mix.run(10);   // "b" queued after "a"
    order.choice = 0;
    mix.run(200);
    const size_t start = firstSound(mix.left);
    CHECK(countNear(mix.left, kLevelA, start, start + 2 * kSongA) == 2 * kSongA);
}

void testNowPlayingFollowsTheSongs()
{
    Fixture f;
    f.mix.run(50);
    const std::string first = f.sequence.nowPlaying();
    f.mix.run(100);
    CHECK(first == "a" && f.sequence.nowPlaying() == "b");
}
// the C runtime's rand() from seed 1: 41, 18467, 6334, 26500, ...
void testRandomPickStartsOnRandModCount()
{
    argon::RandomPickOrder order{1};
    order.restart(4);
    CHECK(order.next() == 41 % 4);
}

void testRandomPickDrawsAgainAfterEachSong()
{
    argon::RandomPickOrder order{1};
    order.restart(4);
    order.started(order.next());
    CHECK(order.next() == 18467 % 4);
}

void testRandomPickCanRepeatASong()
{
    argon::RandomPickOrder order{1};
    order.restart(1);
    order.started(0);
    CHECK(order.next() == 0);
}
}  // namespace

int main()
{
    testRandomPickStartsOnRandModCount();
    testRandomPickDrawsAgainAfterEachSong();
    testRandomPickCanRepeatASong();
    testSongsFollowWithoutGap();
    testResampledSongsFollowWithoutGap();
    testResampledSongKeepsItsLength();
    testNextSongStartsOnTheFrameAfter();
    testPlaylistLoopsWithoutGap();
    testPauseKeepsTheJunction();
    testNowPlayingFollowsTheSongs();
    testChangedChoiceReplacesTheQueuedSong();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
