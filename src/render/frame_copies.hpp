// Argon: the frame copies the screens draw back (docs\frontend\README.md "Scene transition"):
// - the scene transition (0x2f2e0 / 0xa72a0): the last finished frame, kept every frame, frozen and drawn over the new
//   scene fading out over 0.25 s; started on leaving the front end for the loading screen (0x80556), on starting the
//   race (0x97486) and on coming back to the front end (0x30b86)
// - the Hud Lens source (the game's echo texture 0x50e160, 0x93aa8): the 3D view before its see-through bucket
// - the menus' and the race's feedback (0xa7080 / 0xa6eb0, the speed blur, the pause blur): the frame so far, cut out
//   of the pass from inside ImGui's draw
#pragma once
#include "gfx/presenter.hpp"

#include <imgui.h>

class Renderer;

class FrameCopies
{
  public:
    explicit FrameCopies(Renderer &renderer);
    void create();    // with the renderer's resources
    void destroy();   // before ImGui stops (the GPU idle)
    // the transition's and the lens's copies sized to the window (a resize drops a frozen frame)
    void fitToWindow();

    void startTransition();   // nothing until a frame has been kept
    void drawTransition(ImDrawList *drawList);
    void keepFinishedFrame();   // after the frame's render pass, unless a fade is showing the kept one

    void copySceneForLenses();   // the scene pass ended, copied, started again

    bool fitMenuFeedback();                 // false until the window has a size
    ImTextureID menuFeedbackImage() const;  // none until it is fitted
    void cutToMenuFeedback(ImDrawList *drawList); // at this point of the draw: the frame so far into the feedback copy

  private:
    static void cutCallback(const ImDrawList *, const ImDrawCmd *cmd);

    Renderer &renderer_;
    argon::gfx::FrameCopy transition_, lens_, menuFeedback_;
    bool transitionShowing_ = false, frameKept_ = false;
    double transitionStart_ = 0;
};
