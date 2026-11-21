// Argon: the race end's pages, POSTABLE "Results" and WINMENU / LOSEMENU (docs\frontend\race-end.md "Race end"). The
// finish order and the race end's steps: app_results.cpp.
#include "ui/results.hpp"

#include "audio/audio.hpp"
#include "craft/craftsim.hpp"
#include "files/string_table.hpp"
#include "logic/game_logic.hpp"
#include "logic/game_progress.hpp"
#include "logic/progress_keeper.hpp"
#include "logic/race.hpp"
#include "logic/results_rules.hpp"
#include "logic/tourney_win.hpp"
#include "saves/profile_store.hpp"
#include "ui/game_strings.hpp"
#include "ui/lap_time_text.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/race_menus.hpp"

using namespace menu::race;

namespace
{
constexpr float kTimeX = kRight - 285.f + 5.f, kLapX = kRight - 170.f + 5.f, kKillsX = kRight - 55.f + 27.5f, kHeaderY = 165.f;

// 0xb2610: the scale that fits t into w
float fitScale(const HudPainter &painter, const std::u16string &text, int font, float scale, float w)
{
    float tw = painter.textWidth(text, font, scale);
    return tw > w ? scale * w / tw : scale;
}

std::u16string number(int v)
{
    return argon::widenLatin1(std::to_string(v).c_str());
}
}  // namespace

// ---- POSTABLE

// the table's A (0x8fa90): SCOREMENU (0x52afa8 = 3), its score and cash (0x95930)
void ResultsTable::input(const menu::MenuInput &in)
{
    if (!in.a)
    {
        return;
    }
    audio.playUi(0);
    endHost.computeScore();
    sources.raceEnd = race_end::kScoreMenu;
    rows_.clear();
}

// init 0x8dbb0 / POSTABLE ON 0x8ea30: title "Results" (0x428) font 1 right-aligned at (568, 36); the place and the
// message; the table, one row per racer 30 apart (0x8f140); hint "Press [BUTA] to continue." (0x406)
void ResultsTable::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    if (rows_.empty())
    {
        fillRows();
        printRows();
    }
    canvas.text({atRight(originX, kRight), 36.f}, strings.race(0x428, "Results"), {1, kText, 1, kFe, kFe});
    drawPlace(canvas);
    drawCells(canvas, rows_.size() + 1);
    drawHeaders(canvas);
    for (size_t i = 0; i < rows_.size(); ++i)
    {
        drawRow(canvas, i);
    }
    menus.drawHints(canvas, {std::u16string(), strings.race(0x406, "Press [BUTA] to continue."), 1.f});
}

// the player's place (0x25e + place: "1st".."6th") centred at (320, 90) and "Congratulations!" (0x4c9, 1st) / "Try
// Again!" (0x410) at (320, 120): font 1, yellow (the game's picture)
void ResultsTable::drawPlace(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    int place = 1;
    for (size_t i = 0; i < rows_.size(); ++i)
    {
        if (rows_[i].racer == 0)
        {
            place = int(i) + 1;
        }
    }
    canvas.text({originX + 320.f, 90.f}, strings.race(0x25e + std::clamp(place, 1, 6), "1st"), {1, kYellow, 2, 1.f, 1.f});
    canvas.text(
        {originX + 320.f, 120.f}, sources.race.playerWon() ? strings.race(0x4c9, "Congratulations!") : strings.race(0x410, "Try Again!"), {1, kYellow, 2, 1.f, 1.f});
}

// one gradient-framed cell (0xc2310) per column, the header row (y 160, the reddish variant) and a row per racer, 30
// high; columns 62 .. 283 .. 398 .. 513 .. 568: the first from the left margin, the others with the right (xemu)
void ResultsTable::drawCells(const FeCanvas &canvas, size_t rows) const
{
    const float originX = canvas.originX();
    const float cx[5] = {atLeft(originX, 62.f), atRight(originX, kRight - 285.f), atRight(originX, kRight - 170.f), atRight(originX, kRight - 55.f), atRight(originX, kRight)};
    for (size_t row = 0; row < rows; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            MenuKit::drawGradientFrame(
                canvas, {cx[col], 160.f + 30.f * float(row), cx[col + 1] - cx[col], 30.f}, row == 0 ? FrameShade::Red : FrameShade::Blue);
        }
    }
}

// headers (orange 0xffff8000, y 160 + 5) "Position/Character" (0x26e) at 67, "Race Time" (0x294) at 568 - 285 + 5, "Lap
// Time" (0x295) at 568 - 170 + 5, "Kills" (0x41b) centred at 568 - 55 + 27.5 (columns 0x1b3cb4.. 115 / 115 / 55 wide)
void ResultsTable::drawHeaders(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const std::u16string raceTimeHeader = strings.race(0x294, "Race Time"), lapTimeHeader = strings.race(0x295, "Lap Time");
    const float ts = fitScale(painter, raceTimeHeader, 0, kFe, 108.f), ls = fitScale(painter, lapTimeHeader, 0, kFe, 108.f);
    canvas.text({atLeft(originX, 67.f), kHeaderY}, strings.race(0x26e, "Position/Character"), {0, kHead, 0, kFe, kFe});
    canvas.text({atRight(originX, kTimeX), kHeaderY}, raceTimeHeader, {0, kHead, 0, ts, ts});
    canvas.text({atRight(originX, kLapX), kHeaderY}, lapTimeHeader, {0, kHead, 0, ls, ls});
    canvas.text({atRight(originX, kKillsX), kHeaderY}, strings.race(0x41b, "Kills"), {0, kHead, 2, kFe, kFe});
}

// 0x8ea30 passes 0x8f140 true only on the frame POSTABLE ON arrives: the rows are written then and only coloured
// after, so they keep the race as it stood at the finish while the race runs on behind the table
void ResultsTable::fillRows()
{
    rows_.clear();
    std::vector<float> raceTimes;
    for (int racer : sources.race.resultOrder())
    {
        rows_.push_back(rowOf(racer));
        raceTimes.push_back(rows_.back().raceTime);
    }
    argon::results::keepPlacesInOrder(raceTimes);
    for (size_t i = 0; i < rows_.size(); ++i)
    {
        rows_[i].raceTime = raceTimes[i];
    }
}

// the game's debug line per row (0x8f874:"%d: %s race: %s best: %s")
void ResultsTable::printRows() const
{
    for (size_t i = 0; i < rows_.size(); ++i)
    {
        const Row &row = rows_[i];
        std::printf("results: %zu: racer %d race: %.2f best: %.2f%s\n", i + 1, row.racer, row.raceTime, row.lapTime, row.dnf ? " (DNF)" : "");
    }
}

// 0x8f140: a row's times from the racer's race slot (logic\results_rules.hpp); out of the race (craft +0x1145:
// knocked out, or the player at the time limit) it shows "DNF"
ResultsTable::Row ResultsTable::rowOf(int racer) const
{
    const argon::GameCraft &craft = sources.race.racer(racer);
    argon::results::RacerProgress progress;
    progress.lapsDone = craft.laps();
    progress.lapFraction = craft.lapProgress(sources.aiGraph);
    progress.lapTimeSoFar = craft.lapTime();
    progress.finishedLapsTime = sources.race.raceTime(racer);
    progress.bestLap = craft.bestLap() > 0.f ? craft.bestLap() : argon::results::kNoTime;
    const argon::results::RowTimes times = argon::results::rowTimes(progress, sources.hudLaps);
    Row row{racer, times.raceTime, times.lapTime, sources.race.kills(racer)};
    row.dnf = sources.race.knockedOut(racer);
    row.finished = craft.laps() >= sources.hudLaps;
    return row;
}

// "%d: %s" (place, pilot) or, out of the race without having finished, the pilot alone; out of the race the race
// time reads "DNF" (0x40e) and the lap time and kills cells are blank
void ResultsTable::drawRow(const FeCanvas &canvas, size_t index) const
{
    const float originX = canvas.originX();
    const Row &row = rows_[index];
    const argon::GameCraft &craft = sources.race.racer(row.racer);
    const float y = kHeaderY + 30.f * float(index + 1);
    const uint32_t colour = row.racer == 0 ? kYellow : kText;   // the player's row yellow
    const std::u16string pilot = strings.info(200 + craft.pilot(), argon::pilotName(craft.pilot()));
    const std::u16string name = argon::results::showsPlaceNumber(row.dnf, row.finished) ? number(int(index) + 1) + u": " + pilot : pilot;
    const float ns = fitScale(painter, name, 0, kFe, 210.f);
    canvas.text({atLeft(originX, 67.f), y}, name, {0, colour, 0, ns, ns});
    if (row.dnf)
    {
        canvas.text({atRight(originX, kTimeX), y}, strings.race(0x40e, "DNF"), {0, colour, 0, kFe, kFe});
        return;
    }
    canvas.text({atRight(originX, kTimeX), y}, lapTimeText(row.raceTime, LapShown::Record), {0, colour, 0, kFe, kFe});
    canvas.text({atRight(originX, kLapX), y}, lapTimeText(row.lapTime, LapShown::Record), {0, colour, 0, kFe, kFe});
    canvas.text({atRight(originX, kKillsX), y}, number(row.kills), {0, colour, 2, kFe, kFe});
}

// ---- WINMENU / LOSEMENU (init 0xa01c8 / 0x845b0; LOSEMENU update 0x84b30, items 0x85a50, confirm 0x85bf0)

void RaceEndMenu::open()
{
    confirming = false;
    selected = 0;
}

// the items: Quick Race: Race Again, View Replay, Track Select, Main Menu (both menus). Tournament (WINMENU 0xa1bd0):
// Upgrade Menu, Next Race, View Replay, Save and Quit; after the last race Upgrade Menu, View Replay, Continue;
// LOSEMENU (0x85a50): Upgrade Menu, Race Again, View Replay, Main Menu
std::vector<RaceEndMenu::Item> RaceEndMenu::items() const
{
    if (!sources.modes.current().hasUpgrades())
    {
        return {kRaceAgain, kViewReplay, kTrackSelect, kMainMenu};
    }
    if (sources.raceEnd == race_end::kWinMenu)
    {
        return sources.tourneyRun.finished ? std::vector<Item>{kUpgradeMenu, kViewReplay, kContinue}
                                           : std::vector<Item>{kUpgradeMenu, kNextRace, kViewReplay, kSaveAndQuit};
    }
    return {kUpgradeMenu, kRaceAgain, kViewReplay, kMainMenu};
}

// 0xa1cc7: Save and Quit (0x40f) is Quit (0x41e) when the game runs without a profile to save (0x459a8c: none loaded,
// or "Continue without saving." picked)
std::u16string RaceEndMenu::label(Item item) const
{
    static const int ids[8] = {0x3fa, 0x3eb, 0x3fb, 0x3fc, 0x3f3, 0x528, 0x40f, 0x3ed};
    static const char *fallbacks[8] = {"Race Again", "View Replay", "Track Select", "Main Menu", "Next Race", "Upgrade Menu", "Save and Quit", "Continue"};
    if (item == kSaveAndQuit && !sources.profiles.hasCurrent())
    {
        return strings.race(0x41e, "Quit");
    }
    return strings.race(ids[item], fallbacks[item]);
}

// the menus (0x84b30): up / down (wrapping), A runs the item; Race Again, Track Select and Main Menu ask "Are you
// sure?" (0x85bf0, the cursor on Yes); View Replay (0x3eb) plays the race's recording back (0x3e8e0). No B.
void RaceEndMenu::input(const menu::MenuInput &in)
{
    if (stepLeaving())
    {
        return;
    }
    const int rows = confirming ? 2 : int(items().size());
    if (MenuKit::stepCursor(selected, rows, in))
    {
        audio.playUi(2);
    }
    if (!in.a)
    {
        return;
    }
    if (confirming)
    {
        answerConfirm();
    }
    else
    {
        pickItem();
    }
}

void RaceEndMenu::pickItem()
{
    const Item item = items()[size_t(selected)];
    audio.playUi(0);
    if (item == kViewReplay)
    {
        endHost.startViewReplay();
        return;
    }
    if (item == kUpgradeMenu)
    {
        sources.pages.openUpgrade();
        return;
    }
    confirmRow = selected;
    confirming = true;
    selected = 0;
}

// Yes (LOSEMENU 0x85660 / WINMENU 0xa1420, cursor 5 / 6): a second of "Saving Race" with the music fading out, then
// the item runs (0x85007 / 0xa0be6)
void RaceEndMenu::answerConfirm()
{
    audio.playUi(0);
    if (selected == 1)
    {   // No
        confirming = false;
        selected = confirmRow;
        return;
    }
    startLeaving(argon::MusicFadeShape::Out);
}

void RaceEndMenu::leave()
{
    const Item action = items()[size_t(confirmRow)];
    host.resetRaceEnd();
    runAction(action);
}

void RaceEndMenu::runAction(Item action)
{
    switch (action)
    {
    case kNextRace:   // the tournament's next track through the loading screen
        endHost.openMenu();
        endHost.startQuickRace(sources.tourneyRun.character,
                               sources.progressKeeper.nextTourneyTrack(sources.progress.tourneyLevel, sources.tourneyRun.character));
        break;
    case kRaceAgain:
        host.restartRace();
        break;
    case kTrackSelect:
        host.quitRaceTo(menu::PageCourseSelect);
        break;
    default:
        host.quitRaceTo(menu::PageMain);   // Main Menu, Save and Quit, Continue
    }
}

// WINMENU (0xa01c8) / LOSEMENU (0x845b0): title "You Won" (0x3f2) / "You Lost" (0x3e8) font 1 right-aligned at (568,
// 36); the items font 1 x 0.7 centred on 320 at y 151 + 35 n; hint "Select - [BUTA]" (0x408) at (568, 421). The
// confirm page: "Are you sure?" (0x40c), Yes (0x40a), No (0x40b) from y 221. Race-side hud groups: no front-end text
// factor (as PAUSEMENU)
void RaceEndMenu::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const bool won = sources.raceEnd == race_end::kWinMenu;
    canvas.text({atRight(originX, kRight), 36.f}, won ? strings.race(0x3f2, "You Won") : strings.race(0x3e8, "You Lost"), {1, kText, 1, 1.f, 1.f});
    const float scale = 0.7f, h = menus.fontLine(1) * 0.7f;
    if (confirming)
    {
        canvas.text({originX + 320.f, 151.f}, strings.race(0x40c, "Are you sure?"), {1, kText, 2, scale, scale});
        const std::u16string answers[2] = {strings.race(0x40a, "Yes"), strings.race(0x40b, "No")};
        for (int i = 0; i < 2; ++i)
        {
            menus.drawMenuRow(canvas, {originX + 320.f, 221.f + 35.f * float(i)}, answers[i], {1, scale, h, i == selected, kText, 2});
        }
    }
    else
    {
        const std::vector<Item> shown = items();
        for (size_t i = 0; i < shown.size(); ++i)
        {
            menus.drawMenuRow(canvas, {originX + 320.f, 151.f + 35.f * float(i)}, label(shown[i]), {1, scale, h, int(i) == selected, kText, 2});
        }
    }
    const std::u16string hint = leaving() ? std::u16string() : strings.race(0x408, "Select - [BUTA]");
    menus.drawHints(canvas, {savingText(), hint, 1.f});   // leaving: +0x1454 shown, +0x1248 hidden (0x85083)
}
