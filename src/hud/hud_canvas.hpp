// Argon: one frame's HUD target, the game's 640 x 480 virtual screen (widened for other aspects): the text
// renderer (font_draw_text 0xb1660), the data\hud textures, panels and blend modes, all through the HUD painter
// (hud\hud_painter.hpp).
#pragma once
#include <cstdint>
#include <string>

#include "hud/hud_painter.hpp"

// a textured rectangle in 640 x 480 units and the part of the texture it shows
struct HudQuad
{
    float x0, y0, x1, y1;
    ImVec2 uv0{0, 0}, uv1{1, 1};
};

class HudCanvas
{
  public:
    HudCanvas(HudPainter &painter, ImDrawList *drawList, float pixelsPerUnit) : painter_(painter), drawList_(drawList), pixelsPerUnit_(pixelsPerUnit)
    {
    }

    ImVec2 at(float x, float y) const
    {
        return ImVec2(x * pixelsPerUnit_, y * pixelsPerUnit_);
    }
    ImDrawList *drawList() const
    {
        return drawList_;
    }
    float pixelsPerUnit() const
    {
        return pixelsPerUnit_;
    }

    void text(ImVec2 at, const std::u16string &content, const TextStyle &style) const
    {
        painter_.text(drawList_, pixelsPerUnit_, at, content, style);
    }
    Found<const HudTexture> texture(const std::string &name) const
    {
        return painter_.texture(name);
    }   // none before it loads
    void image(const HudTexture &texture, float x0, float y0, float x1, float y1) const
    {
        drawList_->AddImage(texture.image, at(x0, y0), at(x1, y1));
    }
    void image(const HudTexture &texture, const HudQuad &quad, uint32_t argbColour) const
    {
        drawList_->AddImage(texture.image, at(quad.x0, quad.y0), at(quad.x1, quad.y1), quad.uv0, quad.uv1, imColour(argbColour));
    }
    void rect(float x0, float y0, float x1, float y1, uint32_t argbColour) const
    {   // 0xaad40: filled rectangle
        drawList_->AddRectFilled(at(std::min(x0, x1), std::min(y0, y1)), at(std::max(x0, x1), std::max(y0, y1)), imColour(argbColour));
    }
    void panel(const PanelBox &box, const PanelStyle &style) const
    {
        painter_.panel(drawList_, pixelsPerUnit_, box, style);
    }
    void additiveBlend() const
    {
        painter_.blend(drawList_, argon::gfx::UiBlend::Additive);
    }   // blend mode 2
    void normalBlend() const
    {
        drawList_->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }

  private:
    HudPainter &painter_;
    ImDrawList *drawList_;
    float pixelsPerUnit_;
};
