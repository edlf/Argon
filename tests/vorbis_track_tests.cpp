// Argon: tests for a converted video's soundtrack (movie\vorbis_track.hpp): the Vorbis stream's pages copied out of a
// muxed Ogg file. The files are built here with libogg. No window or sound card: run with test.bat (or ctest in build\).
#include "movie/vorbis_track.hpp"
#include "check.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include <ogg/ogg.h>

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr int kPictureSerial = 11, kSoundSerial = 22;
const std::string kTheoraHeader = std::string("\x80theora", 7);
const std::string kVorbisHeader = std::string("\x01vorbis", 7);

// one Ogg stream, each packet flushed to a page of its own (so the streams' pages can be interleaved)
class StreamWriter
{
  public:
    explicit StreamWriter(int serial)
    {
        ogg_stream_init(&stream_, serial);
    }
    ~StreamWriter()
    {
        ogg_stream_clear(&stream_);
    }
    StreamWriter(const StreamWriter &) = delete;
    StreamWriter &operator=(const StreamWriter &) = delete;

    void writePage(std::string &file, const std::string &payload, bool last)
    {
        ogg_packet packet{};
        packet.packet = reinterpret_cast<unsigned char *>(const_cast<char *>(payload.data()));
        packet.bytes = long(payload.size());
        packet.b_o_s = packetNumber_ == 0;
        packet.e_o_s = last;
        packet.granulepos = packetNumber_;
        packet.packetno = packetNumber_++;
        ogg_stream_packetin(&stream_, &packet);
        ogg_page page;
        while (ogg_stream_flush(&stream_, &page) != 0)
        {
            file.append(reinterpret_cast<char *>(page.header), size_t(page.header_len));
            file.append(reinterpret_cast<char *>(page.body), size_t(page.body_len));
        }
    }

  private:
    ogg_stream_state stream_{};
    long packetNumber_ = 0;
};

// both identification pages first (as ffmpeg muxes them), then the streams' packets interleaved
std::string muxedFile(const std::vector<std::string> &soundPackets)
{
    std::string file;
    StreamWriter picture(kPictureSerial), sound(kSoundSerial);
    picture.writePage(file, kTheoraHeader, false);
    sound.writePage(file, kVorbisHeader, false);
    for (size_t i = 0; i < soundPackets.size(); ++i)
    {
        picture.writePage(file, "frame " + std::to_string(i), false);
        sound.writePage(file, soundPackets[i], i + 1 == soundPackets.size());
    }
    picture.writePage(file, "last frame", true);
    return file;
}

std::vector<uint8_t> trackOf(const std::string &file)
{
    std::istringstream in(file);
    return argon::vorbisTrackOf(in);
}

struct PageSummary
{
    std::vector<int> serials;
    std::vector<std::string> payloads;
};

PageSummary summaryOf(const std::vector<uint8_t> &track)
{
    PageSummary summary;
    ogg_sync_state sync;
    ogg_sync_init(&sync);
    char *buffer = ogg_sync_buffer(&sync, long(track.size()));
    std::copy(track.begin(), track.end(), buffer);
    ogg_sync_wrote(&sync, long(track.size()));
    ogg_page page;
    while (ogg_sync_pageout(&sync, &page) == 1)
    {
        summary.serials.push_back(ogg_page_serialno(&page));
        summary.payloads.emplace_back(reinterpret_cast<char *>(page.body), size_t(page.body_len));
    }
    ogg_sync_clear(&sync);
    return summary;
}

void testKeepsOnlyTheSoundStreamsPages()
{
    // Arrange
    const std::string file = muxedFile({"a", "b", "c"});

    // Act
    const PageSummary summary = summaryOf(trackOf(file));

    // Assert
    CHECK(summary.serials == std::vector<int>(4, kSoundSerial));
}

void testKeepsTheSoundPagesInOrder()
{
    // Arrange
    const std::string file = muxedFile({"a", "b", "c"});

    // Act
    const PageSummary summary = summaryOf(trackOf(file));

    // Assert
    CHECK(summary.payloads == (std::vector<std::string>{kVorbisHeader, "a", "b", "c"}));
}

void testFileWithoutSoundGivesNothing()
{
    // Arrange
    std::string file;
    StreamWriter picture(kPictureSerial);
    picture.writePage(file, kTheoraHeader, false);
    picture.writePage(file, "frame", true);

    // Act / Assert
    CHECK(trackOf(file).empty());
}

void testNonOggBytesGiveNothing()
{
    // Arrange / Act / Assert
    CHECK(trackOf(std::string(5000, 'x')).empty());
}
}  // namespace

int main()
{
    testKeepsOnlyTheSoundStreamsPages();
    testKeepsTheSoundPagesInOrder();
    testFileWithoutSoundGivesNothing();
    testNonOggBytesGiveNothing();
    std::printf("vorbis_track_tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
