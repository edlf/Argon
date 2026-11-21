// Argon: FE MESSAGEBOX (vtable 0x19da68: init 0x2a9c0, open 0x2b000, update / draw 0x2ad70, frame 0x2ace0,
// input 0x2b4b0, close 0x2b470; ui\messagebox.hpp)
#include "ui/messagebox.hpp"

#include <vector>

#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

namespace
{
constexpr int kBusyTicks = 30;   // open + 0.5 s (0x1a3504); the box acts once the clock is past it
}

// the cursor on the second button of kinds 5..9 (0x2b408), else on the first
void MessageBoxPage::open(const std::u16string &message, Kind boxKind, MessageListener &caller)
{
    text = message;
    kind = boxKind;
    listener = &caller;
    selected = buttons().first();
    ticks = 0;
    flash = 255;
    closingPad = -1;
    shown = true;
}

void MessageBoxPage::open(const std::u16string &message, Kind boxKind)
{
    open(message, boxKind, nobody);
}

// kind 1 (0x2b089): +0x1f4 = now + seconds; no button, the box closes once the clock passes it (0x2b60b)
void MessageBoxPage::openTimed(const std::u16string &message, float seconds)
{
    open(message, Kind::Timed);
    timedTicks = int(seconds * 60.f);
}

// kind 3 (multiplayer): the controller message, closed only by the pad +0x1f4 (0x2b4b0)
void MessageBoxPage::openForPad(const std::u16string &message, int pad)
{
    open(message, Kind::Controller);
    closingPad = pad;
}

void MessageBoxPage::close()
{
    shown = false;
    listener = &nobody;
}

// input 0x2b4b0: an info box closes on A; the questions (kinds 5, 6, 8, 9) move with up / down and answer on A, B
// does nothing (0x2b7cc); the controller message closes on A / START (+0x88 & 0x110); Timed, kind 7, Saving and
// Loading take no input
void MessageBoxPage::input(const menu::MenuInput &in)
{
    if (kind == Kind::Info || kind == Kind::Controller)
    {
        if (closingPad >= 0 ? players.portPresses(closingPad).a : in.a)
        {
            if (kind == Kind::Info)
            {
                play(menu::UiEvent::Pick);   // the controller message closes silently
            }
            answer(false);
        }
        return;
    }
    if (!buttons().takesCursorInput())
    {
        return;
    }
    moveCursor(in);
    if (in.a)
    {
        play(menu::UiEvent::Pick);
        answer(selected == 0);
    }
}

// every press plays the cursor sound, at the end of the buttons too (0x2b841 / 0x2b892)
void MessageBoxPage::moveCursor(const menu::MenuInput &in)
{
    if (in.up || in.down)
    {
        play(menu::UiEvent::Cursor);
        selected = buttons().moved(selected, in.down);
    }
}

// one 60 Hz frame: the controller message's grey flashes (0x2ad70); Saving / Loading answer once their time is past
void MessageBoxPage::step()
{
    if (!shown)
    {
        return;
    }
    flash = menu::stepFlash(flash);
    ++ticks;
    if (isBusy() && ticks > kBusyTicks)
    {
        answer(true);
    }
    else if (kind == Kind::Timed && ticks > timedTicks)
    {
        answer(false);
    }
}

// closed before the caller hears the answer, so that it can open the next question
void MessageBoxPage::answer(bool yes)
{
    MessageListener &caller = *listener;
    close();
    caller.messageClosed(yes);
}

// 0x2ace0: the gradient frame (52, 150) 536 x 200 (swapped) behind every kind but Saving / Loading
void MessageBoxPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    if (isBusy())
    {
        drawBusyLabel(canvas);
        return;
    }
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 52.f, 536.f), 150.f, boxWidth(originX, 52.f, 536.f), 200.f}, FrameShade::Red);
    drawText(canvas);
    drawButtons(canvas);
}

// init 0x2a9c0 / draw 0x2ad70: the text white (font_draw_text colour 0xffffffff), font 0, from (62, 180), 516 wide,
// at most 10 lines; the controller message grey, flashing
void MessageBoxPage::drawText(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const std::vector<std::u16string> lines = menus.wrapText(text, 0, kFe, menus.wrapWidth());
    float y = 180.f;
    for (size_t i = 0; i < lines.size() && i < 10; ++i)
    {
        canvas.text({atLeft(originX, 62.f), y}, lines[i], {0, textColour(), 0, kFe, kFe});
        y += menus.fontLine(0) * kFe;
    }
}

uint32_t MessageBoxPage::textColour() const
{
    if (kind != Kind::Controller)
    {
        return 0xffffffffu;
    }
    return 0xff000000u | uint32_t(flash) * 0x10101u;
}

// the buttons font 0 centred on 320 at y 280 / 300 / 320 (here the small rows, 24 apart) with the hud cursor (the
// menus' row panel here), every button in its panel, the cursor's filled
void MessageBoxPage::drawButtons(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const std::vector<menu::ButtonLabel> labels = buttons().labels();
    for (size_t i = 0; i < labels.size(); ++i)
    {
        const std::u16string label = strings.frontEnd(labels[i].feString, labels[i].fallback);
        menus.drawMenuRow(
            canvas, {originX + 320.f, 280.f + menu::kSmallRows.step * float(i)}, label, menus.listRowLook(menu::kSmallRows, int(i) == selected, kText).alignedTo(2));
    }
}

// 0x2b341: the text empty, "Saving" / "Loading" in the left header hint (the other hints, the title and the ticker hidden)
void MessageBoxPage::drawBusyLabel(const FeCanvas &canvas) const
{
    const std::u16string label = kind == Kind::Saving ? strings.frontEnd(0x200, "Saving") : strings.frontEnd(0x201, "Loading");
    menus.drawHints(canvas, {label, std::u16string()});
}
