// Argon: the player's ship views cycled by the Black button (pad bit 8, on its press: player_pad_input 0x71033). The
// game steps its tripod +0xe24 0 -> 1 -> 2 -> 3 -> 0 (close, mid, far, cockpit; jump table 0x71474). The viewer's views
// (DriveCamera, logic\drive_camera.hpp) are 0 cockpit, 1 close, 2 mid, 3 far, so the same order is 1 -> 2 -> 3 -> 0 -> 1.
#pragma once

namespace argon::views
{

constexpr int kShipViewCount = 4;   // the viewer's views 0..3 are the game's tripods; the others are not cycled

constexpr bool isShipView(int view)
{
    return view >= 0 && view < kShipViewCount;
}

constexpr int nextShipView(int view)
{
    return (view + 1) % kShipViewCount;
}

}  // namespace argon::views
