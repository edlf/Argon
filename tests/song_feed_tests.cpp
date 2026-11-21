// Argon: tests for a song decoded a few seconds ahead of where it plays (audio\song_feed.hpp): what goes in comes out
// whole and in order, through the ring's wrap, converted to the music's format. Run with test.bat / test.sh.
#include "audio/song_feed.hpp"
#include "check.hpp"

#include <cstdio>
#include <memory>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

// frame f holds (f mod 30000) in each channel
std::shared_ptr<argon::Pcm> rampSong(size_t frames, int channels, int rate)
{
    auto pcm = std::make_shared<argon::Pcm>();
    pcm->channels = channels;
    pcm->rate = rate;
    pcm->samples.resize(frames * size_t(channels));
    for (size_t f = 0; f < frames; ++f)
    {
        for (int c = 0; c < channels; ++c)
        {
            pcm->samples[f * size_t(channels) + size_t(c)] = int16_t(f % 30000);
        }
    }
    return pcm;
}

argon::SongFeed feedOf(std::shared_ptr<argon::Pcm> song)
{
    return argon::SongFeed(std::make_unique<argon::PcmFrames>(std::move(song)));
}

// the whole song as the audio thread would take it, filled between reads of `block` frames
std::vector<int16_t> drain(argon::SongFeed &feed, size_t block)
{
    std::vector<int16_t> out, piece(block * argon::SongFeed::kChannels);
    for (int guard = 0; guard < 100000 && !feed.finished(); ++guard)
    {
        feed.fill();
        const size_t n = feed.read(piece.data(), block);
        out.insert(out.end(), piece.begin(), piece.begin() + std::ptrdiff_t(n * argon::SongFeed::kChannels));
    }
    return out;
}

bool isRamp(const std::vector<int16_t> &stereo, size_t frames)
{
    if (stereo.size() != frames * 2)
    {
        return false;
    }
    for (size_t f = 0; f < frames; ++f)
    {
        if (stereo[f * 2] != int16_t(f % 30000) || stereo[f * 2 + 1] != int16_t(f % 30000))
        {
            return false;
        }
    }
    return true;
}

void songInTheMusicFormatComesOutAsItIs()
{
    argon::SongFeed feed = feedOf(rampSong(50000, 2, 44100));
    CHECK(feed.ok() && isRamp(drain(feed, 1000), 50000));
}

// longer than the ring (~11.9 s), read in odd blocks: every frame once, in order, across the wrap
void songLongerThanTheRingComesOutWhole()
{
    argon::SongFeed feed = feedOf(rampSong(1300000, 2, 44100));
    CHECK(isRamp(drain(feed, 4567), 1300000));
}

void playFramesCutsTheSong()
{
    std::shared_ptr<argon::Pcm> song = rampSong(10000, 2, 44100);
    song->playFrames = 6000;
    argon::SongFeed feed = feedOf(song);
    CHECK(drain(feed, 1000).size() == 6000 * 2);
}

void monoHalfRateSongIsConvertedToTwiceItsLength()
{
    argon::SongFeed feed = feedOf(rampSong(22050, 1, 22050));
    CHECK(feed.ok() && drain(feed, 1000).size() == 44100 * 2);
}

void feedIsNotFinishedBeforeItIsRead()
{
    argon::SongFeed feed = feedOf(rampSong(1000, 2, 44100));
    feed.fill();
    CHECK(!feed.finished());
}

void emptyFeedReadsNothing()
{
    argon::SongFeed feed = feedOf(rampSong(1000, 2, 44100));
    int16_t out[64];
    CHECK(feed.read(out, 32) == 0);
}

}  // namespace

int main()
{
    songInTheMusicFormatComesOutAsItIs();
    songLongerThanTheRingComesOutWhole();
    playFramesCutsTheSong();
    monoHalfRateSongIsConvertedToTwiceItsLength();
    feedIsNotFinishedBeforeItIsRead();
    emptyFeedReadsNothing();
    std::printf("song_feed_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
