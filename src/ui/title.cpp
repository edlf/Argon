// Argon: the title screen "FE TITLE" (vtable 0x19dba0: build 0x34460 fe_title_screen_init, enter 0x34cd0,
// update 0x34660, input 0x34970, draw 0x898a0; docs\frontend\intro-title.md "Title screen"). The viewer opens on it.
#include "ui/title.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"
#include "ui/title_texts.hpp"

using namespace menu::fe;

namespace
{
constexpr double kIntroSeconds = 2.0;        // 0x1a3534
constexpr double kLogoSeconds = 4.9;         // 0x1a39e0
constexpr double kLeavingSeconds = 1.225;    // 0x1a39dc
constexpr double kLogoBackSpeed = 4.0;
}  // namespace

// enter (0x34cd0): the page clock +0x7f8 starts; sequence 1. frontend\titlelogo (shell +0x4458) and
// frontend\dodechstart (+0x3e48) are loaded with the shell (0x30d0b), both flagged animated (header 0x80000).
void TitlePage::open()
{
    door.openMenu();
    shell.models().loadTitle();
    shell.goTo(menu::PageTitle);
    enterTime = glfwGetTime();
    startSequence(Sequence::Intro, enterTime);
}

void TitlePage::startSequence(Sequence next, double now)
{
    sequence = next;
    sequenceStart = now;
}

// 0x34660 sequences: 1 (2 s) -> 8: "Press START to begin" shown, the logo clock runs 0 .. 4.9 s -> 5: held at 4.9.
// Input 0x34970, START / A: in 5 -> 7: the logo runs back at 4x and after 1.225 s the page leaves; in any other
// state fe_shell_set_state(4): state 4 leaves at once (no fly-out). Leaving: UI sound 3, the page off, FE CHOOSESAVE.
void TitlePage::input(const menu::MenuInput &in)
{
    double now = glfwGetTime();
    advance(now);
    if (sequence == Sequence::Leaving && now - sequenceStart > kLeavingSeconds)
    {
        leave();
        return;
    }
    if (!in.a || !shell.models().preloaded())
    {
        return;   // the viewer's: not before the front end's models are loaded
    }
    if (sequence == Sequence::WaitForStart)
    {
        startSequence(Sequence::Leaving, now);
        startPressed = now;
    }
    else if (sequence != Sequence::Leaving)
    {
        leave();
    }
}

void TitlePage::advance(double now)
{
    double elapsed = now - sequenceStart;
    if (sequence == Sequence::Intro && elapsed > kIntroSeconds)
    {
        startSequence(Sequence::LogoIn, now);
        logoShown = now;
    }
    else if (sequence == Sequence::LogoIn && elapsed > kLogoSeconds)
    {
        startSequence(Sequence::WaitForStart, now);
    }
}

void TitlePage::leave()
{
    play(menu::UiEvent::LeftForward);
    shell.chooseSave().open();
}

// the dodecahedron: seconds since enter; past 20 s the base moves on by 10 (a loop over 10 .. 20 s). The logo flies
// in (sequence 8), holds (5), flies back at 4x (7; the dodecahedron's clock drops to 0 once the logo's is under 0.7)
TitlePage::ModelClocks TitlePage::modelClocks() const
{
    double now = glfwGetTime();
    double elapsed = now - enterTime;
    if (elapsed >= 20.0)
    {
        elapsed = 10.0 + std::fmod(elapsed - 10.0, 10.0);
    }
    ModelClocks clocks{0.f, float(elapsed)};
    if (sequence == Sequence::LogoIn)
    {
        clocks.logo = float(std::min(now - logoShown, kLogoSeconds));
    }
    else if (sequence == Sequence::WaitForStart)
    {
        clocks.logo = float(kLogoSeconds);
    }
    else if (sequence == Sequence::Leaving)
    {
        clocks.logo = float(kLogoSeconds - (now - startPressed) * kLogoBackSpeed);
        if (clocks.logo < 0.7f)
        {
            clocks.dodecahedron = 0.f;
        }
        clocks.logo = std::max(clocks.logo, 0.f);
    }
    return clocks;
}

// the page's text elements (ui\title_texts.hpp). The game shows "Press START to begin" from sequence 8; here that line
// is up from the start: the viewer's steady "Loading..." (infotext 0x210) in its place and colour until the models are
// loaded (docs\argon\differences.md), then the blinking "Press START to begin" (START
// already works in sequence 1). No menu bars, logo, title or ticker on this page (0x30303).
void TitlePage::draw(const FeCanvas &canvas)
{
    ImDrawList *drawList = canvas.drawList();
    shell.drawPageModels(drawList);
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    drawStartPrompt(canvas);
    drawText(canvas, kDevelopedBy, kDevelopedBy.colour);
    drawText(canvas, kCopyright, kCopyright.colour);
}

void TitlePage::drawStartPrompt(const FeCanvas &canvas)
{
    if (shell.models().preloaded())
    {
        drawText(canvas, kPressStart, menu::blinkColour(kPressStart.colour, MenuKit::menuBlink()));   // 0xad620's pulse (ui\menukit.cpp)
        return;
    }
    drawLine(canvas, kPressStart.y, kPressStart.colour, strings.info(0x210, "Loading..."));
}

void TitlePage::drawText(const FeCanvas &canvas, const TitleText &text, uint32_t colour)
{
    drawLine(canvas, text.y, colour, strings.frontEnd(text.feString, text.fallback));
}

void TitlePage::drawLine(const FeCanvas &canvas, float y, uint32_t colour, const std::u16string &line)
{
    const float originX = canvas.originX();
    canvas.text({originX + 320.f, y}, line, {0, colour, 2, kFe, kFe});
}

// the dodecahedron and the logo in their clocks, matrix 0x50a180 (identity)
FeScene TitlePage::scene()
{
    FeModels &models = shell.models();
    const ModelClocks clocks = modelClocks();
    FeScene scene;
    scene.models[0] = &models.dodecahedron();
    scene.models[1] = &models.titleLogo();
    scene.clocks[0] = clocks.dodecahedron;
    scene.clocks[1] = clocks.logo;
    scene.zNear = 0.001f;
    return scene;
}
