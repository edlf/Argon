// Argon: the pause screen's background (render\pause_background.hpp).
#include "render/pause_background.hpp"

#include "render/frame_copies.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kPasses = 6;           // three sideways, then three up and down
constexpr float kCopyOffset = 1.75f; // px of the 640 x 480 frame between the copies
}  // namespace

PauseBackground::PauseBackground(FrameCopies &copies) : copies_(copies)
{
}

void PauseBackground::blurAgain()
{
    blurred_ = false;
}

void PauseBackground::draw(ImDrawList *drawList)
{
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (!copies_.fitMenuFeedback() || !copies_.menuFeedbackImage())
    {
        drawList->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 160));
        return;
    }
    if (!blurred_)
    {
        copies_.cutToMenuFeedback(drawList);
        for (int i = 0; i < kPasses; ++i)
        {
            drawPass(drawList, i >= kPasses / 2 ? Axis::Vertical : Axis::Horizontal);
            copies_.cutToMenuFeedback(drawList);
        }
        blurred_ = true;
    }
    drawList->AddImage(copies_.menuFeedbackImage(), ImVec2(0, 0), ds);
}

void PauseBackground::drawPass(ImDrawList *drawList, Axis axis)
{
    const bool vertical = axis == Axis::Vertical;
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = ds.y / 480.f;
    drawList->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 255));
    float weight = 1.f;
    for (float o = -2.f; o <= 2.f; o += 1.f)
    {
        const int alpha = std::clamp(int(std::lround(weight * 255.f)), 0, 255);
        const float shift = -o * kCopyOffset * pixelsPerUnit;
        const ImVec2 p0 = vertical ? ImVec2(0, shift) : ImVec2(shift, 0), p1 = vertical ? ImVec2(ds.x, ds.y + shift) : ImVec2(ds.x + shift, ds.y);
        drawList->AddImage(copies_.menuFeedbackImage(), p0, p1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, alpha));
        weight *= 0.5f;
    }
}
