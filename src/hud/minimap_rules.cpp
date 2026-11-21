// Argon: the minimap's map per track (hud\minimap_rules.hpp)
#include "hud/minimap_rules.hpp"

#include <array>

namespace argon::hud
{
namespace
{
constexpr int kFuusoku = 2, kAltFuusoku = 0xa;

// 0x49318: the case per track id 0..0xf
constexpr std::array<const char *, 16> kMapByTrack = {"silver",
                                                      "kalau",
                                                      "fuusoku",
                                                      "flames",
                                                      "solar",
                                                      "bracken",
                                                      "mount",
                                                      "draco",
                                                      "alt_silver",
                                                      "kalau",
                                                      "fuusoku",
                                                      "flames",
                                                      "solar",
                                                      "alt_bracken",
                                                      "alt_mount",
                                                      "draco"};
}  // namespace

std::string minimapName(int trackId)
{
    const auto index = static_cast<unsigned>(trackId);   // ja 0x47362: an unsigned compare, so -1 is past 0xf too
    return index < kMapByTrack.size() ? kMapByTrack[index] : "mars";
}

float minimapWorldPerTexel(int trackId)
{
    return trackId == kFuusoku || trackId == kAltFuusoku ? 11.88f : 9.9f;
}

}  // namespace argon::hud
