// Argon: the monitor fullscreen switches (F11, window_mode.hpp): the monitor it overlaps most.
#pragma once
#include <algorithm>
#include <vector>

namespace argon
{

struct ScreenRect
{
    int x = 0, y = 0, width = 0, height = 0;
};

inline long long overlapArea(const ScreenRect &first, const ScreenRect &second)
{
    const long long overlapWidth = std::min(first.x + first.width, second.x + second.width) - std::max(first.x, second.x);
    const long long overlapHeight = std::min(first.y + first.height, second.y + second.height) - std::max(first.y, second.y);
    return overlapWidth > 0 && overlapHeight > 0 ? overlapWidth * overlapHeight : 0;
}

// the index of the monitor the window overlaps most; the first one (GLFW's primary) when it overlaps none
inline size_t monitorUnderWindow(const ScreenRect &window, const std::vector<ScreenRect> &monitors)
{
    size_t best = 0;
    long long bestArea = 0;
    for (size_t i = 0; i < monitors.size(); ++i)
    {
        const long long area = overlapArea(window, monitors[i]);
        if (area > bestArea)
        {
            best = i;
            bestArea = area;
        }
    }
    return best;
}

}  // namespace argon
