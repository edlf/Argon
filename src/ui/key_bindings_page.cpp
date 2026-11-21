// Argon: Argon Options' Key Bindings (key_bindings_page.hpp).
#include "ui/key_bindings_page.hpp"

#include "files/string_table.hpp"
#include "input/player_input.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/viewer_text.hpp"

#include <GLFW/glfw3.h>

using namespace menu::fe;

namespace
{
constexpr float kFirstKeyX = 300.f, kSecondKeyX = 430.f;   // the keys' columns, as the settings lists' options from 300
}

void KeyBindingsPage::open()
{
    shell.goTo(menu::PageKeyBindings);
    edited = services.store.keyBindings();
    selected = 0;
    slot = 0;
    awaitingKey = false;
    followCursor();
}

std::u16string KeyBindingsPage::title() const
{
    return strings.viewer(viewer_text::kKeyBindings.label);
}

std::optional<std::u16string> KeyBindingsPage::ticker() const
{
    if (awaitingKey)
    {
        return strings.viewer(viewer_text::kPressAKey);
    }
    if (selected == kResetRow)
    {
        return strings.frontEnd(0x333, "");
    }
    if (selected == kSaveRow)
    {
        return strings.viewer(viewer_text::kSave.description);
    }
    return strings.viewer(viewer_text::kKeyBindingTicker);
}

bool KeyBindingsPage::onAction() const
{
    return selected < kKeyActionCount;
}

void KeyBindingsPage::input(const menu::MenuInput &in)
{
    if (awaitingKey)
    {
        takeAwaitedKey(in);
        return;
    }
    if (in.b)
    {
        leavePage();
        return;
    }
    if (menus.menuCursor(selected, kRowCount, in))
    {
        play(menu::UiEvent::Cursor);
        followCursor();
    }
    if (onAction())
    {
        changeSlot(in);
    }
    if (in.a)
    {
        runRow();
    }
}

// the first key pressed since A: bound to the action's slot (taken from wherever else it was); Esc or a pad's B keeps
// the old key; a key the viewer keeps for itself is refused and the page keeps waiting
void KeyBindingsPage::takeAwaitedKey(const menu::MenuInput &in)
{
    int key = kUnboundKey;
    if (services.players.capturedKey(key) && key != GLFW_KEY_ESCAPE)
    {
        if (isReservedKey(key))
        {
            play(menu::UiEvent::Refused);
            return;
        }
        edited.bind(KeyAction(selected), slot, key);
        play(menu::UiEvent::Pick);
        awaitingKey = false;
        shell.restartTicker();
        return;
    }
    if (key == GLFW_KEY_ESCAPE || in.b)
    {
        play(menu::UiEvent::Back);
        awaitingKey = false;
        shell.restartTicker();
    }
}

// left / right: the first or second key; X clears it
void KeyBindingsPage::changeSlot(const menu::MenuInput &in)
{
    const int chosen = in.left ? 0 : in.right ? 1 : slot;
    if (chosen != slot)
    {
        slot = chosen;
        play(menu::UiEvent::Cursor);
    }
    if (in.x && edited.key(KeyAction(selected), slot) != kUnboundKey)
    {
        edited.clear(KeyAction(selected), slot);
        play(menu::UiEvent::Cursor);
    }
}

void KeyBindingsPage::runRow()
{
    if (onAction())
    {
        play(menu::UiEvent::Pick);
        services.players.startKeyCapture();
        awaitingKey = true;
        shell.restartTicker();
    }
    else if (selected == kResetRow)
    {
        shell.messageBox().open(strings.viewer(viewer_text::kResetPageQuestion), MessageBoxPage::Kind::YesNo, *this);
    }
    else
    {
        services.store.storeKeyBindings(edited);
        leavePage();
    }
}

void KeyBindingsPage::messageClosed(bool yes)
{
    if (yes)
    {
        play(menu::UiEvent::Pick);
        edited = KeyBindings();
    }
}

void KeyBindingsPage::leavePage()
{
    play(menu::UiEvent::LeftBack);
    shell.argonOptions().openAt(ArgonOptionsPage::kKeyBindings);
}

menu::ListLayout KeyBindingsPage::listLayout() const
{
    return menus.listLayout(menu::kSmallRows, 120.f, kTickerY - 2.f);
}

void KeyBindingsPage::followCursor()
{
    window.follow(selected, kRowCount, menu::rowsThatFit(listLayout(), kRowCount));
}

std::u16string KeyBindingsPage::rowLabel(int row) const
{
    if (row == kResetRow)
    {
        return strings.frontEnd(0x332, "Reset to Default");
    }
    if (row == kSaveRow)
    {
        return strings.viewer(viewer_text::kSave.label);
    }
    return strings.viewer(viewer_text::kKeyActions[row]);
}

// "..." in the slot awaiting its key
std::u16string KeyBindingsPage::keyText(int row, int keySlot) const
{
    if (awaitingKey && row == selected && keySlot == slot)
    {
        return u"...";
    }
    return argon::widenLatin1(keyName(edited.key(KeyAction(row), keySlot)).c_str());
}

void KeyBindingsPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const menu::ListLayout layout = listLayout();
    for (int row = window.first(); row < window.end(); ++row)
    {
        drawRow(canvas, row, layout.rowY(row - window.first()));
    }
    const float markX = atLeft(originX, kRowX) + menu::kScrollMarkIndent;
    MenuKit::drawScrollMarks(canvas, {markX, markX}, layout, window);
    menus.drawHints(canvas, {awaitingKey ? std::u16string() : strings.frontEnd(0x33, "[BUTB] - Back"),
                             awaitingKey ? std::u16string() : strings.frontEnd(0x32, "Select - [BUTA]")});
}

// an action's keys in two columns; on the cursor's row the key not picked is grey
void KeyBindingsPage::drawRow(const FeCanvas &canvas, int row, float y) const
{
    const float originX = canvas.originX();
    const float textY = menus.drawMenuRow(canvas, {atLeft(originX, kRowX), y}, rowLabel(row), menus.listRowLook(menu::kSmallRows, row == selected, kText));
    if (row >= kKeyActionCount)
    {
        return;
    }
    const float columns[kKeySlots] = {kFirstKeyX, kSecondKeyX};
    for (int keySlot = 0; keySlot < kKeySlots; ++keySlot)
    {
        const uint32_t colour = row == selected && keySlot != slot ? kGrey : kText;
        canvas.text({originX + columns[keySlot], textY}, keyText(row, keySlot), {0, colour, 0, kFe, kFe});
    }
}
