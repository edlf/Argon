// Argon: the game's main menu (front end "FE MAIN" page of CFEShell, vtable 0x19d798). Notes: docs\frontend\README.md.
// Decoded from default.xbe: page build 0x288b0, enter 0x29d30, input 0x28e90, update 0x2a520, item model 0x2a720,
// text element draw 0xad620 (row panels). The shell around it: ui\menu.cpp.
#include "ui/mainmenu.hpp"

#include "files/string_table.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_common.hpp"
#include "ui/page_id.hpp"
#include "ui/viewer_text.hpp"

using namespace menu::fe;

namespace
{
// rows (0x288b0): string id, ticker; the first five have a model (frontend\mainmenu\..., ui\fe_models.cpp)
struct MenuRow
{
    int titleId, tickerId;
    ViewerText fallback;
    ViewerText tickerFallback;
};
const MenuRow kMenuRows[MainMenuPage::kRowKinds] = {
    {0x79, 0x8c, {u"Tournament"}, {u"Unlock new speeds, tracks, and characters."}},
    {0x7a, 0x8d, {u"Time Attack"}, {u"Race against the clock."}},
    {0x7b, 0x8e, {u"Quick Race"}, {u"Compete in an exhibition race."}},
    {0x7c, 0x8f, {u"Multiplayer"}, {u"Race up to four player split-screen."}},
    {0x7d, 0x90, {u"Options"}, {u"Change settings, view records, and customize music."}},
    // the game's Game Demos row (fe 0x7e / 0x91, XDemos.xbe): the viewer's editor, so its own text (-1: no fe id)
    {-1, -1, viewer_text::kDebug, viewer_text::kDebugTicker},
    // viewer row: Argon's own settings (no fe strings)
    {-1, -1, viewer_text::kArgonOptions, viewer_text::kArgonOptionsTicker},
    // viewer row: the game's "Launch the Xbox Dashboard." exit (fe 0x245, Options Exit on builds with the dashboard
    // enabled) as a return to the launcher: saves the loaded profile and closes the app
    {0x8, -1, {u"Exit"}, viewer_text::kExitWithProfile},
};
}  // namespace

// enter (0x29d30): hints 0x32 "Select - [BUTA]" / 0x33 "[BUTB] - Back", title 0x78 "Main Menu"; rows start at x 640,
// move to 62 in 10 steps
void MainMenuPage::enter()
{
    for (float &x : rowX)
    {
        x = kRowFrom;
    }
    followCursor();
    if (tourney.awardsPending)
    {
        shell.goTo(menu::PageAwards);   // 0x29e60: after a tournament win, FE AWARDS first
    }
    else if (fastestLapPending)
    {   // 0x29f49: the page left pending (0x5994c1), Time Attack's "FE FASTESTLAP" after a profile best
        fastestLapPending = false;
        shell.trackRecords().openAfterRace();
    }
}

std::u16string MainMenuPage::title() const
{
    return strings.frontEnd(0x78, "Main Menu");
}

std::optional<std::u16string> MainMenuPage::ticker() const
{
    if (selectedKind() == kExit)
    {
        return strings.viewer(profiles.hasCurrent() ? viewer_text::kExitWithProfile : viewer_text::kExitToLauncher);
    }
    const MenuRow &row = kMenuRows[selectedKind()];
    return strings.frontEnd(row.tickerId, row.tickerFallback);
}

// 0x28e90: up / down move the selection (wrapping, Game Demos only when d:\xdemos exists: always here), A / START
// picks, B back to the title; both slide the title and hints out first (fe_shell_set_state(4), 0x2a520 cases 4 / 7)
void MainMenuPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        shell.slideOut(menu::UiEvent::LeftBack);
        shell.title().open();
        return;
    }
    menu::MenuInput upDown;
    upDown.up = in.up;
    upDown.down = in.down;
    if (menus.menuCursor(selected, kRows, upDown))
    {
        play(menu::UiEvent::Cursor);
        followCursor();
    }
    if (in.a)
    {
        pickRow();
    }
}

void MainMenuPage::messageClosed(bool yes)
{
    if (yes)
    {
        door.quit();
    }
}

// Tournament / Time Attack / Quick Race: FE CHARSELECT in their game mode (0x28e90: modes 0, 3, 1); Options; the
// editor. Each slides the rows, title and hints out first (0x291e9: the rows from 62 to 640 in 10 steps) and the next
// page opens 0.1 s after the pick (0x2a520 case 7), the rows a little over half way out
void MainMenuPage::pickRow()
{
    const Row kind = selectedKind();
    if (kind == kExit)
    {
        shell.messageBox().open(argon::widenLatin1(profiles.hasCurrent() ? "Save your profile and return to the launcher?" : "Return to the launcher?"),
                                MessageBoxPage::Kind::YesNo,
                                *this);
        return;
    }
    if (kind == kMultiplayer && FeShell::connectedGamepads() < 2)
    {
        play(menu::UiEvent::Refused);   // 0x28e90: fewer than two controllers
        return;
    }
    shell.slideOut(menu::UiEvent::LeftForward);
    switch (kind)
    {
    case kTournament:
        startMode(modes.tournament());
        break;
    case kTimeAttack:
        startMode(modes.timeAttack());
        break;
    case kQuickRace:
        startMode(modes.quickRace());
        break;
    case kMultiplayer:
        startMode(modes.multiplayer());   // game mode 4; FE CHARSELECT seats the players
        break;
    case kOptions:
        shell.options().open();
        break;
    case kArgonSettings:
        shell.argonOptions().open();
        break;
    default:
        break;   // the editor: slideOutDone
    }
}

void MainMenuPage::select(int row)
{
    selected = row;
    followCursor();
}

void MainMenuPage::selectKind(Row kind)
{
    for (int row = 0; row < kRows; ++row)
    {
        if (kShownRows[row] == kind)
        {
            select(row);
        }
    }
}

void MainMenuPage::startMode(GameLogic &mode)
{
    multiplayer.leave();
    modes.select(mode);
    shell.goTo(menu::PageCharSelect);
}

// one 60 Hz step: the rows slide in, or back out to x 640 while the shell slides out
void MainMenuPage::step()
{
    if (shell.isSlidingOut())
    {
        menu::slideRows(rowX, kRowX, kRowFrom);
    }
    else
    {
        menu::slideRows(rowX, kRowFrom, kRowX);
    }
}

void MainMenuPage::slideOutDone()
{
#if ARGON_DEVELOP
    door.closeMenu();
    door.startEditor();
#endif
}

// the selected row's model (0x2a720), camera 1: rotation 0, translation (-4, -2 for Quick Race else -1, 0);
// Multiplayer's x / y axes x 0.75. The rows past Options have none
FeScene MainMenuPage::scene()
{
    FeScene scene;
    const Row kind = selectedKind();
    if (kind >= FeModels::kMenuItems)
    {
        return scene;
    }
    scene.models[0] = &shell.models().menuItem(kind);
    scene.placement[3] = glm::vec4(-4.f, kind == kQuickRace ? -2.f : -1.f, 0.f, 1.f);
    if (kind == kMultiplayer)
    {
        scene.placement[0] *= 0.75f;
        scene.placement[1] *= 0.75f;
    }
    scene.zNear = 0.001f;
    return scene;
}

// header hints (0x29d30): 0x33 "[BUTB] - Back" in the left text (x 62), 0x32 "Select - [BUTA]" in the right one
// (ending at 568), y 421 in the code; drawn at 1 with their icons (xemu), at y 397
void MainMenuPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const int pads = FeShell::connectedGamepads();
    for (int i = window.first(); i < window.end(); ++i)
    {
        drawRow(canvas, i, pads);
    }
    const float firstX = atLeft(originX, rowX[0]), lastX = atLeft(originX, rowX[window.end() - window.first() - 1]);
    MenuKit::drawScrollMarks(canvas, {firstX + menu::kScrollMarkIndent, lastX + menu::kScrollMarkIndent}, listLayout(), window);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
}

// rows (0x288b0 / 0xad620): font 1 at 0.75, a chamfer panel behind each (x - h/2, w + h, h = the font's line at 0.75 =
// 26, as measured), the text centred in it; the selected one filled 0xffa08040 with the bright outline. Multiplayer is
// grey (0xff555555) with fewer than 2 pads (0x2a520). The game: y 105 + 40 i for its 6 rows; here the large rows 38
// apart so the viewer's extra rows (Argon Options, Exit) clear the ticker at y 370; the developer build's Debug row
// does not fit: its list scrolls (ui\menu_list.hpp)
menu::ListLayout MainMenuPage::listLayout() const
{
    return menus.listLayout(menu::kLargeRows.spacedBy(38.f), 105.f, kTickerY - 2.f);
}

void MainMenuPage::followCursor()
{
    window.follow(selected, kRows, menu::rowsThatFit(listLayout(), kRows));
}

void MainMenuPage::drawRow(const FeCanvas &canvas, int row, int pads) const
{
    const float originX = canvas.originX();
    const menu::ListLayout layout = listLayout();
    const float scale = menu::kLargeRows.scale, line = menus.fontLine(menu::kLargeRows.font), h = layout.rowHeight;
    const Row kind = kShownRows[row];
    const int slot = row - window.first();
    std::u16string text = strings.frontEnd(kMenuRows[kind].titleId, kMenuRows[kind].fallback);
    const float w = painter.textWidth(text, menu::kLargeRows.font, scale), x = atLeft(originX, rowX[slot]), y = layout.rowY(slot);
    const bool sel = row == selected;
    canvas.panel({x - h * 0.5f, y, w + h, h}, {sel, true, 0xffffff00u, sel, 0xffa08040u, 0xffa08040u});
    uint32_t colour = kText;
    if (kind == kMultiplayer && pads < 2)
    {
        colour = kGrey;
    }
    if (sel && colour != kGrey)
    {
        colour = menu::blinkColour(colour, MenuKit::menuBlink());   // 0xad620 (ui\menukit.cpp)
    }
    canvas.text({x, y + (h - line * scale) * 0.5f}, text, {menu::kLargeRows.font, colour, 0, scale, scale});
}
