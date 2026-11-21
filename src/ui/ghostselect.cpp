// Argon: FE GHOSTSELECT "Select Ghost" (vtable 0x19e0f8: build 0x234d0, input 0x23b40, update 0x239c0, draw
// 0x238d0, messages 0x241b0, list 0x24c90): after FE SPEEDSELECT in Time Attack. Rows: "No Loaded Ghost" (fe 0x349),
// then the saved ghosts of this track (any character), named by their character; A picks one (row 0: none) and goes
// on to the loading screen (update: sequence 7 -> LOADER), B back to FE SPEEDSELECT (sequence 4). The ghost files:
// app_ghosts.cpp.
#include "ui/ghostselect.hpp"
#include "ui/lap_time_text.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

void GhostSelectPage::open(int pilotId, int trackId)
{
    pilot = pilotId;
    track = trackId;
    ghosts.scan();
    ghostRows = ghosts.ofTrack(track);
    if (ghostRows.empty())
    {
        startRace();
        return;
    }
    selected = firstShown = 0;
    shell.goTo(menu::PageGhostSelect);
}

std::u16string GhostSelectPage::title() const
{
    return strings.frontEnd(0x348, "Select Ghost");
}

std::optional<std::u16string> GhostSelectPage::ticker() const
{
    return strings.frontEnd(0x34d, "Select which ghost you want to race against.");
}

// input 0x23b40: up / down move the cursor through the rows, scrolling the five shown (+0x1420); A picks the row
// (the first one: no loaded ghost, z:\eventbuffer\preloaded.* emptied), B back (Y: the Xbox Dashboard, not ported)
void GhostSelectPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::LeftBack);   // 0x239c0 case 4
        shell.speedSelect().open(pilot, track);
        return;
    }
    moveCursor(in);
    if (in.a)
    {
        play(menu::UiEvent::Pick);
        pickRow();
    }
}

void GhostSelectPage::moveCursor(const menu::MenuInput &in)
{
    const int before = selected;
    if (in.up && selected > 0)
    {
        --selected;
    }
    if (in.down && selected + 1 < rowCount())
    {
        ++selected;
    }
    if (selected != before)
    {
        play(menu::UiEvent::Cursor);
    }
    firstShown = std::clamp(firstShown, std::max(0, selected - (kRowsShown - 1)), selected);
}

void GhostSelectPage::pickRow()
{
    TimeAttackLogic &ta = modes.timeAttack();
    ta.useLoadedGhost({});
    if (selected == 0)
    {
        leaveForRace();
        return;
    }
    shell.messageBox().open(std::u16string(), MessageBoxPage::Kind::Loading, *this);   // 0x241b0: kind 0xa
}

void GhostSelectPage::messageClosed(bool loadNow)
{
    if (loadNow)
    {
        loadPickedGhost();
    }
}

// after "Loading" (0x2b4b0 kind 0xa): a ghost that does not load: fe 0x1f8 "This is an invalid save."
void GhostSelectPage::loadPickedGhost()
{
    const SavedGhost &picked = ghosts.all()[size_t(ghostRows[size_t(selected - 1)])];
    ghost::Lap lap;
    if (!GhostLibrary::read(picked.file, lap))
    {
        shell.messageBox().open(strings.frontEnd(0x1f8, "This is an invalid save."), MessageBoxPage::Kind::Info);
        return;
    }
    modes.timeAttack().useLoadedGhost(std::move(lap));
    leaveForRace();
}

// 0x239c0 case 7: the switch to the loader
void GhostSelectPage::leaveForRace()
{
    play(menu::UiEvent::LeftForward);
    startRace();
}

void GhostSelectPage::startRace()
{
    races.startQuickRace(pilot, track);
}

std::u16string GhostSelectPage::rowLabel(int row) const
{
    if (row == 0)
    {
        return strings.frontEnd(0x349, "No Loaded Ghost");
    }
    return ghostKeeper.ghostLabel(ghosts.all()[size_t(ghostRows[size_t(row - 1)])]);
}

// a ghost row's lap time (the file's; not in the game: docs\argon\differences.md); none for "No Loaded Ghost"
std::u16string GhostSelectPage::rowTime(int row) const
{
    if (row == 0)
    {
        return std::u16string();
    }
    return lapTimeText(ghosts.all()[size_t(ghostRows[size_t(row - 1)])].time, LapShown::Record);
}

// build 0x234d0 / draw 0x238d0: frames (52, 100) 536 x 40 (swapped; the game leaves its header text empty, the viewer
// shows the track's name as FE GHOSTMANAGE does) and (52, 140) 536 x 160; five rows (font 0) at (62, 170 + 20 i), the
// viewer adds each ghost's lap time right-aligned at 568; hints Back (0x33) / Select (0x32); the list's arrows
// (0x24c90): icons\up.dds at (62 + 20, 148) while rows are scrolled off the top, icons\down.dds at (82, 279) while
// more follow, 40 x 20
void GhostSelectPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 52.f, 536.f), 100.f, boxWidth(originX, 52.f, 536.f), 40.f}, FrameShade::Red);
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 52.f, 536.f), 140.f, boxWidth(originX, 52.f, 536.f), 160.f});
    menus.drawGhostTrackName(canvas, track);
    for (int r = 0; r < kRowsShown && firstShown + r < rowCount(); ++r)
    {
        const int row = firstShown + r;
        const float textY =
            menus.drawMenuRow(canvas, {atLeft(originX, kRowX), 170.f + 20.f * float(r)}, rowLabel(row), {0, kFe, menus.fontLine(0), row == selected, kText});
        canvas.text({atRight(originX, kRight), textY}, rowTime(row), {0, kText, 1, kFe, kFe});
    }
    if (firstShown > 0)
    {
        menus.drawScrollArrow(canvas, "BUTU", atLeft(originX, 82.f), 148.f);
    }
    if (firstShown + kRowsShown < rowCount())
    {
        menus.drawScrollArrow(canvas, "BUTD", atLeft(originX, 82.f), 279.f);
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
}
