// Argon: the front end's shell draw (CFEShell, ui\fe_shell.hpp): the background and its feedback texture, the page's
// models, title and ticker around the current page (the menu bars and the App side of the front end: app\app_menu.cpp).
// Notes: docs\frontend\README.md. Decoded from default.xbe: shell draw 0x2ffa0, background 0xc2570, menu bars 0xc2870.
// The pages are their own classes (FE MAIN: ui\mainmenu.cpp).
#include "hud/hud_painter.hpp"
#include "movie/loading_flyby.hpp"
#include "movie/story_scene.hpp"
#include "render/frame_copies.hpp"
#include "ui/fe_shell.hpp"
#include "ui/feedback_swirl.hpp"
#include "ui/menu_common.hpp"
#include "ui/page_id.hpp"

using namespace menu::fe;

namespace
{
// the pages whose shell ticker panel xemu does not show; Argon keeps it on Track Records (docs\argon\differences.md)
bool pageShowsTickerBar(int page)
{
    return page != menu::PageCharInfo && page != menu::PageShipInfo && page != menu::PageHighScores && page != menu::PageGhosts;
}

// the darkening panel (0xc1b00 -> panel (-200, -5) 1040 x 490, fill 0x50000000, plasma scale 0.25; the wrapper passes
// EAX = 0x50000000 too: the whole fill is 0x50000000)
void drawDarkening(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    canvas.panel({originX - 200.f, -5.f, 1040.f, 490.f}, {false, false, 0xffffff00u, true, 0x50000000u, 0x50000000u, 0.25f});
}

// HudPainter::panel's outline: 1 unit wide lines centred on the panel's edges, drawn by ImDrawList::AddLine half a screen
// pixel right of and below the points it is given. toScreen / fromScreen: a 640-frame coordinate and the screen pixel
// coordinate the outline lands on (k pixels a unit)
constexpr float kPanelCornerCut = 8.f, kPanelOutlineHalf = 0.5f, kLinePixelOffset = 0.5f;
float toScreen(float units, float pixelsPerUnit)
{
    return units * pixelsPerUnit + kLinePixelOffset;
}
float fromScreen(float pixels, float pixelsPerUnit)
{
    return (pixels - kLinePixelOffset) / pixelsPerUnit;
}

// the inside of a panel (top-left and bottom-right corners cut by 8) across one horizontal band [bandTop, bandBottom],
// in 640-frame units: the widest rectangle in the band that stays off the outline
struct PanelBand
{
    float left, right;
};
PanelBand panelInsideBand(float left, float top, float width, float height, float bandTop, float bandBottom)
{
    const float leftCut = std::max(0.f, kPanelCornerCut - (bandTop - top));
    const float rightCut = std::max(0.f, kPanelCornerCut - (top + height - bandBottom));
    return {left + kPanelOutlineHalf + leftCut, left + width - kPanelOutlineHalf - rightCut};
}

// the menus' feedback swirl (0xa7080 (15, 8)) in 0xa0ffffff
const FeedbackSwirl kMenuSwirl{15.f, 8.f, 0xa0ffffffu};
}  // namespace

// ---- the shell draw (0x2ffa0): the 640 x 480 virtual screen centred, as the HUD

void FeShell::draw()
{
    if (!shown)
    {
        return;
    }
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pixelsPerUnit = ds.y / 480.f, vw = ds.x / pixelsPerUnit, originX = (vw - 640.f) * 0.5f;
    const FeCanvas canvas(sources.painter, drawList, pixelsPerUnit, originX);
    FePage &current = page(currentPage);
    if (current.overBlack())
    {
        drawList->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 255));
    }
    else
    {
        drawBackground(canvas);
    }
    sources.frameCopies.drawTransition(drawList);
    // the loading screen, the title and the credits draw only their own elements over the background (0x808e0: no
    // bars, logo or title)
    if (current.bare())
    {
        current.draw(canvas);
        return;
    }
    drawPageModels(drawList);
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    drawModelFade(drawList);
    host.drawMenuBars(canvas, vw);
    drawTitleAndTicker(canvas);
    active().draw(canvas);   // FE MESSAGEBOX replaces the page
}

// background (0xc2570, the start of the shell draw): no model. Clear to black, the darkening panel, then the feedback;
// then that result is copied into the feedback texture (0xa6eb0) before anything else is drawn. The panel's plasma
// glow, smeared outward by the feedback, is the whole background. Stepped once per 60 Hz tick: the render pass is cut
// there (FrameCopies::cutToMenuFeedback) for the copy; frames between ticks redraw the last copy. The loading screen over its flyby
// or story scene has no background of its own (0x808e0)
void FeShell::drawBackground(const FeCanvas &canvas)
{
    ImDrawList *drawList = canvas.drawList();
    if (currentPage == menu::PageLoader && (sources.flyby.on() || sources.story->playing()))
    {
        return;
    }
    if (!sources.frameCopies.fitMenuFeedback())
    {
        drawDarkening(canvas);
        return;
    }
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (steps != backgroundStep)
    {
        drawList->AddRectFilled(ImVec2(0, 0), ds, IM_COL32(0, 0, 0, 255));   // the clear
        drawDarkening(canvas);
        drawFeedbackSwirl(drawList, sources.frameCopies.menuFeedbackImage(), ds, kMenuSwirl);
        sources.frameCopies.cutToMenuFeedback(drawList);
        backgroundStep = steps;
    }
    drawList->AddImage(sources.frameCopies.menuFeedbackImage(), ImVec2(0, 0), ds);
}

// the model fade (shell draw 0x30303 -> 0xc2810): after the 3D model, the feedback texture again over the whole screen
// at alpha (shell +0xa238 = 0x5a20c8) x 255: 1 - clamp(now - the time the model showed, 0, 1) on the character and
// ship pages (0x152df, 0x16372, 0x33519), 0 on the main menu (0x13708). The locked character's branch (+0xa23c set
// by 0x162da, 0x304fb) keeps it at at least 0.2 (0x1a358c) over the black silhouette
void FeShell::drawModelFade(ImDrawList *drawList)
{
    const FeScene pageScene = scene();
    if (!sources.frameCopies.menuFeedbackImage())
    {
        return;
    }
    const double since = pageScene.fadeInFrom < 0.0 ? -1.0 : glfwGetTime() - pageScene.fadeInFrom;
    const float fade = menu::modelFade(since, pageScene.lightsOff);
    if (fade <= 0.f)
    {
        return;
    }
    drawList->AddImage(sources.frameCopies.menuFeedbackImage(),
                 ImVec2(0, 0),
                 ImGui::GetIO().DisplaySize,
                 ImVec2(0, 0),
                 ImVec2(1, 1),
                 IM_COL32(255, 255, 255, int(fade * 255.f + 0.5f)));
}

// title (shell text +0x9ff0, font 1, 0x31510): the page's title, right edge at 630 - 62, y 36 (sliding: ui\shell_slide.hpp),
// shrunk to 240 wide.
// Ticker (0x2ffa0): panel (62, 370) 506 x 20 fill 0x80303050, the description at (x, 372) clipped to it; xemu shows no
// panel on Character Data, Ship Data and the record pages (Argon keeps it on Track Records). The game draws it at y 372 (2 below the panel's top); here
// its capitals are centred in the panel (1.6 lower) and clipped to the panel's shape inside its outline, its cut corners
// included, one screen pixel row at a time (docs\argon\differences.md). Panel and text are skipped together while the
// element is hidden (0x304bc) and on a page without a ticker (FePage::ticker)
void FeShell::drawTitleAndTicker(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    if (messageBoxPage.isOpen() && messageBoxPage.hidesShellText())
    {
        return;
    }
    const std::u16string title = page(currentPage).title();
    const float titleWidth = sources.painter.textWidth(title, 1, kFe), titleScale = titleWidth > 240.f ? kFe * 240.f / titleWidth : kFe;
    canvas.text({atRight(originX, kRight), titleY(titleWidth * titleScale)}, title, {1, kText, 1, titleScale, titleScale});
    if (const std::optional<std::u16string> ticker = tickerText())
    {
        drawTicker(canvas, *ticker);
    }
}

// the ticker element (shell +0x1230, drawn by 0x2ffa0 from 0x304cb unless hidden)
void FeShell::drawTicker(const FeCanvas &canvas, const std::u16string &text)
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    using menu::kTickerHeight;
    using menu::kTickerY;
    const float left = atLeft(originX, menu::kTickerX), width = spanWidth(originX, menu::kTickerWidth);
    if (pageShowsTickerBar(currentPage))
    {
        canvas.panel({left, kTickerY, width, kTickerHeight}, {false, false, 0xffffff00u, true, 0x80303050u, 0x80303050u});
    }
    const float textX = atRight(originX, tickerX), textY = menu::inkCentredY(kTickerY, kTickerHeight, kFe);
    const float insideTop = toScreen(kTickerY + kPanelOutlineHalf, pixelsPerUnit), insideBottom = toScreen(kTickerY + kTickerHeight - kPanelOutlineHalf, pixelsPerUnit);
    for (float rowTop = std::ceil(insideTop); rowTop < std::floor(insideBottom); rowTop += 1.f)
    {
        const PanelBand band = panelInsideBand(left, kTickerY, width, kTickerHeight, fromScreen(rowTop, pixelsPerUnit), fromScreen(rowTop + 1.f, pixelsPerUnit));
        drawList->PushClipRect(ImVec2(toScreen(band.left, pixelsPerUnit), rowTop), ImVec2(toScreen(band.right, pixelsPerUnit), rowTop + 1.f), true);
        canvas.text({textX, textY}, text, {0, kText, 0, kFe, kFe});
        drawList->PopClipRect();
    }
}
