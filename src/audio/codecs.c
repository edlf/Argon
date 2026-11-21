/* Argon: the song decoders for custom music (docs\frontend\options.md "Custom music"): dr_flac (FLAC),
   dr_mp3 (MP3) and stb_vorbis (Ogg Vorbis), all public domain, downloaded into the build folder by CMake. */
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"
