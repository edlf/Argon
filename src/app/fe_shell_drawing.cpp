#include "app/fe_shell_drawing.hpp"

#include <algorithm>

#include "render/scene_recorder.hpp"
#include "ui/fe_canvas.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

void FeShellDrawing::drawPageModels(ImDrawList *drawList)
{
    drawList->AddCallback(pageModelsCallback, this);
}

void FeShellDrawing::drawBoxModels(ImDrawList *drawList)
{
    drawList->AddCallback(boxModelsCallback, this);
}

// the page's models, drawn from an ImGui callback with the shell camera
void FeShellDrawing::pageModelsCallback(const ImDrawList *, const ImDrawCmd *cmd)
{
    const FrontEndModels &models = static_cast<FeShellDrawing *>(cmd->UserCallbackData)->models_;
    if (models.recorder)
    {
        models.recorder->drawFrontEndModels(models.scene, models.aspect);
    }
}

// the page's boxed models (FeScene::boxed), drawn where the page puts this callback: over its frames
void FeShellDrawing::boxModelsCallback(const ImDrawList *, const ImDrawCmd *cmd)
{
    const FrontEndModels &models = static_cast<FeShellDrawing *>(cmd->UserCallbackData)->models_;
    if (!models.recorder)
    {
        return;
    }
    for (const FeScene &box : models.boxes)
    {
        models.recorder->drawFrontEndModels(box, box.box.w / std::max(1.f, box.box.h));
    }
}

// 0xc2870 (neon_draw_menu_bars): top / bottom bars, logo at (62, 2) 256 x 128. Each bar's fill: v0..v2 its colour
// argument, v3..v5 EAX = the other bar's colour (0xc2916 / 0xc2947): the top bar is faint blue along its top and
// strong orange along its bottom edge, the bottom bar the other way round. Also drawn by the race's menus (0x7e5e0).
void FeShellDrawing::drawMenuBars(const FeCanvas &canvas, float vw)
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    canvas.panel({originX - 400.f - (vw - 640.f), -10.f, 1400.f + 2.f * (vw - 640.f), 84.f}, {false, false, 0xffffff00u, true, 0x304080c0u, 0xa0ffa030u, 0.4f});
    canvas.panel({originX - 400.f - (vw - 640.f), 391.f, 1400.f + 2.f * (vw - 640.f), 84.f}, {false, false, 0xffffff00u, true, 0xa0ffa030u, 0x304080c0u, 0.4f});
    if (const Found<const HudTexture> logo = painter_.texture("qrlogo"))
    {
        drawList->AddImage(logo->image, ImVec2((atLeft(originX, 62.f)) * pixelsPerUnit, 2.f * pixelsPerUnit), ImVec2((atLeft(originX, 62.f) + 256.f) * pixelsPerUnit, 130.f * pixelsPerUnit));
    }
}
