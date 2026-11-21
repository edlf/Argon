// Argon: the soundtrack of a converted video, see vorbis_track.hpp.
#include "movie/vorbis_track.hpp"

#include <cstring>

#include <ogg/ogg.h>

namespace argon
{
namespace
{
const long kReadChunkBytes = 64 * 1024;

class OggPageReader
{
  public:
    explicit OggPageReader(std::istream &in) : in_(in)
    {
        ogg_sync_init(&sync_);
    }
    ~OggPageReader()
    {
        ogg_sync_clear(&sync_);
    }
    OggPageReader(const OggPageReader &) = delete;
    OggPageReader &operator=(const OggPageReader &) = delete;

    bool next(ogg_page &page)
    {
        while (ogg_sync_pageout(&sync_, &page) != 1)
        {
            char *buffer = ogg_sync_buffer(&sync_, kReadChunkBytes);
            in_.read(buffer, kReadChunkBytes);
            const std::streamsize got = in_.gcount();
            if (got <= 0)
            {
                return false;
            }
            ogg_sync_wrote(&sync_, long(got));
        }
        return true;
    }

  private:
    std::istream &in_;
    ogg_sync_state sync_{};
};

// a stream's first page holds its identification header alone: 0x01 "vorbis" for Vorbis
bool startsVorbisStream(ogg_page &page)
{
    return ogg_page_bos(&page) && page.body_len >= 7 && page.body[0] == 1 && std::memcmp(page.body + 1, "vorbis", 6) == 0;
}

void appendPage(std::vector<uint8_t> &out, const ogg_page &page)
{
    out.insert(out.end(), page.header, page.header + page.header_len);
    out.insert(out.end(), page.body, page.body + page.body_len);
}
}  // namespace

std::vector<uint8_t> vorbisTrackOf(std::istream &ogg)
{
    OggPageReader pages(ogg);
    std::vector<uint8_t> track;
    bool found = false;
    int serial = 0;
    ogg_page page;
    while (pages.next(page))
    {
        if (!found && startsVorbisStream(page))
        {
            found = true;
            serial = ogg_page_serialno(&page);
        }
        if (found && ogg_page_serialno(&page) == serial)
        {
            appendPage(track, page);
        }
    }
    return track;
}

}  // namespace argon
