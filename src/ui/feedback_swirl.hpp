// Argon: fe_feedback_swirl 0xa7080 (zoom, turn): the feedback texture (the last frame) drawn back twice over
// the screen, centred, scaled 1 + zoom / 320 and turned by +turn and -turn degrees. The menus' background uses it
// (15, 8) in 0xa0ffffff; the race's speed blur with Hyper Blur (1, 0.2).
#pragma once
#include <cstdint>

#include <imgui.h>

struct FeedbackSwirl
{
    float zoom = 0.f, turnDeg = 0.f;
    uint32_t colour = 0xffffffffu;   // ARGB
};

void drawFeedbackSwirl(ImDrawList *drawList, ImTextureID feedback, ImVec2 screen, const FeedbackSwirl &swirl);
