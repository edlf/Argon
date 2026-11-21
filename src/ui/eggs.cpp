// Argon: the Options pages FE EASTEREGGS (vtable 0x19d928: build 0x1ec40, enter 0x1f390, input 0x1f180,
// update 0x1f440, values 0x1f5e0, ticker 0x1f7d0) and FE CHEATMENU (vtable 0x19db38: build 0x19f80, enter 0x1abd0,
// input 0x1a5e0, update 0x1b030, row 0x1b1a0, ticker 0x1b230); docs\frontend\cheats.md "Easter Eggs and Cheats".
// Unlocks: an easter egg per Expert-or-above tournament won with a "bottom row" character (0x84160), the cheat
// menu with a profile name starting "CHEAT" (fe 0x4b1; 0x83af0 sets 0x1bbc26), a cheat by its code (fe 0x3d4 + i)
// typed in FE NAME ENTRY. --unlock-all opens them all. The unlocks and the cheats' effects: app_unlocks.cpp.
#include "ui/eggs.hpp"

#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_common.hpp"
#include "ui/page_id.hpp"

using namespace menu::fe;

namespace
{
constexpr int kEggsOptionsRow = 7, kCheatsOptionsRow = 8;   // FE OPTIONS rows (options.cpp kOptionsRows), where B leaves the cursor
constexpr float kValueX = 462.f, kOnX = 512.f;

uint32_t greyLevel(int v)
{
    uint32_t grey = uint32_t(v);
    return 0xff000000u | (grey << 16) | (grey << 8) | grey;
}
}  // namespace

// ---- FE EASTEREGGS

void EasterEggsPage::open()
{
    selected = 0;
    pulse = 0xff;
    shell.goTo(menu::PageEggs);
}

std::u16string EasterEggsPage::title() const
{
    return strings.frontEnd(0x314, "Easter Eggs");
}

bool EasterEggsPage::rowIsOpen(int row) const
{
    return row < kEggs && progress.eggs[row];
}

// input (0x1f180): B, or A on Exit: back to FE OPTIONS; left / right on an unlocked egg switches it (0x1bbc10);
// up / down the rows 0..7 and Exit (8)
void EasterEggsPage::input(const menu::MenuInput &in)
{
    if (in.b || (in.a && selected == kExitRow))
    {
        play(menu::UiEvent::Back);
        shell.options().openAt(kEggsOptionsRow);
        return;
    }
    if (menus.menuCursor(selected, kEggs + 1, in))
    {
        play(menu::UiEvent::Cursor);
    }
    if ((in.left || in.right) && rowIsOpen(selected))
    {
        progress.eggsOn[selected] = !progress.eggsOn[selected];
        progressStore.storeProgress();
    }
}

void EasterEggsPage::step()
{
    pulse -= 16;
    if (pulse < 0x40)
    {
        pulse = 0xff;
    }
}

// build 0x1ec40: names (fe 0x30c + i, "????" 0x17f while locked) at (62, 100 + 20 i), Exit (8) at (62, 260); here
// the small rows, 24 apart
void EasterEggsPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    for (int i = 0; i <= kExitRow; ++i)
    {
        bool open = rowIsOpen(i);
        std::u16string text = i == kExitRow ? strings.frontEnd(8, "Exit")
                           : open        ? strings.frontEnd(0x30c + i, "Easter Egg")
                                         : strings.frontEnd(0x17f, "????????????????");
        const float rowY = 100.f + menu::kSmallRows.step * float(i);
        float y =
            menus.drawMenuRow(canvas, {atLeft(originX, kRowX), rowY}, text, menus.listRowLook(menu::kSmallRows, i == selected, i < kEggs && !open ? kGrey : kText));
        if (i < kEggs)
        {
            drawOffOn(canvas, i, y);
        }
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), std::u16string()});
}

// Off (fe 6; "Locked" 0x315 while locked) at 462, On (fe 5, hidden while locked) at 512; the choice 0xff85dde5, the
// other grey; on the cursor row the choice pulses
void EasterEggsPage::drawOffOn(const FeCanvas &canvas, int row, float y) const
{
    const float originX = canvas.originX();
    if (!rowIsOpen(row))
    {
        canvas.text({atRight(originX, kValueX), y}, strings.frontEnd(0x315, "Locked"), {0, kGrey, 0, kFe, kFe});
        return;
    }
    bool on = progress.eggsOn[row];
    uint32_t chosen = row == selected ? greyLevel(pulse) : kText;
    canvas.text({atRight(originX, kValueX), y}, strings.frontEnd(6, "Off"), {0, on ? kGrey : chosen, 0, kFe, kFe});
    canvas.text({atRight(originX, kOnX), y}, strings.frontEnd(5, "On"), {0, on ? chosen : kGrey, 0, kFe, kFe});
}

std::optional<std::u16string> EasterEggsPage::ticker() const
{   // 0x1f7d0
    if (selected >= kEggs)
    {
        return strings.frontEnd(0x33b, "Return to the Options menu");
    }
    if (!rowIsOpen(selected))
    {
        return strings.frontEnd(0x317, "Win Expert tournament or above with \"bottom row\" characters to unlock easter eggs.");
    }
    return strings.frontEnd(9, "");
}

// ---- FE CHEATMENU

void CheatMenuPage::open()
{
    selected = 0;
    shell.goTo(menu::PageCheats);
}

std::u16string CheatMenuPage::title() const
{
    return strings.frontEnd(0x300, "Cheat Menu");
}

bool CheatMenuPage::rowIsOpen(int row) const
{
    return row < kCheats && progress.cheats[row];
}

// input (0x1a5e0): rows 0..6 the cheats, 7 Enter Cheat Code, 8 Exit. A on an unlocked cheat 0..4 activates it for
// good; left / right on the unlocked 5 Infinite Shield / 6 Infinite Turbo switch them (0x1bbbf0 / 0x1bbbf1; their
// race effect is not ported). A on Enter Cheat Code: FE NAME ENTRY for a code; Exit / B back to FE OPTIONS.
void CheatMenuPage::input(const menu::MenuInput &in)
{
    if (in.b || (in.a && selected == kExitRow))
    {
        play(menu::UiEvent::Back);
        shell.options().openAt(kCheatsOptionsRow);
        return;
    }
    if (menus.menuCursor(selected, kExitRow + 1, in))
    {
        play(menu::UiEvent::Cursor);
    }
    if (in.a && selected == kEnterCodeRow)
    {
        play(menu::UiEvent::Back);   // 0x1a899: the screen-back sound here too
        shell.nameEntry().openForCheatCode();
        return;
    }
    if (in.a)
    {
        pickCheat();
    }
    if (rowIsOpen(selected) && isSwitch(selected) && (in.left || in.right))
    {
        progressStore.switchCheat(selected);
    }
}

// A on a cheat (0x1a5e0): locked rows and the two switches refuse it, the others activate for good
void CheatMenuPage::pickCheat()
{
    if (!rowIsOpen(selected) || isSwitch(selected))
    {
        play(menu::UiEvent::Refused);
        return;
    }
    play(menu::UiEvent::Pick);
    progressStore.activateCheat(selected);
}

// build 0x19f80: names (fe 0x2f8 + i, "????" 0x17f grey while locked) at (62, 100 + 20 i); Enter Cheat Code (0x301)
// at (62, 240), Exit at (62, 260); here the small rows, 24 apart
void CheatMenuPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    for (int i = 0; i <= kExitRow; ++i)
    {
        std::u16string text = i == kExitRow        ? strings.frontEnd(8, "Exit")
                           : i == kEnterCodeRow ? strings.frontEnd(0x301, "Enter Cheat Code")
                           : rowIsOpen(i)       ? strings.frontEnd(0x2f8 + i, "Cheat")
                                                : strings.frontEnd(0x17f, "????????????????");
        const float rowY = 100.f + menu::kSmallRows.step * float(i);
        float y = menus.drawMenuRow(
            canvas, {atLeft(originX, kRowX), rowY}, text, menus.listRowLook(menu::kSmallRows, i == selected, i < kCheats && !rowIsOpen(i) ? kGrey : kText));
        if (i < kCheats)
        {
            drawValue(canvas, i, y);
        }
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), rightHint()});
}

// the value at 462: "Locked" (0x315) grey, Inactivated (0xb) grey / Activated (0xa) for 0..4, Off (fe 6) for 5 / 6
// with On (fe 5) at 512 (the choice 0xff85dde5, the other grey)
void CheatMenuPage::drawValue(const FeCanvas &canvas, int row, float y) const
{
    const float originX = canvas.originX();
    const bool on = progress.cheatsOn[row];
    if (!rowIsOpen(row))
    {
        canvas.text({atRight(originX, kValueX), y}, strings.frontEnd(0x315, "Locked"), {0, kGrey, 0, kFe, kFe});
    }
    else if (!isSwitch(row))
    {
        canvas.text(
            {atRight(originX, kValueX), y}, on ? strings.frontEnd(0xa, "Activated") : strings.frontEnd(0xb, "Inactivated"), {0, on ? kText : kGrey, 0, kFe, kFe});
    }
    else
    {
        canvas.text({atRight(originX, kValueX), y}, strings.frontEnd(6, "Off"), {0, on ? kGrey : kText, 0, kFe, kFe});
        canvas.text({atRight(originX, kOnX), y}, strings.frontEnd(5, "On"), {0, on ? kText : kGrey, 0, kFe, kFe});
    }
}

// the right hint (0x1b230): Select on an unlocked cheat 0..4, Enter Cheat Code and Exit; Change Settings (0x4d) on an
// unlocked 5 / 6; none while locked
std::u16string CheatMenuPage::rightHint() const
{
    if (selected < kCheats && !rowIsOpen(selected))
    {
        return {};
    }
    return isSwitch(selected) ? strings.frontEnd(0x4d, "[BUTL][BUTR] - Change Settings") : strings.frontEnd(0x32, "Select - [BUTA]");
}

std::optional<std::u16string> CheatMenuPage::ticker() const
{   // 0x1b230
    if (selected == kEnterCodeRow)
    {
        return strings.frontEnd(0x375, "Enter Cheat Codes");
    }
    if (selected == kExitRow)
    {
        return strings.frontEnd(0x33b, "Return to the Options menu");
    }
    if (!rowIsOpen(selected))
    {
        return strings.frontEnd(0x303, "Enter the secret code to unlock this cheat.");
    }
    return isSwitch(selected) ? strings.frontEnd(9, "") : strings.frontEnd(0x302, "Once activated this cheat is permanent for this profile.");
}
