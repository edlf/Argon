// Reads the picture of a converted video (tools/assets/convert_assets.py: Theora in Ogg, <name>.ogv) with libogg + libtheora, as
// BGRX frames one at a time. The soundtrack muxed beside it is read by argon::vorbisTrackOf (vorbis_track.hpp).
#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

struct VideoFrame
{
    double seconds = 0;              // presentation time
    int width = 0, height = 0;
    std::vector<uint8_t> bgrx;       // top-down rows, 4 bytes a pixel
};

class TheoraVideoReader
{
  public:
    explicit TheoraVideoReader(const std::filesystem::path &file);   // throws std::runtime_error
    ~TheoraVideoReader();
    TheoraVideoReader(const TheoraVideoReader &) = delete;
    TheoraVideoReader &operator=(const TheoraVideoReader &) = delete;

    int width() const;
    int height() const;
    bool readFrame(VideoFrame &out);   // false at the end of the video

  private:
    struct Decoder;
    std::unique_ptr<Decoder> decoder_;
};
