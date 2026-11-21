#include "app/editor_track_load.hpp"

#include "files/string_table.hpp"
#include "ui/fe_canvas.hpp"

#include <imgui.h>

void EditorTrackLoad::start(const std::string &trackName, const TrackLoad::Use &use, TrackRequest request)
{
    trackName_ = trackName;
    load_ = std::make_unique<TrackLoad>(use, std::move(request), std::launch::async);
}

void EditorTrackLoad::step()
{
    if (load_ && load_->step(TrackLoad::kFrameSliceMs))
    {
        load_.reset();
    }
}

void EditorTrackLoad::draw(HudPainter &painter) const
{
    if (!load_)
    {
        return;
    }
    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = size.y / 480.f, originX = (size.x / pixelsPerUnit - 640.f) * 0.5f;
    const FeCanvas canvas(painter, drawList, pixelsPerUnit, originX);
    drawList->AddRectFilled(ImVec2(0, 0), size, IM_COL32(0, 0, 0, 255));
    canvas.text({originX + 320.f, 200.f}, argon::widenLatin1("Loading"), {1, 0xff85dde5u, 2, 1.f, 1.f});
    canvas.text({originX + 320.f, 240.f}, argon::widenLatin1(trackName_.c_str()), {0, 0xffffffffu, 2, 1.f, 1.f});
    const float barX = (originX + 220.f) * pixelsPerUnit, barY = 280.f * pixelsPerUnit, barW = 200.f * pixelsPerUnit, barH = 8.f * pixelsPerUnit;
    drawList->AddRect(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(133, 221, 229, 255));
    drawList->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW * load_->progress(), barY + barH), IM_COL32(133, 221, 229, 255));
}
