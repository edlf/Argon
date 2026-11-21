// Argon: the rain over the race scene (hud\rain_overlay.hpp; 0x91890's draw, the haze 0x92270).
#include "hud/rain_overlay.hpp"

#include <glm/glm.hpp>

#include "hud/hud_painter.hpp"
#include "logic/rain.hpp"
#include "render/frame_copies.hpp"

namespace
{
const ImVec2 kGameView(640.f, 480.f);

ImVec2 toDisplay(glm::vec2 gamePixels, ImVec2 displaySize)
{
    return ImVec2(gamePixels.x * displaySize.x / kGameView.x, gamePixels.y * displaySize.y / kGameView.y);
}
}  // namespace

void RainOverlay::draw(ImDrawList *drawList, ImVec2 displaySize) const
{
    drawStreaks(drawList, displaySize);
    drawLensDrops(drawList, displaySize);
    drawCloud(drawList, displaySize);
}

// grey 0x808080 lines, alpha 35 at the head and 0 at the tail, one game pixel wide
void RainOverlay::drawStreaks(ImDrawList *drawList, ImVec2 displaySize) const
{
    const uint8_t alpha = sources_.rain.streakAlpha();
    if (alpha == 0)
    {
        return;
    }
    const float kNearW = 0.1f, width = displaySize.y / kGameView.y;
    const ImU32 head = IM_COL32(0x80, 0x80, 0x80, alpha), tail = IM_COL32(0x80, 0x80, 0x80, 0);
    const ImVec2 white = ImGui::GetFontTexUvWhitePixel();
    const glm::mat4 &viewProj = sources_.camera.viewProj;
    for (const argon::rain::Streak &streak : sources_.rain.streaks())
    {
        const glm::vec4 headClip = viewProj * glm::vec4(streak.head, 1.f), tailClip = viewProj * glm::vec4(streak.tail, 1.f);
        if (headClip.w <= kNearW || tailClip.w <= kNearW)
        {
            continue;
        }
        const ImVec2 pa((headClip.x / headClip.w * 0.5f + 0.5f) * displaySize.x, (headClip.y / headClip.w * 0.5f + 0.5f) * displaySize.y);
        const ImVec2 pb((tailClip.x / tailClip.w * 0.5f + 0.5f) * displaySize.x, (tailClip.y / tailClip.w * 0.5f + 0.5f) * displaySize.y);
        const glm::vec2 along(pb.x - pa.x, pb.y - pa.y);
        const float len = glm::length(along);
        if (len < 1e-3f)
        {
            continue;
        }
        const ImVec2 n(-along.y / len * width * 0.5f, along.x / len * width * 0.5f);
        drawList->PrimReserve(6, 4);
        const ImDrawIdx base = ImDrawIdx(drawList->_VtxCurrentIdx);
        drawList->PrimWriteVtx(ImVec2(pa.x + n.x, pa.y + n.y), white, head);
        drawList->PrimWriteVtx(ImVec2(pa.x - n.x, pa.y - n.y), white, head);
        drawList->PrimWriteVtx(ImVec2(pb.x - n.x, pb.y - n.y), white, tail);
        drawList->PrimWriteVtx(ImVec2(pb.x + n.x, pb.y + n.y), white, tail);
        for (int corner : {0, 1, 2, 0, 2, 3})
        {
            drawList->PrimWriteIdx(ImDrawIdx(base + corner));
        }
    }
}

// each drop a fan of 8 triangles over this frame's 3D view (the echo 0x50e160, clamped), white: opaque little lenses
void RainOverlay::drawLensDrops(ImDrawList *drawList, ImVec2 displaySize) const
{
    const argon::rain::LensDrops &drops = sources_.rain.lensDrops();
    drawList->PushTextureID(sources_.frameCopies.menuFeedbackImage());
    for (const argon::rain::LensDrop &drop : drops.drops())
    {
        if (drop.size <= 0.f)
        {
            continue;
        }
        const argon::rain::DropFan fan = drops.fan(drop);
        drawList->PrimReserve(8 * 3, int(fan.size()));
        const ImDrawIdx centre = ImDrawIdx(drawList->_VtxCurrentIdx);
        for (const argon::rain::DropVertex &vertex : fan)
        {
            const glm::vec2 uv = glm::clamp(vertex.texel / glm::vec2(kGameView.x, kGameView.y), glm::vec2(0.f), glm::vec2(1.f));
            drawList->PrimWriteVtx(toDisplay(vertex.pos, displaySize), ImVec2(uv.x, uv.y), IM_COL32_WHITE);
        }
        for (int k = 0; k < 8; ++k)
        {
            drawList->PrimWriteIdx(centre);
            drawList->PrimWriteIdx(ImDrawIdx(centre + 1 + k));
            drawList->PrimWriteIdx(ImDrawIdx(centre + 1 + (k + 1) % 8));
        }
    }
    drawList->PopTextureID();
}

// raincloud.dds over the view (0x92270: a quarter of the texture, scrolling), grey with the haze's alpha
void RainOverlay::drawCloud(ImDrawList *drawList, ImVec2 displaySize) const
{
    const Found<const HudTexture> cloud = sources_.painter.texture("raincloud");
    if (!sources_.rain.cloudShown() || !cloud)
    {
        return;
    }
    const glm::vec4 cloudTexels = sources_.rain.cloudTexels();
    drawList->AddImage(cloud->image, ImVec2(0, 0), displaySize, ImVec2(cloudTexels.x, cloudTexels.y), ImVec2(cloudTexels.z, cloudTexels.w), imColour(sources_.rain.cloudColour()));
}
