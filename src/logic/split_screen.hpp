// Argon: the split screen's views (neon_initialize_rain 0x7efd0 sets one per player, in the game's 640 x 480):
// one player the whole screen, two the top and bottom halves, three and four the quarters in reading order (three
// leave the bottom right empty).
#pragma once

namespace argon::split
{

constexpr int kMaxPlayers = 4;

struct ScreenRect
{
    float x = 0, y = 0, w = 640, h = 480;
};

constexpr ScreenRect viewOf(int players, int index)
{
    if (players <= 1)
    {
        return {};
    }
    if (players == 2)
    {
        return {0.f, 240.f * float(index), 640.f, 240.f};
    }
    return {320.f * float(index % 2), 240.f * float(index / 2), 320.f, 240.f};
}

// gamehud_layout 0x45270: a view's HUD frame, 52 (x) / 36 (y) in from the screen's edges and 12 from the other views';
// view in a screen screenWidth wide (the game's 640, wider in a wide window) and 480 high
constexpr ScreenRect hudFrame(const ScreenRect &view, float screenWidth = 640.f)
{
    const float left = view.x < 0.5f ? 52.f : 12.f, right = view.x + view.w > screenWidth - 0.5f ? 52.f : 12.f;
    const float top = view.y < 0.5f ? 36.f : 12.f, bottom = view.y + view.h > 479.5f ? 36.f : 12.f;
    return {view.x + left, view.y + top, view.w - left - right, view.h - top - bottom};
}

}  // namespace argon::split
