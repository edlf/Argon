// Argon: the race scene's echo effects (render\speed_blur.hpp).
#include "render/speed_blur.hpp"

#include "hud/hud_painter.hpp"
#include "render/frame_copies.hpp"
#include "ui/feedback_swirl.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

SpeedBlur::SpeedBlur(FrameCopies &copies) : copies_(copies)
{
}

float SpeedBlur::cameraSpeed(const HudCamera &camera, const BlurLook &look)
{
    return look.marsFix ? speed_.smoothed(camera, ImGui::GetIO().DeltaTime, glfwGetTime()) : speed_.measured(camera, glfwGetTime());
}

void SpeedBlur::drawOver(ImDrawList *drawList, const BlurLook &look, float speed)
{
    if (look.hyperBlur)
    {
        drawHyperBlur(drawList, speed);
    }
    else
    {
        drawZoom(drawList, look, speed);
    }
}

void SpeedBlur::drawEcho(ImDrawList *drawList)
{
    drawList->AddImage(copies_.menuFeedbackImage(), ImVec2(0, 0), ImGui::GetIO().DisplaySize);
}

// above 200 m/s (720 km/h, in practice a turbo) the echo with alpha a = clamp((s - 170) x 1.5, 0, 120), zoomed by
// a x 0.1 (0xa6f00: scale 1 + zoom / 320): the picture smears outward. The Mars tracks keep the fade without the zoom
// (docs\argon\differences.md "Speed blur on Mars"): zoomed, the echo laid a second copy of the overhead gantries a few
// pixels further out, and as they came closer the two jumped apart and back
void SpeedBlur::drawZoom(ImDrawList *drawList, const BlurLook &look, float speed)
{
    const int alpha = speed > 200.f ? std::clamp(int((speed - 170.f) * 1.5f), 0, 120) : 0;
    if (alpha <= 0)
    {
        return;
    }
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float sc = look.marsFix ? 1.f : 1.f + float(alpha) * 0.1f / 320.f;
    const ImVec2 c(ds.x * 0.5f, ds.y * 0.5f), h(ds.x * 0.5f * sc, ds.y * 0.5f * sc);
    drawList->AddImage(
        copies_.menuFeedbackImage(), ImVec2(c.x - h.x, c.y - h.y), ImVec2(c.x + h.x, c.y + h.y), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, alpha));
}

// Hyper Blur (0x93928): at any speed, a = clamp((s - 50) x 1.4, 0, 190) and alpha = (a + 20) - (a + 20) / 4 (15..158);
// the swirl 0xa7080 (1, 0.2) in that alpha: a turning, radial smear that never quite goes away
void SpeedBlur::drawHyperBlur(ImDrawList *drawList, float speed)
{
    const int strength = std::clamp(int(std::lround((speed - 50.f) * 1.4f)), 0, 190) + 20;
    const uint32_t alpha = uint32_t(strength - strength / 4);
    drawFeedbackSwirl(drawList, copies_.menuFeedbackImage(), ImGui::GetIO().DisplaySize, FeedbackSwirl{1.f, 0.2f, alpha << 24 | 0xffffffu});
}

// the echo (this frame's 3D view) drawn back through the view's 9 x 9 grid (rain object 0x90880: vertices at
// x0 + w / 8 i, y0 + h / 8 j, colour 0xff507060), each vertex's texel = its position + 8 x (sin, cos)(t + i + j): the
// view tinted green-blue and wobbling
void SpeedBlur::drawUnderwater(ImDrawList *drawList, float clock)
{
    const int kGrid = 9, kCells = kGrid - 1;
    const float kWobble = 8.f;   // 0x1a3594, texels of the 640 x 480 frame
    const ImU32 tint = imColour(0xff507060u);
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    drawList->PushTextureID(copies_.menuFeedbackImage());
    drawList->PrimReserve(kCells * kCells * 6, kGrid * kGrid);
    const unsigned base = drawList->_VtxCurrentIdx;
    for (int j = 0; j < kGrid; ++j)
    {
        for (int i = 0; i < kGrid; ++i)
        {
            const float phase = clock + float(i + j), x = float(i) / kCells, y = float(j) / kCells;
            const ImVec2 uv(x + std::sin(phase) * kWobble / 640.f, y + std::cos(phase) * kWobble / 480.f);
            drawList->PrimWriteVtx(ImVec2(x * ds.x, y * ds.y), uv, tint);
        }
    }
    for (int j = 0; j < kCells; ++j)
    {
        for (int i = 0; i < kCells; ++i)
        {
            const unsigned corner = base + unsigned(j * kGrid + i);
            for (unsigned vertex : {corner, corner + 1, corner + kGrid, corner + 1, corner + kGrid, corner + kGrid + 1})
            {
                drawList->PrimWriteIdx(ImDrawIdx(vertex));
            }
        }
    }
    drawList->PopTextureID();
}
