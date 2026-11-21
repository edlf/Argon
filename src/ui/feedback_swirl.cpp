// Argon: the feedback swirl (ui\feedback_swirl.hpp)
#include "ui/feedback_swirl.hpp"

#include <cmath>

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"

void drawFeedbackSwirl(ImDrawList *drawList, ImTextureID feedback, ImVec2 screen, const FeedbackSwirl &swirl)
{
    const float scale = 1.f + swirl.zoom / 320.f, turn = swirl.turnDeg * 3.14159265f / 180.f;
    const ImVec2 c(screen.x * 0.5f, screen.y * 0.5f), h(screen.x * 0.5f, screen.y * 0.5f);
    const float dx[4] = {-h.x, h.x, h.x, -h.x}, dy[4] = {-h.y, -h.y, h.y, h.y};
    for (float sign : {1.f, -1.f})
    {
        const float cs = std::cos(turn * sign) * scale, sn = std::sin(turn * sign) * scale;
        ImVec2 corners[4];
        for (int i = 0; i < 4; ++i)
        {
            corners[i] = ImVec2(c.x + dx[i] * cs - dy[i] * sn, c.y + dx[i] * sn + dy[i] * cs);
        }
        drawList->AddImageQuad(
            feedback, corners[0], corners[1], corners[2], corners[3], ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), imColour(swirl.colour));
    }
}
