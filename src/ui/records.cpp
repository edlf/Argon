// Argon: the Options record pages (docs\frontend\options.md "Records"): FE FASTESTLAP as Track Records (vtable 0x19e028:
// build 0x1f9f0, enter 0x20fa0, input 0x20a60, update 0x206c0, rows 0x1fef0, time 0x1fe10, frames 0x205f0), FE HIGHSCORES
// (0x19e230: build 0x24fb0, enter 0x26490, input 0x25f20, update 0x25b20, rows / frames 0x254f0) and FE GHOSTMANAGE
// (0x19dad0: build 0x215f0, enter 0x21c80, input 0x22000, frames 0x21f10). Track Records shows the lap records
// (app_laprecords.cpp), High Scores the profile's scores, Ghost Data the saved ghosts (app_ghosts.cpp).
#include "ui/records.hpp"
#include "ui/lap_time_text.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "logic/lap_record_rules.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

namespace
{
constexpr int kSpeeds = 5, kCharacters = 16;
constexpr int kTrackRecordsRow = 2, kHighScoresRow = 3, kGhostDataRow = 4;   // their FE OPTIONS rows
}  // namespace

// ---- the record tables

RecordTablePage::RecordTablePage(const PageKit &kit, const RecordSources &sources, const Place &place)
    : FePage(kit), sources(sources), page(place.page), optionsRow(place.optionsRow)
{
}

void RecordTablePage::openTable()
{
    secondRoutes = false;
    shell.goTo(page);
}

int RecordTablePage::trackOfRow(int row) const
{
    if (row < 8)
    {
        return row + (secondRoutes ? 8 : 0);
    }
    return secondRoutes ? 0x11 : 0x10;
}

// input: B back to FE OPTIONS (the select sound, then the switch's); up / down the first / second routes; left /
// right the page's column choice
void RecordTablePage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::Back);
        play(menu::UiEvent::LeftBack);
        shell.options().openAt(optionsRow);
        return;
    }
    moveAround(in);
}

// each press plays the cursor sound: both choices wrap
void RecordTablePage::moveAround(const menu::MenuInput &in)
{
    if (in.up || in.down || in.left || in.right)
    {
        play(menu::UiEvent::Cursor);
    }
    if (in.up || in.down)
    {
        secondRoutes = !secondRoutes;
    }
    changeColumn(in);
}

// the switch to FE MAIN after a race (0x206c0 case 7)
void RecordTablePage::leaveForMain()
{
    play(menu::UiEvent::LeftBack);
    shell.goTo(menu::PageMain);
}

uint32_t RecordTablePage::rowColour(int) const
{
    return kText;
}

// rows at (55, 180 + 16 i): tracks 0..7 (or their second routes) and Mars (infotext 0x13c / 0x13d, "????" fe 0x17f
// while locked)
void RecordTablePage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    drawFrames(canvas);
    drawHeaders(canvas);
    for (int i = 0; i < kTrackRows; ++i)
    {
        float y = 180.f + 16.f * float(i);
        canvas.text({originX + 55.f, y}, trackName(i), {0, rowColour(trackOfRow(i)), 0, kFe, kFe});
        drawCells(canvas, y, trackOfRow(i));
    }
    drawPageHints(canvas);
}

void RecordTablePage::drawPageHints(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x4a, "[BUTUFIX][BUTDFIX] - More Tracks")});
    canvas.text({atLeft(originX, 62.f), kHintY + 24.f}, leftRightHint(), {0, kText, 0, kHintScale, kHintScale});
}

std::u16string RecordTablePage::trackName(int row) const
{
    if (row < 8)
    {
        return strings.info(300 + trackOfRow(row), "");
    }
    return sources.progress.mars ? strings.info(secondRoutes ? 0x13d : 0x13c, "Mars") : strings.frontEnd(0x17f, "????????????????");
}

// the column frames (0x205f0 / 0x254f0): header row y 155 x 25 (swapped colours), body y 180 x 160; columns x 50
// (230 wide), 280 (the middle one), and the last after it, 310 wide together, touching (xemu)
void RecordTablePage::drawFrames(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float middle = middleColumnWidth();
    const float xs[3] = {50.f, 280.f, 280.f + middle}, ws[3] = {230.f, middle, 310.f - middle};
    for (int c = 0; c < 3; ++c)
    {
        MenuKit::drawGradientFrame(canvas, {originX + xs[c], 155.f, ws[c], 25.f}, FrameShade::Red);
        MenuKit::drawGradientFrame(canvas, {originX + xs[c], 180.f, ws[c], 160.f});
    }
}

// ---- Track Records

TrackRecordsPage::TrackRecordsPage(const PageKit &kit, const RecordSources &sources) : RecordTablePage(kit, sources, {menu::PageTrackRecords, kTrackRecordsRow})
{
}

void TrackRecordsPage::open()
{
    speed = std::clamp(sources.speedClass, 0, kSpeeds - 1);
    openTable();
}

void TrackRecordsPage::openAfterRace()
{
    open();
    secondRoutes = savesNewRecords() && argon::laps::opensOnSecondRoutes(sources.newLapRecords);
}

bool TrackRecordsPage::savesNewRecords() const
{
    for (bool fell : sources.newLapRecords)
    {
        if (fell)
        {
            return true;
        }
    }
    return false;
}

// input 0x20a60: with new records A (Continue) saves them and B does nothing; else B back to FE OPTIONS; both move
// as the other tables
void TrackRecordsPage::input(const menu::MenuInput &in)
{
    if (!savesNewRecords())
    {
        RecordTablePage::input(in);
        return;
    }
    if (in.a)
    {
        play(menu::UiEvent::Pick);
        continueAfterNewRecords();
        return;
    }
    moveAround(in);
}

// 0x20ab0: the new-record flags cleared; with a profile the "Saving" box (kind 4) first (none without one, 0x459a8c,
// or with new high scores, 0x52ac15: the viewer keeps none), then FE MAIN
void TrackRecordsPage::continueAfterNewRecords()
{
    sources.newLapRecords.fill(false);
    if (!sources.profiles.hasCurrent())
    {
        leaveForMain();
        return;
    }
    shell.messageBox().open(std::u16string(), MessageBoxPage::Kind::Saving, *this);
}

// enter 0x20fa0 after the box (+0x414d): the table saved, FE MAIN
void TrackRecordsPage::messageClosed(bool)
{
    sources.lapRecordStore.storeLapRecords();
    leaveForMain();
}

void TrackRecordsPage::step()
{
    flash = menu::stepFlash(flash);
}

// 0x206c0: on the race's speed a new record's row (all three columns) flashes grey
uint32_t TrackRecordsPage::rowColour(int track) const
{
    if (speed != sources.speedClass || !sources.newLapRecords[size_t(track)])
    {
        return kText;
    }
    return 0xff000000u | uint32_t(flash) * 0x10101u;
}

// enter 0x20fa0 with new records: More Speeds (fe 0x49) left and Continue (0x40) right in the header hints, More
// Tracks (0x4a) right in the footer
void TrackRecordsPage::drawPageHints(const FeCanvas &canvas) const
{
    if (!savesNewRecords())
    {
        RecordTablePage::drawPageHints(canvas);
        return;
    }
    menus.drawHints(canvas, {leftRightHint(), strings.frontEnd(0x40, "Continue - [BUTA]")});
    menus.drawHints(canvas, {std::u16string(), strings.frontEnd(0x4a, "[BUTUFIX][BUTDFIX] - More Tracks"), kHintScale, kHintY + 24.f});
}

std::u16string TrackRecordsPage::title() const
{
    return strings.frontEnd(0x208, "Track Records");
}

std::optional<std::u16string> TrackRecordsPage::ticker() const
{
    return strings.frontEnd(0x209, "DPad L & R changes speed");
}

void TrackRecordsPage::changeColumn(const menu::MenuInput &in)
{   // the speed class, wrapping
    if (in.left)
    {
        speed = (speed + kSpeeds - 1) % kSpeeds;
    }
    if (in.right)
    {
        speed = (speed + 1) % kSpeeds;
    }
}

// 0x1f9f0 / 0x1fef0: the speed class (infotext 0xfa + speed) right at 568, y 106, orange; headers "Track" (0x3c1) /
// "Fastest Time" (0x20a) / "Profile" (0x20b) centred at 165 / 350 / 510, y 160
void TrackRecordsPage::drawHeaders(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.text({atRight(originX, kRight), 106.f}, strings.info(0xfa + speed, ""), {0, kOrange, 1, kFe, kFe});
    canvas.text({originX + 165.f, 160.f}, strings.frontEnd(0x3c1, "Track"), {0, kOrange, 2, kFe, kFe});
    canvas.text({originX + 350.f, 160.f}, strings.frontEnd(0x20a, "Fastest Time"), {0, kOrange, 2, kFe, kFe});
    canvas.text({originX + 510.f, 160.f}, strings.frontEnd(0x20b, "Profile"), {0, kOrange, 2, kFe, kFe});
}

// the record (fe_fastestlap_row_time 0x1fe10: table +4, the name +8); none: "--:--.--" and "--------" (fe 0x20c)
void TrackRecordsPage::drawCells(const FeCanvas &canvas, float y, int track) const
{
    const float originX = canvas.originX();
    const LapRecord &record = sources.lapRecords.at(track, speed);
    const uint32_t colour = rowColour(track);
    canvas.text({originX + 350.f, y}, record.set() ? lapTimeText(record.time, LapShown::Record) : argon::widenLatin1("--:--.--"), {0, colour, 2, kFe, kFe});
    canvas.text({originX + 508.f, y}, record.set() ? record.name : strings.frontEnd(0x20c, "--------"), {0, colour, 2, kFe, kFe});
}

std::u16string TrackRecordsPage::leftRightHint() const
{
    return strings.frontEnd(0x49, "[BUTLFIX][BUTRFIX] - More Speeds");
}

// ---- High Scores

HighScoresPage::HighScoresPage(const PageKit &kit, const RecordSources &sources) : RecordTablePage(kit, sources, {menu::PageHighScores, kHighScoresRow})
{
}

void HighScoresPage::open()
{
    character = std::clamp(sources.character, 0, kCharacters - 1);
    if (!sources.rules.charUnlocked(character))
    {
        character = 0;
    }
    openTable();
}

std::u16string HighScoresPage::title() const
{
    return strings.frontEnd(0x3c0, "High Scores");
}

void HighScoresPage::changeColumn(const menu::MenuInput &in)
{   // the next unlocked character, wrapping
    if (!in.left && !in.right)
    {
        return;
    }
    const int step = in.right ? 1 : kCharacters - 1;
    for (int t = 0; t < kCharacters; ++t)
    {
        character = (character + step) % kCharacters;
        if (sources.rules.charUnlocked(character))
        {
            return;
        }
    }
}

// 0x24fb0: the character (infotext 200 + id, font 1 (xemu)) right at 568, y 96, orange; headers "Track" at 55,
// "Tournament" (0xf0) / "Quick Race" (0x7b) centred at 360 / 515, y 160
void HighScoresPage::drawHeaders(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.text({atRight(originX, kRight), 96.f}, strings.info(200 + character, argon::pilotName(character)), {1, kOrange, 1, kFe, kFe});
    canvas.text({originX + 55.f, 160.f}, strings.frontEnd(0x3c1, "Track"), {0, kOrange, 0, kFe, kFe});
    canvas.text({originX + 360.f, 160.f}, strings.frontEnd(0xf0, "Tournament"), {0, kOrange, 2, kFe, kFe});
    canvas.text({originX + 515.f, 160.f}, strings.frontEnd(0x7b, "Quick Race"), {0, kOrange, 2, kFe, kFe});
}

void HighScoresPage::drawCells(const FeCanvas &canvas, float y, int track) const
{
    const float originX = canvas.originX();
    canvas.text({originX + 360.f, y}, scoreText(sources.progress.hiTourney[character][track]), {0, kText, 2, kFe, kFe});
    canvas.text({originX + 515.f, y}, scoreText(sources.progress.hiQuick[character][track]), {0, kText, 2, kFe, kFe});
}

std::u16string HighScoresPage::scoreText(int score) const
{   // none: "n/a" (fe 0x3c3)
    return score > 0 ? argon::widenLatin1(std::to_string(score).c_str()) : strings.frontEnd(0x3c3, "n/a");
}

std::u16string HighScoresPage::leftRightHint() const
{
    return strings.frontEnd(0x4b, "[BUTLFIX][BUTRFIX] - More Characters");
}

// ---- Ghost Data (FE GHOSTMANAGE)

// enter (0x21c80): the track (0x52ccf8) moves on to the first one with ghosts (0x21d2a: 0..0x11, wrapping)
void GhostDataPage::open()
{
    sources.ghosts.scan();
    selected = firstShown = 0;
    showTrackWithGhosts(1);
    shell.goTo(menu::PageGhosts);
}

std::u16string GhostDataPage::title() const
{
    return strings.frontEnd(0x3fc, "Ghosts");
}

std::optional<std::u16string> GhostDataPage::ticker() const
{   // 0x3ff "Select ghost to delete." with ghosts, else none
    return sources.ghosts.all().empty() ? std::u16string() : strings.frontEnd(0x3ff, "Select ghost to delete.");
}

std::vector<int> GhostDataPage::ghostsOfTrack() const
{
    return sources.ghosts.ofTrack(track);
}

void GhostDataPage::showTrackWithGhosts(int step)
{
    if (sources.ghosts.all().empty())
    {
        return;
    }
    for (int tries = 0; tries < kTracks && ghostsOfTrack().empty(); ++tries)
    {
        track = (track + step + kTracks) % kTracks;
    }
}

// a row (0x223f0): the ghost's character (infotext 200 + character); the game lists unreadable ghosts as "Invalid
// ghost." (fe 0x1e0), the viewer leaves them out
std::u16string GhostDataPage::rowLabel(int ghost) const
{
    return sources.ghostKeeper.ghostLabel(sources.ghosts.all()[size_t(ghost)]);
}

// input (0x22000): B back to FE OPTIONS; up / down the ghosts (the five rows scroll); left / right the previous /
// next track with ghosts, the cursor back on top; X asks "Are you sure you want to delete ghost: <character>"
// (fe 0x3fe + the row's text) and deletes it
void GhostDataPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::LeftBack);
        shell.options().openAt(kGhostDataRow);
        return;
    }
    if (sources.ghosts.all().empty())
    {
        return;
    }
    if (in.left || in.right)
    {
        showNextTrackWithGhosts(in.right ? 1 : -1);
        return;
    }
    moveCursor(in);
    if (in.x)
    {
        askToDelete();
    }
}

// left / right (0x22000): the cursor sound only when another track has ghosts
void GhostDataPage::showNextTrackWithGhosts(int step)
{
    const int before = track;
    track = (track + step + kTracks) % kTracks;
    showTrackWithGhosts(step);
    selected = firstShown = 0;
    if (track != before)
    {
        play(menu::UiEvent::Cursor);
    }
}

void GhostDataPage::moveCursor(const menu::MenuInput &in)
{
    const int before = selected;
    if (in.up && selected > 0)
    {
        --selected;
    }
    if (in.down)
    {
        ++selected;
    }
    keepCursorInList();
    if (selected != before)
    {
        play(menu::UiEvent::Cursor);
    }
}

void GhostDataPage::askToDelete()
{
    const std::vector<int> rows = ghostsOfTrack();
    if (selected >= int(rows.size()))
    {
        return;
    }
    shell.messageBox().open(
        strings.frontEnd(0x3fe, "Are you sure you want to delete ghost: ") + rowLabel(rows[size_t(selected)]), MessageBoxPage::Kind::YesNo, *this);
}

void GhostDataPage::messageClosed(bool yes)
{
    const std::vector<int> rows = ghostsOfTrack();
    if (!yes || selected >= int(rows.size()))
    {
        return;
    }
    sources.ghosts.remove(size_t(rows[size_t(selected)]));
    showTrackWithGhosts(1);
    keepCursorInList();
}

void GhostDataPage::keepCursorInList()
{
    const int count = int(ghostsOfTrack().size());
    selected = std::clamp(selected, 0, std::max(0, count - 1));
    firstShown = std::clamp(firstShown, std::max(0, selected - (kRowsShown - 1)), selected);
}

// 0x21f10: frames (52, 100) 536 x 40 (swapped) and (52, 140) 536 x 160
void GhostDataPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 52.f, 536.f), 100.f, boxWidth(originX, 52.f, 536.f), 40.f}, FrameShade::Red);
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 52.f, 536.f), 140.f, boxWidth(originX, 52.f, 536.f), 160.f});
    if (sources.ghosts.all().empty())
    {
        drawNoGhosts(canvas);
    }
    else
    {
        drawList(canvas);
    }
}

// no ghosts: fe 0x3fd at (62, 150), 516 wide; hint Back only
void GhostDataPage::drawNoGhosts(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float line = menus.fontLine(0) * kFe;
    float y = 150.f;
    for (const auto &textLine : menus.wrapText(strings.frontEnd(0x3fd, "There are no ghosts available. Press [BUTB] to return to the Options menu."), 0, kFe, 516.f))
    {
        canvas.text({atLeft(originX, 62.f), y}, textLine, {0, kText, 0, kFe, kFe});
        y += line;
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), std::u16string()});
}

// 0x223f0: the track's name (infotext 300 + track) in the top frame (its element and font are not decoded: font 1 at
// 0.6 here, centred in the frame); five rows (font 0, the cursor's panel) at (62, 170 + 20 i); icons\up.dds at
// (82, 148) while rows are scrolled off the top, icons\down.dds at (82, 279) while more follow (as FE GHOSTSELECT's);
// hints Back, footers "[BUTLFIX][BUTRFIX] - More Tracks" (fe 0x4f) and "Delete Ghost Data - [BUTX]" (0x4c) (0x21c80)
void GhostDataPage::drawList(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    menus.drawGhostTrackName(canvas, track);
    const std::vector<int> rows = ghostsOfTrack();
    for (int r = 0; r < kRowsShown && firstShown + r < int(rows.size()); ++r)
    {
        const int row = firstShown + r;
        menus.drawMenuRow(
            canvas, {atLeft(originX, kRowX), 170.f + 20.f * float(r)}, rowLabel(rows[size_t(row)]), {0, kFe, menus.fontLine(0), row == selected, kText});
    }
    if (firstShown > 0)
    {
        menus.drawScrollArrow(canvas, "BUTU", atLeft(originX, 82.f), 148.f);
    }
    if (firstShown + kRowsShown < int(rows.size()))
    {
        menus.drawScrollArrow(canvas, "BUTD", atLeft(originX, 82.f), 279.f);
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), std::u16string()});
    menus.drawHints(
        canvas, {strings.frontEnd(0x4f, "[BUTLFIX][BUTRFIX] - More Tracks"), strings.frontEnd(0x4c, "Delete Ghost Data - [BUTX]"), kHintScale, kHintY + 24.f});
}
