// Argon: Argon Options (argon_options.hpp).
#include "ui/argon_options.hpp"

#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/viewer_text.hpp"

using namespace menu::fe;

namespace
{
// the rows' text, Exit's from the game's tables
const viewer_text::RowText *const kPageRows[ArgonOptionsPage::kExit] = {
    &viewer_text::kGeneral, &viewer_text::kGraphics, &viewer_text::kAudio, &viewer_text::kGameFixes, &viewer_text::kKeyBindings};

// the settings list each of the first rows opens
constexpr int kListPages[ArgonOptionsPage::kKeyBindings] = {
    menu::PageArgonGeneral, menu::PageArgonGraphics, menu::PageArgonAudio, menu::PageArgonFixes};
}  // namespace

void ArgonOptionsPage::open()
{
    openAt(kGeneral);
}

void ArgonOptionsPage::openAt(Row row)
{
    shell.goTo(menu::PageArgonSettings);
    selected = row;
    for (float &x : rowX)
    {
        x = kRowFrom;
    }
    followCursor();
}

std::u16string ArgonOptionsPage::title() const
{
    return strings.viewer(viewer_text::kArgonOptions);
}

std::optional<std::u16string> ArgonOptionsPage::ticker() const
{
    if (selected == kExit)
    {
        return strings.frontEnd(0x374, "Return to Main Menu");
    }
    return strings.viewer(kPageRows[selected]->description);
}

std::u16string ArgonOptionsPage::rowLabel(int row) const
{
    return row == kExit ? strings.frontEnd(0x8, "Exit") : strings.viewer(kPageRows[row]->label);
}

void ArgonOptionsPage::step()
{
    menu::slideRows(rowX, kRowFrom, kRowX);
}

// up / down, A opens the row's page; Exit or B back to FE MAIN on the Argon Options row
void ArgonOptionsPage::input(const menu::MenuInput &in)
{
    if (menus.menuCursor(selected, kRowCount, in))
    {
        play(menu::UiEvent::Cursor);
        followCursor();
    }
    if (in.b || (in.a && selected == kExit))
    {
        leaveToMain();
        return;
    }
    if (in.a)
    {
        shell.slideOut(menu::UiEvent::LeftForward);
        openRow();
    }
}

void ArgonOptionsPage::leaveToMain()
{
    play(menu::UiEvent::LeftBack);
    shell.mainMenu().selectKind(MainMenuPage::kArgonSettings);
    shell.goTo(menu::PageMain);
}

void ArgonOptionsPage::openRow()
{
    if (selected == kKeyBindings)
    {
        shell.keyBindings().open();
        return;
    }
    shell.settings(kListPages[selected]).open();
}

menu::ListLayout ArgonOptionsPage::listLayout() const
{
    return menus.listLayout(menu::kLargeRows, 100.f, kTickerY - 2.f);
}

void ArgonOptionsPage::followCursor()
{
    window.follow(selected, kRowCount, menu::rowsThatFit(listLayout(), kRowCount));
}

void ArgonOptionsPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const menu::ListLayout layout = listLayout();
    for (int row = window.first(); row < window.end(); ++row)
    {
        const int slot = row - window.first();
        menus.drawMenuRow(canvas, {atLeft(originX, rowX[slot]), layout.rowY(slot)}, rowLabel(row), menus.listRowLook(menu::kLargeRows, row == selected, kText));
    }
    const float firstX = atLeft(originX, rowX[0]), lastX = atLeft(originX, rowX[window.end() - window.first() - 1]);
    MenuKit::drawScrollMarks(canvas, {firstX + menu::kScrollMarkIndent, lastX + menu::kScrollMarkIndent}, layout, window);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
}
