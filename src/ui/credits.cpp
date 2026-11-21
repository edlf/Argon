// Argon: FE OPTIONS CREDITS (vtable 0x19e1c8: enter 0x2e170, input 0x2e110, update 0x2e390 = the credits
// player; docs\frontend\main-menu.md "Credits").
#include "ui/credits.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "craft/craftsim.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

namespace
{
// 0x1b99a0: 4 picture motions (x0, y0, x1, y1, scale0, scale1) over 8 s (x 0.125)
const float kMotion[4][6] = {{0, 350, 200, 500, 1.5f, 2.f}, {500, 250, 450, 250, 0.9f, 1.f}, {640, 350, 440, 500, 1.5f, 2.f}, {140, 250, 190, 250, 0.9f, 1.f}};
constexpr int kLastPage = 0x61;           // fe 0xb68 + n, n <= 0x61
constexpr double kPictureSeconds = 8.0;
constexpr float kTextScale = 0.65f;       // 0xb11f0
constexpr int kCreditsOptionsRow = 5;     // FE OPTIONS row, where leaving leaves the cursor
constexpr int kPictures = 16;

std::string pictureKey(int character)
{
    return "charbitmap" + std::to_string(character);
}
}  // namespace

// enter (0x2e170): the FE clock restarts; the text starts at y 50 with page 0, scrolling 2 s later; picture
// slot 0 shows picture 8 with motion 0 from now + 1 s, slot 1 picture 0 with motion 1 from now
void CreditsPage::open()
{
    double now = glfwGetTime();
    startTime = now + 2.0;
    textY = 50.f;
    firstPage = 0;
    slots[0] = {8, 0, now + 1.0};
    slots[1] = {0, 1, now};
    loadPictures();
    shell.goTo(menu::PageCredits);
}

// the 16 pictures: data\charbitmaps\<character>.dds, the characters in id order (main_menu 0x30af0 loads them into
// the shell's elements +0x765c; 256 x 512, Ka Hekili's and Max's 512 x 512)
void CreditsPage::loadPictures()
{
    for (int i = 0; i < kPictures; ++i)
    {
        if (!painter.texture(pictureKey(i)))
        {
            painter.loadTexture(pictureKey(i), gameFolder + "/data/charbitmaps/" + argon::pilotFolder(i) + ".dds");
        }
    }
}

bool CreditsPage::started() const
{
    return glfwGetTime() > startTime;
}

// input (0x2e110): A or B leaves (back to FE OPTIONS); the update leaves too once the last page is gone
void CreditsPage::input(const menu::MenuInput &in)
{
    if (in.a || in.b)
    {
        leave();
    }
}

void CreditsPage::leave()
{
    play(menu::UiEvent::LeftBack);
    shell.options().openAt(kCreditsOptionsRow);
}

// update 0x2e390, per 60 Hz frame: once started, the text moves up 1 px
void CreditsPage::step()
{
    if (started())
    {
        textY -= 1.f;
    }
}

void CreditsPage::draw(const FeCanvas &canvas)
{
    if (started())
    {
        drawPictures(canvas);
    }
    dropPagesAboveTop();
    if (firstPage > kLastPage)
    {
        leave();
        return;
    }
    drawText(canvas);
}

// ---- the text: pages fe 0xb68 + n, "[C:aarrggbb]" colour and "[S:x]" scale tags, lines centred on x 320 at 0.65,
// stacked by their heights; a page fully above the top is dropped

std::vector<CreditsPage::Line> CreditsPage::layout(int page) const
{
    std::u16string text = strings.frontEnd(0xb68 + page, "");
    std::vector<Line> lines;
    Style style;
    for (size_t i = 0; i <= text.size();)
    {
        size_t end = std::min(text.find(u'\n', i), text.size());
        lines.push_back(measure(splitTags(text.substr(i, end - i), style), style));
        i = end + 1;
    }
    return lines;
}

std::vector<CreditsPage::Segment> CreditsPage::splitTags(const std::u16string &line, Style &style) const
{
    std::vector<Segment> segments;
    std::u16string text;
    for (size_t p = 0; p < line.size();)
    {
        bool tag = line[p] == u'[' && p + 3 < line.size() && (line[p + 1] == u'C' || line[p + 1] == u'S') && line[p + 2] == u':';
        size_t close = tag ? line.find(u']', p) : std::u16string::npos;
        if (close == std::u16string::npos)
        {
            text += line[p++];
            continue;
        }
        if (!text.empty())
        {
            segments.push_back({text, style.colour, style.scale});
        }
        text.clear();
        std::string value;
        for (size_t r = p + 3; r < close; ++r)
        {
            value += char(line[r]);   // the tags' values are ASCII
        }
        if (line[p + 1] == u'C')
        {
            style.colour = uint32_t(std::stoul(value, nullptr, 16));
        }
        else
        {
            style.scale = float(std::atof(value.c_str()));
        }
        p = close + 1;
    }
    if (!text.empty())
    {
        segments.push_back({text, style.colour, style.scale});
    }
    return segments;
}

CreditsPage::Line CreditsPage::measure(std::vector<Segment> segments, const Style &style) const
{
    const float line0 = menus.fontLine(0);
    Line line{std::move(segments)};
    for (const Segment &segment : line.segments)
    {
        const float scale = kTextScale * segment.scale * kFe;
        line.width += painter.textWidth(segment.text, 0, scale);
        line.height = std::max({line.height, line0 * kTextScale * segment.scale, painter.tallestIcon(segment.text, scale)});
    }
    if (line.height == 0)
    {
        line.height = line0 * kTextScale * style.scale;   // an empty line: the current scale's height
    }
    return line;
}

float CreditsPage::height(const std::vector<Line> &lines)
{
    float h = 0;
    for (const Line &line : lines)
    {
        h += line.height;
    }
    return h;
}

void CreditsPage::dropPagesAboveTop()
{
    while (firstPage <= kLastPage)
    {
        float h = height(layout(firstPage));
        if (h > 0.f && textY + h >= 0.f)
        {
            return;
        }
        if (h > 0.f)
        {
            textY += h;
        }
        ++firstPage;
    }
}

void CreditsPage::drawText(const FeCanvas &canvas) const
{
    float y = textY;
    for (int page = firstPage; y < 480.f && page <= kLastPage; ++page)
    {
        std::vector<Line> lines = layout(page);
        drawLines(canvas, y, lines);
        y += height(lines);
    }
}

void CreditsPage::drawLines(const FeCanvas &canvas, float y, const std::vector<Line> &lines) const
{
    const float originX = canvas.originX();
    for (const Line &line : lines)
    {
        if (y < 480.f && y + line.height > 0.f)
        {
            float x = originX + 320.f - line.width * 0.5f;
            for (const Segment &segment : line.segments)
            {
                float scale = kTextScale * segment.scale * kFe;
                canvas.text({x, y}, segment.text, {0, segment.colour, 0, scale, scale});
                x += painter.textWidth(segment.text, 0, scale);
            }
        }
        y += line.height;
    }
}

// ---- the pictures (0x2e390, while started): two at a time, each moving / scaling over 8 s from its motion, then
// the next motion (+2, wrapping in its pair) and the next picture (of 16); alpha blended (SRCALPHA / INVSRCALPHA).
// Then a black rectangle over the screen (0x2e7ab: 0xab290 (0, 0, 640, 480), alpha 255 - 255 x clamp(now - start,
// 0, 1)): the pictures and background come out of black over the first second

void CreditsPage::drawPictures(const FeCanvas &canvas)
{
    ImDrawList *drawList = canvas.drawList();
    double now = glfwGetTime();
    for (PictureSlot &slot : slots)
    {
        if (now - slot.start > kPictureSeconds)
        {
            slot.motion = (slot.motion + 2) % 4;
            slot.picture = (slot.picture + 1) % kPictures;
            slot.start = now;
        }
        drawPicture(canvas, slot, float((now - slot.start) / kPictureSeconds));
    }
    const float black = 1.f - std::clamp(float(now - startTime), 0.f, 1.f);
    drawList->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, int(black * 255.f + 0.5f)));
}

// the picture at its texture's size x the motion's scale, centred on the motion's (x, y)
void CreditsPage::drawPicture(const FeCanvas &canvas, const PictureSlot &slot, float progress) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    const Found<const HudTexture> picture = painter.texture(pictureKey(slot.picture));
    if (!picture)
    {
        return;
    }
    const float *motion = kMotion[slot.motion];
    const float x = motion[0] + (motion[2] - motion[0]) * progress, y = motion[1] + (motion[3] - motion[1]) * progress, scale = motion[4] + (motion[5] - motion[4]) * progress;
    const float w = float(picture->w) * scale, h = float(picture->h) * scale;
    drawList->AddImage(picture->image, ImVec2((originX + x - w * 0.5f) * pixelsPerUnit, (y - h * 0.5f) * pixelsPerUnit), ImVec2((originX + x + w * 0.5f) * pixelsPerUnit, (y + h * 0.5f) * pixelsPerUnit));
}
