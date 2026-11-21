#include "ui/multiplayer_end.hpp"
#include "ui/lap_time_text.hpp"

#include <algorithm>

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/race.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"

using namespace menu::race;

namespace
{
// 0x85f10: columns 0x1b3ccc.. 115 / 115 / 95 wide from the right margin, the table's top 0x1b3cc8 = 220; headers at
// top + 5, the players' rows centred on top + 16 + 32 n from n = 1
constexpr float kTimeX = kRight - 325.f + 5.f, kLapX = kRight - 210.f + 5.f, kWinsX = kRight - 47.5f + 5.f;
constexpr float kTop = 220.f, kHeaderY = kTop + 5.f, kRowY = kTop + 16.f, kRowStep = 32.f, kNameWidth = 210.f;

std::u16string number(int v)
{
    return argon::widenLatin1(std::to_string(v).c_str());
}
}  // namespace

void MultiplayerEndMenu::open()
{
    confirming = false;
    selected = 0;
}

// any player's pad: up / down / A (0x3bf60 in state 7: messages 5 / 6 / 9); every item asks "Are you sure?" first
void MultiplayerEndMenu::input(const menu::MenuInput &in)
{
    if (MenuKit::stepCursor(selected, confirming ? 2 : kItems, in))
    {
        audio.playUi(2);
    }
    if (!in.a)
    {
        return;
    }
    audio.playUi(0);
    if (confirming)
    {
        answerConfirm();
        return;
    }
    confirmRow = selected;
    confirming = true;
    selected = 0;
}

void MultiplayerEndMenu::answerConfirm()
{
    if (selected == 1)
    {   // No
        confirming = false;
        selected = confirmRow;
        return;
    }
    host.resetRaceEnd();
    runAction(confirmRow);
}

// Race Again (0x880d0: GAMELOGIC message 5), Track Select, Main Menu (0x88000: state 8, back to the front end)
void MultiplayerEndMenu::runAction(int item)
{
    if (item == kRaceAgain)
    {
        host.restartRace();
        return;
    }
    host.quitRaceTo(item == kTrackSelect ? menu::PageCourseSelect : menu::PageMain);
}

std::u16string MultiplayerEndMenu::label(int item) const
{
    static const int kIds[kItems] = {0x3fa, 0x3fb, 0x3fc};
    static const char *const kFallbacks[kItems] = {"Race Again", "Track Select", "Main Menu"};
    return strings.race(kIds[item], kFallbacks[item]);
}

// "Race Over" (0x41c) font 1 right-aligned at (568, 36) (0x85f10); the items centred on 320 at y 100 / 135 / 170 (the confirm page the same rows);
// the players' table below; hint "Select - [BUTA]" (0x408)
void MultiplayerEndMenu::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    canvas.text({atRight(originX, kRight), 36.f}, strings.race(0x41c, "Race Over"), {1, kText, 1, kFe, kFe});
    drawItems(canvas);
    drawTable(canvas);
    menus.drawHints(canvas, {std::u16string(), strings.race(0x408, "Select - [BUTA]"), 1.f});
}

void MultiplayerEndMenu::drawItems(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float headingScale = 0.7f, step = menu::kLargeRows.step;   // the items: the large rows (the game: 0.7, 35 apart)
    if (confirming)
    {
        canvas.text({originX + 320.f, 100.f}, strings.race(0x40c, "Are you sure?"), {1, kText, 2, headingScale, headingScale});
        const std::u16string answers[2] = {strings.race(0x40a, "Yes"), strings.race(0x40b, "No")};
        for (int i = 0; i < 2; ++i)
        {
            menus.drawMenuRow(
                canvas, {originX + 320.f, 135.f + step * float(i)}, answers[i], menus.listRowLook(menu::kLargeRows, i == selected, kText).alignedTo(2));
        }
        return;
    }
    for (int i = 0; i < kItems; ++i)
    {
        menus.drawMenuRow(canvas, {originX + 320.f, 100.f + step * float(i)}, label(i), menus.listRowLook(menu::kLargeRows, i == selected, kText).alignedTo(2));
    }
}

// headers (orange) "Position" (0x271), "Race Time" (0x294), "Lap Time" (0x295), "Wins" (0x429, centred); a row per
// player in the finish order (0x86e70): "n: [CHARc] Player p" (infotext 0x218 + controller; a knocked-out one without
// the place) scaled to fit 210, the race time (DNF 0x40e; the one left after the knock-outs "By Default" 0x515), the
// lap time, the wins this session (0x52af88)
void MultiplayerEndMenu::drawTable(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.text({atLeft(originX, 67.f), kHeaderY}, strings.race(0x271, "Position"), {0, kHead, 0, kFe, kFe});
    canvas.text({atRight(originX, kTimeX), kHeaderY}, strings.race(0x294, "Race Time"), {0, kHead, 0, kFe, kFe});
    canvas.text({atRight(originX, kLapX), kHeaderY}, strings.race(0x295, "Lap Time"), {0, kHead, 0, kFe, kFe});
    canvas.text({atRight(originX, kWinsX), kHeaderY}, strings.race(0x429, "Wins"), {0, kHead, 2, kFe, kFe});
    size_t row = 0;
    for (int racer : sources.race.resultOrder())
    {
        if (racer < sources.multiplayer.players())
        {
            drawPlayerRow(canvas, row++, racer);
        }
    }
}

void MultiplayerEndMenu::drawPlayerRow(const FeCanvas &canvas, size_t row, int player) const
{
    const float originX = canvas.originX();
    const float y = kRowY + kRowStep * float(row + 1) - menus.fontLine(0) * 0.5f;   // row 0: the headers
    const argon::GameCraft &craft = sources.race.racer(player);
    const std::u16string place = sources.race.knockedOut(player) ? std::u16string() : number(int(row) + 1) + u": ";
    const std::u16string name = place + u"[CHAR" + number(craft.pilot()) + u"] " + strings.info(0x218 + pads.padOfPlayer(player), "Player");
    const float w = painter.textWidth(name, 0, kFe), s = w > kNameWidth ? kFe * kNameWidth / w : kFe;
    canvas.text({atLeft(originX, 67.f), y}, name, {0, kText, 0, s, s});
    canvas.text({atRight(originX, kTimeX), y}, raceTimeText(player), {0, kText, 0, kFe, kFe});
    canvas.text({atRight(originX, kLapX), y}, lapTimeText(craft.bestLap() > 0.f ? craft.bestLap() : 999999.f, LapShown::Record), {0, kText, 0, kFe, kFe});
    canvas.text({atRight(originX, kWinsX), y}, number(sources.multiplayer.wins(player)), {0, kText, 2, kFe, kFe});
}

// the time as the race ended: the one left after the knock-outs "By Default" (it flies on behind the menu)
std::u16string MultiplayerEndMenu::raceTimeText(int player) const
{
    if (player == sources.multiplayer.defaultWinner())
    {
        return strings.race(0x515, "By Default");
    }
    if (sources.race.knockedOut(player) || sources.race.racer(player).laps() < sources.hudLaps)
    {
        return strings.race(0x40e, "DNF");
    }
    return lapTimeText(sources.race.raceTime(player), LapShown::Record);
}
