// Argon: where a view goes on the window (scene_views.hpp).
#include "render/scene_views.hpp"

#include <algorithm>
#include <cmath>

namespace
{
const float kLayoutWidth = 640.f, kLayoutHeight = 480.f;
}

float windowAspect(const argon::gfx::WindowSize &window)
{
    return float(window.width) / float(std::max(1u, window.height));
}

float viewAspect(const argon::gfx::WindowSize &window, const argon::split::ScreenRect &area)
{
    return windowAspect(window) * (area.w / kLayoutWidth) / (area.h / kLayoutHeight);
}

argon::gfx::Rect screenArea(const argon::gfx::WindowSize &window, const argon::split::ScreenRect &area)
{
    const float sx = float(window.width) / kLayoutWidth, sy = float(window.height) / kLayoutHeight;
    const int32_t x = int32_t(std::lround(area.x * sx)), y = int32_t(std::lround(area.y * sy));
    const uint32_t w = uint32_t(std::lround((area.x + area.w) * sx)) - uint32_t(x);
    const uint32_t h = uint32_t(std::lround((area.y + area.h) * sy)) - uint32_t(y);
    return argon::gfx::Rect{x, y, w, h};
}
