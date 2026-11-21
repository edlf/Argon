#include "ui_extras/fps.hpp"
#include <algorithm>
#include <cstdio>
#include <imgui.h>

#include "platform/platform.hpp"

namespace
{
const float kWidth = 230.f, kHeight = 74.f;
const float kTargetMs = 16.67f, kGraphFloorMs = 33.3f;

ImU32 frameColour(float ms)
{
    if (ms <= 16.8f)
    {
        return IM_COL32(90, 220, 90, 230);
    }
    return ms <= 33.4f ? IM_COL32(230, 210, 70, 230) : IM_COL32(230, 80, 70, 240);
}
} // namespace

void FpsMeter::recordFrame(float seconds)
{
    frameMs_[next_] = seconds * 1000.f;
    next_ = (next_ + 1) % kHistory;
}

float FpsMeter::averageMs() const
{
    float sum = 0.f;
    int count = 0;
    for (float frameMs : frameMs_)
    {
        if (frameMs > 0.f)
        {
            sum += frameMs;
            ++count;
        }
    }
    return count ? sum / float(count) : 0.f;
}

float FpsMeter::worstMs() const
{
    return *std::max_element(std::begin(frameMs_), std::end(frameMs_));
}

// the average FPS and frame time over the last 120 frames, the worst frame, the process's memory and a line graph of
// the frame times (green under 16.7 ms, yellow under 33.3 ms, red above)
void FpsMeter::draw(const argon::gfx::Device &device)
{
    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    const float x0 = ImGui::GetIO().DisplaySize.x - kWidth - 6.f, y0 = 6.f;
    const float avg = averageMs(), worst = std::max(0.f, worstMs());
    drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + kWidth, y0 + kHeight), IM_COL32(0, 0, 0, 160), 3.f);
    char text[64];
    std::snprintf(text, sizeof text, "%.0f fps  %.2f ms  (max %.1f)", avg > 0.f ? 1000.f / avg : 0.f, avg, worst);
    drawList->AddText(ImVec2(x0 + 5.f, y0 + 3.f), IM_COL32(255, 255, 255, 230), text);
    sampleMemoryUsage(device);
    drawList->AddText(ImVec2(x0 + 5.f, y0 + 19.f), IM_COL32(200, 200, 200, 230), memoryText_);
    drawGraph(x0, y0, worst);
}

// 0 .. max(33.3 ms, the worst frame), oldest on the left; one segment per pair of frames, coloured by the newer one
void FpsMeter::drawGraph(float x0, float y0, float worst) const
{
    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    const float gx = x0 + 5.f, gy = y0 + 36.f, gw = kWidth - 10.f, gh = kHeight - 41.f;
    const float top = std::max(kGraphFloorMs, worst), step = gw / float(kHistory - 1);
    const float targetY = gy + gh - kTargetMs / top * gh;
    drawList->AddLine(ImVec2(gx, targetY), ImVec2(gx + gw, targetY), IM_COL32(255, 255, 255, 50));
    bool have = false;
    ImVec2 prev;
    for (int i = 0; i < kHistory; ++i)
    {
        const float frameMs = frameMs_[(next_ + i) % kHistory];
        have = have && frameMs > 0.f;
        const ImVec2 p(gx + step * float(i), gy + gh - std::min(frameMs, top) / top * gh);
        if (have)
        {
            drawList->AddLine(prev, p, frameColour(frameMs), 1.5f);
        }
        prev = p;
        have = frameMs > 0.f;
    }
}

void FpsMeter::sampleMemoryUsage(const argon::gfx::Device &device)
{
    const double now = ImGui::GetTime();
    if (memorySampledAt_ >= 0.0 && now - memorySampledAt_ < 0.5)
    {
        return;
    }
    memorySampledAt_ = now;
    const double mib = 1024.0 * 1024.0, ram = double(platform::processPrivateWorkingSet()) / mib;
    if (const std::optional<uint64_t> video = device.videoMemoryInUse())
    {
        std::snprintf(memoryText_, sizeof memoryText_, "RAM %.0f MB  VRAM %.0f MB", ram, double(*video) / mib);
    }
    else
    {
        std::snprintf(memoryText_, sizeof memoryText_, "RAM %.0f MB", ram);
    }
}

