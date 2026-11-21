// Argon: a game track id's data\tracks folder (0x1b9538): 0..7 the tracks, 8..15 their second routes, 16 / 17 Mars
#pragma once
#include <string>

inline std::string courseFolder(int id)
{
    static const char *kNames[18] = {"silver",
                                     "kalau",
                                     "fuusoku",
                                     "flames",
                                     "solar",
                                     "bracken",
                                     "mount",
                                     "draco",
                                     "alt_silver",
                                     "alt_kalau",
                                     "alt_fuusoku",
                                     "alt_flames",
                                     "alt_solar",
                                     "alt_bracken",
                                     "alt_mount",
                                     "alt_draco",
                                     "mars",
                                     "alt_mars"};
    return id >= 0 && id < 18 ? kNames[id] : "";
}
