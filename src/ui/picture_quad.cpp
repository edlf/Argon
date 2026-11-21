// Argon: rotated-quad pictures (ui\picture_quad.hpp)
#include "ui/picture_quad.hpp"

#include "ui/menu_common.hpp"

namespace
{
// the 640 x 480 layout into the viewport box as drawn: the box widens from margin to margin in a wide window
// (boxLeft / boxWidth), and the picture stretches with it, as the game's whole picture is stretched to 16:9
struct BoxSpace
{
    float pixelsPerUnit, layoutLeft, left, scaleX;
    ImVec2 toScreen(glm::vec2 point) const
    {
        return ImVec2((left + (point.x - layoutLeft) * scaleX) * pixelsPerUnit, point.y * pixelsPerUnit);
    }
};

BoxSpace boxSpace(float pixelsPerUnit, float originX, const ImVec4 &box)
{
    return {pixelsPerUnit, box.x, menu::boxLeft(originX, box.x, box.z), menu::boxWidth(originX, box.x, box.z) / box.z};
}

ImVec2 toImVec(glm::vec2 v)
{
    return ImVec2(v.x, v.y);
}
}  // namespace

void drawPictureQuad(HudPainter &painter, const FeCanvas &canvas, const PictureQuad &quad)
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    if (!quad.picture)
    {
        return;
    }
    const menu::RotatedQuad rotated = menu::rotatedQuad(quad.spec);
    const BoxSpace space = boxSpace(pixelsPerUnit, originX, quad.viewport);
    const ImVec4 &box = quad.viewport;
    drawList->PushClipRect(space.toScreen({box.x, box.y}), space.toScreen({box.x + box.z, box.y + box.w}), true);
    painter.blend(drawList, argon::gfx::UiBlend::Additive);
    // corners 0, 1, 3, 2 go round the quad
    drawList->AddImageQuad(quad.picture->image,
                     space.toScreen(rotated.corners[0]),
                     space.toScreen(rotated.corners[1]),
                     space.toScreen(rotated.corners[3]),
                     space.toScreen(rotated.corners[2]),
                     toImVec(rotated.uvs[0]),
                     toImVec(rotated.uvs[1]),
                     toImVec(rotated.uvs[3]),
                     toImVec(rotated.uvs[2]),
                     IM_COL32_WHITE);
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    drawList->PopClipRect();
}
