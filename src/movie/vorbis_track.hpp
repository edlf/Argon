// Argon: the soundtrack of a converted video (tools\assets\convert_assets.py: Theora + Vorbis muxed in one <name>.ogv). The
// Vorbis stream's pages are copied out as an Ogg file of their own, which stb_vorbis reads (it reads one stream only).
#pragma once
#include <cstdint>
#include <istream>
#include <vector>

namespace argon
{

// the pages of the first Vorbis stream, in file order; empty when the file has none
std::vector<uint8_t> vorbisTrackOf(std::istream &ogg);

}  // namespace argon
