// Argon: FE TOURNEYTYPE (vtable 0x19dfc0: build 0x35110, enter 0x36320, input 0x35940, update 0x35e50,
// ticker 0x365b0); docs\frontend\tournament.md "Tournament". The tournament itself: app_tourney.cpp.
#include "ui/tourney.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "logic/tourney_rules.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"

using namespace menu::fe;

namespace
{
const char *const kIconTextures[3] = {"tourn_C", "tourn_B", "tourn_A"};   // Trophy, RaceWon, RaceToGo (shell +0x1fd0..)
constexpr float kLevelScale = 0.75f;
}  // namespace

// enter (0x36320): the rows (fe 0xf1 + i: Novice, Amateur, Expert, Master, Redshift) at (62, 100 + 40 i), font 1 x
// 0.75; levels above the highest open one (0x1bbc04) grey; New / Continue hidden; the cursor on the level of the
// last tournament started (0x1bbbec, kept in the profile)
void TourneyTypePage::open()
{
    selected = std::clamp(progress.tourneyLevel, 0, kLevels - 1);
    choice = kNoChoice;
    shell.goTo(menu::PageTourneyType);
}

void TourneyTypePage::enter()
{
    for (const char *name : kIconTextures)
    {
        if (!painter.texture(name))
        {
            painter.loadTexture(name, gameFolder + "/data/frontend/" + std::string(name) + ".dds");
        }
    }
}

std::u16string TourneyTypePage::title() const
{
    return strings.frontEnd(0xf0, "Tournament");
}

bool TourneyTypePage::selectedIsOpen() const
{
    return selected <= rules.speedMax();
}

// races won so far (-1, a finished tournament, starts again)
bool TourneyTypePage::selectedHasProgress() const
{
    return progress.tourney[character][selected] > 0;
}

// input (0x35940): with New / Continue shown, B hides them (the cursor back on the level), up / down move between
// them, A picks; else B back to FE CHARSELECT (0x35e50), up / down the levels (wrapping), A on an open level
void TourneyTypePage::input(const menu::MenuInput &in)
{
    if (choice != kNoChoice)
    {
        chooseNewOrContinue(in);
        return;
    }
    if (in.b)
    {
        play(menu::UiEvent::Back);
        shell.goTo(menu::PageCharSelect);
        return;
    }
    moveLevelCursor(in);
    if (!in.a)
    {
        return;
    }
    if (!selectedIsOpen())
    {
        play(menu::UiEvent::Refused);
        return;
    }
    pickLevel();
}

void TourneyTypePage::moveLevelCursor(const menu::MenuInput &in)
{
    const int before = selected;
    menus.menuCursor(selected, kLevels, in);
    if (selected != before)
    {
        play(menu::UiEvent::Cursor);
        shell.restartTicker();   // 0x35f81: the ticker follows the cursor
    }
}

// A on an open level (0x35a6d): no race won yet (or the tournament finished) starts it from race 0; else New /
// Continue Tournament come up with the cursor on Continue
void TourneyTypePage::pickLevel()
{
    if (selectedHasProgress())
    {
        choice = kContinueTournament;
        return;
    }
    progress.tourney[character][selected] = 0;
    races.startTourneyRace(selected);
}

// 0x35a12: A on New Tournament asks first (fe 0x156, Yes / No), on Continue Tournament starts the next race
void TourneyTypePage::chooseNewOrContinue(const menu::MenuInput &in)
{
    if (in.b)
    {
        choice = kNoChoice;
        return;
    }
    if (in.up || in.down)
    {
        play(menu::UiEvent::Cursor);
        choice ^= 1;
    }
    if (!in.a)
    {
        return;
    }
    const int picked = choice;
    choice = kNoChoice;
    if (picked == kNewTournament)
    {
        shell.messageBox().open(strings.frontEnd(0x156, "Are you sure you want to start a new tournament?"), MessageBoxPage::Kind::YesNo, *this);
        return;
    }
    races.startTourneyRace(selected);
}

// enter 0x36340 with +0x1219: Yes (+0x1f4 = 1) clears the level's progress and starts it; No re-enters the page
void TourneyTypePage::messageClosed(bool yes)
{
    if (yes)
    {
        startNewTournament();
        return;
    }
    open();
}

void TourneyTypePage::startNewTournament()
{
    progress.tourney[character][selected] = 0;
    races.startTourneyRace(selected);
}

// update 0x35e50: the speedometer with its needle on the cursor's level
FeScene TourneyTypePage::scene()
{
    return shell.models().speedometerScene(selected);
}

void TourneyTypePage::draw(const FeCanvas &canvas)
{
    drawLevels(canvas);
    if (choice != kNoChoice)
    {
        drawNewOrContinue(canvas);
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), selectedIsOpen() ? strings.frontEnd(0x32, "Select - [BUTA]") : std::u16string()});
}

void TourneyTypePage::drawLevels(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float scale = kLevelScale * kFe, h = menus.fontLine(1) * kLevelScale;
    for (int i = 0; i < kLevels; ++i)
    {
        const bool asking = i == selected && choice != kNoChoice;
        const uint32_t colour = i <= rules.speedMax() || asking ? kText : kGrey;
        menus.drawMenuRow(
            canvas, {atLeft(originX, kRowX), 100.f + 40.f * float(i)}, strings.frontEnd(0xf1 + i, "Tournament"), {1, scale, h, i == selected && !asking, colour});
        drawLevelIcons(canvas, i);
    }
}

menu::TourneyRowState TourneyTypePage::rowState(int level) const
{
    const int ch = std::clamp(character, 0, 15);
    const float width = painter.textWidth(strings.frontEnd(0xf1 + level, "Tournament"), 1, kLevelScale * kFe);
    return {level, kRowX + width, progress.trophies[ch][level], progress.tourney[ch][level], argon::tourney::racesIn(level)};
}

// 0x36100: the trophy (tourn_C) once won, then a race icon per race while the tournament is under way (tourn_B won,
// tourn_A to go), white (colour -1), from the left margin
void TourneyTypePage::drawLevelIcons(const FeCanvas &canvas, int level) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    for (const menu::PlacedTourneyIcon &icon : menu::tourneyRowIcons(rowState(level)))
    {
        const std::string name = kIconTextures[int(icon.icon)];
        if (const Found<const HudTexture> texture = painter.texture(name))
        {
            const float x = atLeft(originX, icon.x);
            drawList->AddImage(texture->image, ImVec2(x * pixelsPerUnit, icon.y * pixelsPerUnit), ImVec2((x + icon.size) * pixelsPerUnit, (icon.y + icon.size) * pixelsPerUnit));
        }
    }
}

// New Tournament / Continue Tournament (font 0, right-aligned at 568, y 300 / 330), shown with races won; the panel
// only on the chosen one
void TourneyTypePage::drawNewOrContinue(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    for (int i = 0; i < 2; ++i)
    {
        menus.drawMenuRow(canvas,
                        {atRight(originX, kRight), 300.f + 30.f * float(i)},
                        strings.frontEnd(0x140 + i, i ? "Continue Tournament" : "New Tournament"),
                        {0, kFe, menus.fontLine(0), choice == i, kText, 1, false});
    }
}

// ticker (0x365b0): a locked level: its unlock hint (fe 0x116 + level); else "%s%d/%d %s %s. %s" (0x1a26a0): fe
// 0x12c "Progress: ", the races won, the race count, fe 0x12d, infotext 300 + the next track and, on the highest open
// level, fe 0x104 + level
std::optional<std::u16string> TourneyTypePage::ticker() const
{
    if (!selectedIsOpen())
    {
        return strings.frontEnd(0x116 + selected, "");
    }
    const int ch = character;
    int done = rules.tourneyDone(selected, ch);
    char counts[32];
    std::snprintf(counts, sizeof counts, "%d/%d ", done, argon::tourney::racesIn(selected));
    std::u16string text = strings.frontEnd(0x12c, "Progress: ") + argon::widenLatin1(counts) + strings.frontEnd(0x12d, "races completed. Next race:") + u" " +
                       strings.info(300 + argon::tourney::trackOf(selected, ch, done), "") + u". ";
    if (selected == rules.speedMax())
    {
        text += strings.frontEnd(0x104 + selected, "");
    }
    return text;
}
