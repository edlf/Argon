// Argon: the drawing the front end's shell asks for (ui\fe_shell_host.hpp): the page's models and boxed models, put
// in the ImGui draw list as callbacks and drawn by the frame's scene recorder, and the menu bars with the logo (also
// the race menus'). The frame recorder fills the models while the frame records (render\frame_recorder.hpp).
#pragma once
#include "render/frame_recorder.hpp"
#include "ui/fe_shell_host.hpp"

class HudPainter;
struct ImDrawCmd;

class FeShellDrawing : public FeShellHost
{
  public:
    explicit FeShellDrawing(HudPainter &painter) : painter_(painter)
    {
    }

    FrontEndModels &frameModels()   // this frame's page models, set while it records
    {
        return models_;
    }

    void drawPageModels(ImDrawList *drawList) override;
    void drawBoxModels(ImDrawList *drawList) override;
    void drawMenuBars(const FeCanvas &canvas, float vw) override;

  private:
    static void pageModelsCallback(const ImDrawList *, const ImDrawCmd *cmd);
    static void boxModelsCallback(const ImDrawList *, const ImDrawCmd *cmd);

    HudPainter &painter_;
    FrontEndModels models_;
};
