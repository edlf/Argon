// Argon: the rain over the race scene (0x91890's draw), in the 2D pass after the echo copy (0x93b39) as the game draws
// it with one player: the streaks (lines), the lens drops (the echo through each drop's fan), then the haze. The race
// draw calls it dry too, so a lens wet by surfacing or spray shows on every track.
#pragma once
#include <imgui.h>

#include "render/scene_views.hpp"

class FrameCopies;
class HudPainter;
namespace argon::rain
{
class Rain;
}

struct RainOverlaySources
{
    const argon::rain::Rain &rain;
    const HudCamera &camera;           // the streaks are projected with this frame's first view
    const FrameCopies &frameCopies;    // the echo the lens drops show through
    const HudPainter &painter;         // raincloud.dds
};

class RainOverlay
{
  public:
    explicit RainOverlay(const RainOverlaySources &sources) : sources_(sources)
    {
    }
    void draw(ImDrawList *drawList, ImVec2 displaySize) const;

  private:
    void drawStreaks(ImDrawList *drawList, ImVec2 displaySize) const;
    void drawLensDrops(ImDrawList *drawList, ImVec2 displaySize) const;
    void drawCloud(ImDrawList *drawList, ImVec2 displaySize) const;

    RainOverlaySources sources_;
};
