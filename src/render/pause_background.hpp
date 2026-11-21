// Argon: the pause screen's background (pause_background 0x7e5a0 -> 0x7e5e0 (0, 1.75)), which the upgrade page and the
// race end's menus share: the screen cleared, then the echo of the race frame drawn back in 5 copies offset by
// -2..2 x 1.75 px (alpha 255, 128, 64, 32, 16), three times sideways and three times up and down, the result copied back
// into the echo after each pass. The echo starts as the race frame drawn under this (the 3D view, the HUD); blurred
// once, then held: the game's picture does not drift (seen on the console).
#pragma once
#include <imgui.h>

class FrameCopies;

class PauseBackground
{
  public:
    explicit PauseBackground(FrameCopies &copies);
    void blurAgain();   // the next draw blurs this frame's race picture (the menu opened)
    void draw(ImDrawList *drawList);

  private:
    enum class Axis
    {
        Horizontal,
        Vertical
    };
    void drawPass(ImDrawList *drawList, Axis axis);

    FrameCopies &copies_;
    bool blurred_ = false;
};
