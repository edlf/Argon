// Argon: the race's score pages, SCOREMENU and POINTSMENU (docs\frontend\tournament.md "Race scores"). The scores
// themselves: app_score.cpp.
#include "ui/score.hpp"

#include "audio/audio.hpp"
#include "audio/race_sfx.hpp"
#include "audio/sound_ids.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "logic/game_logic.hpp"
#include "logic/race_scoring.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"

using namespace menu::race;

namespace
{
constexpr float kLeft = 62.f;
constexpr float kGameFrame = 1.f / 60.f;
constexpr float kCountingScale = 1.5f, kCountingY = 215.f;   // 0x95b00 while the score climbs

std::u16string number(int v)
{
    return argon::widenLatin1(std::to_string(v).c_str());
}
std::u16string times(int v)
{
    return argon::widenLatin1(("x " + std::to_string(v)).c_str());
}

void drawLine(const FeCanvas &canvas, float y, uint32_t colour, const std::u16string &label, const std::u16string &value)
{
    const float originX = canvas.originX();
    canvas.text({atLeft(originX, kLeft), y}, label, {0, colour, 0, kFe, kFe});
    canvas.text({atRight(originX, kRight), y}, value, {0, colour, 1, kFe, kFe});
}
}  // namespace

// ---- SCOREMENU: the score counts up, then A goes on (0x950be: a second of "Saving Race", the music dipping), X to
// POINTSMENU (0x52afa8 = 6, the breakdown)

void ScoreMenu::input(const menu::MenuInput &in)
{
    if (stepLeaving())
    {
        return;
    }
    if (sources.scoring.tally.counting())
    {
        if (in.a)
        {
            skipCount();
        }
        else
        {
            countScore();
        }
        return;
    }
    if (in.x)
    {
        audio.playUi(0);
        sources.raceEnd = race_end::kPointsMenu;
    }
    else if (in.a)
    {
        audio.playUi(0);
        startLeaving(argon::MusicFadeShape::Dip);
    }
}

void ScoreMenu::leave()
{
    sources.pages.showWinOrLose();
}

void ScoreMenu::skipCount()
{
    audio.raceSfx().playOnListener(argon::GenericSound::MenuSelect);
    finishCount();
}

void ScoreMenu::countScore()
{
    countClock_ += ImGui::GetIO().DeltaTime;
    while (countClock_ >= kGameFrame && sources.scoring.tally.counting())
    {
        countClock_ -= kGameFrame;
        stepCount();
    }
}

// each step chings a little higher (22050 + 2 x step Hz); reaching the score plays final_total (0x95930)
void ScoreMenu::stepCount()
{
    argon::RaceSfx &sfx = audio.raceSfx();
    const bool reached = sources.scoring.tally.step();
    sfx.setPitch(sfx.playOnListener(argon::GenericSound::Ching), sources.scoring.tally.chingPitch());
    if (reached)
    {
        sfx.playOnListener(argon::GenericSound::PerChing);
    }
}

void ScoreMenu::finishCount()
{
    sources.scoring.tally.finish();
    countClock_ = 0.f;
    audio.raceSfx().playOnListener(argon::GenericSound::PerChing);
}

void ScoreMenu::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    canvas.text({atRight(originX, kRight), 36.f}, strings.race(0x28d, "Race Scores"), {1, kText, 1, kFe, kFe});
    drawScoreLines(canvas);
    if (!sources.scoring.tally.counting())
    {
        drawCashLine(canvas);
    }
    if (leaving())
    {   // +0x26e4 shown, the hints +0x22cc / +0x24d8 hidden (0x9545b)
        menus.drawHints(canvas, {savingText(), std::u16string(), 1.f});
        return;
    }
    menus.drawHints(canvas, {std::u16string(), strings.race(0x519, "Press [BUTA] to continue."), 1.f});
    canvas.text({atRight(originX, kRight), 421.f}, strings.race(0x427, "Show score breakdown - [BUTX]"), {0, kText, 1, 1.f, 1.f}); // the game's picture
}

// init 0x94390 / fill 0x95780: "Race Score:" (0x28f) at y 100; "Points earned in race =" (0x290) y 140; "Race position
// bonus (1st) =" (0x291 with race 0x410 + place) y 160; "Race speed multiplier (Novice)" (0x292 with infotext 0xfa +
// speed) and "x %d" y 180; "Score this race =" (0x28b) y 220; "Previous High Score =" (0x28a) y 260; labels at 62,
// values right at 568; the yellow lines as the game's picture
void ScoreMenu::drawScoreLines(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.text({atLeft(originX, kLeft), 100.f}, strings.race(0x28f, "Race Score:"), {0, kYellow, 0, kFe, kFe});
    drawLine(canvas, 140.f, kText, strings.race(0x290, "Points earned in race ="), number(sources.scoring.score.points));
    drawLine(canvas,
             160.f,
             kText,
             strings.race(0x291, "Race position bonus") + u" (" + strings.race(0x410 + std::clamp(sources.scoring.score.place, 1, 6), "1st Place") + u") =",
             number(sources.scoring.score.bonus));
    drawLine(canvas,
             180.f,
             kText,
             strings.race(0x292, "Race speed multiplier") + u" (" + strings.info(0xfa + std::clamp(sources.speedClass, 0, 4), "") + u")",
             times(sources.scoring.score.speedModifier));
    drawScoreThisRace(canvas);
    drawLine(canvas, 260.f, kYellow, strings.race(0x28a, "Previous High Score ="), number(sources.scoring.score.previousHigh));
}

// y 220; while counting the climbing score x 1.5 at y 215 (0x95b00)
void ScoreMenu::drawScoreThisRace(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const std::u16string label = strings.race(0x28b, "Score this race =");
    if (!sources.scoring.tally.counting())
    {
        drawLine(canvas, 220.f, kYellow, label, number(sources.scoring.score.score));
        return;
    }
    const float scale = kFe * kCountingScale;
    canvas.text({atLeft(originX, kLeft), kCountingY}, label, {0, kYellow, 0, scale, scale});
    canvas.text({atRight(originX, kRight), kCountingY}, number(sources.scoring.tally.shown()), {0, kYellow, 1, scale, scale});
}

// the cash line centred at (320, 300) (Quick Race: the high score); the tournament's hint 0x293 at y 340
void ScoreMenu::drawCashLine(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const bool tournament = sources.modes.current().paysCash(), gained = sources.scoring.score.gain > 0;
    std::u16string line = tournament ? (gained ? strings.race(0x28c, "NEW CASH = ") + number(sources.scoring.score.gain) : strings.race(0x28e, "No New Cash"))
                                     : (gained ? strings.race(0x521, "New High Score!") : strings.race(0x522, "No New High Score"));
    canvas.text({originX + 320.f, 300.f}, line, {1, kYellow, 2, 1.f, 1.f});
    if (!tournament)
    {
        return;
    }
    float y = 340.f;
    for (const auto &textLine : menus.wrapText(strings.race(0x293, "Earn new cash by beating your previous high score."), 0, kFe, 516.f))
    {
        canvas.text({atLeft(originX, kLeft), y}, textLine, {0, kText, 0, kFe, kFe});
        y += menus.fontLine(0) * kFe;
    }
}

// ---- POINTSMENU (init 0x8bfc0, update 0x8d2c0): A goes on as SCOREMENU's (0x8d32d)

void PointsMenu::input(const menu::MenuInput &in)
{
    if (stepLeaving())
    {
        return;
    }
    if (in.a)
    {
        audio.playUi(0);
        startLeaving(argon::MusicFadeShape::Dip);
    }
}

void PointsMenu::leave()
{
    sources.pages.showWinOrLose();
}

// title "Points" (0x527); rows, labels at 62 and the points right at 568: Power-ups (0x52a) y 100, Mega-ups (0x52b) 120,
// Bonus chips (0x52c) 140, Kills (0x52e) 160, Nemesis killed (0x52f) 180, Fast lap points (0x529) 210, Airtime (0x530)
// 230, Damage caused (0x52d) 250, Position bonus (0x531) 280, Speed modifier (0x532) 300, Total (0x533) 330
void PointsMenu::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    canvas.text({atRight(originX, kRight), 36.f}, strings.race(0x527, "Points"), {1, kText, 1, kFe, kFe});
    struct Row
    {
        int str;
        const char *fallback;
        float y;
        std::u16string value;
    };
    const RacePoints &earned = sources.scoring.points;
    const Row rows[] = {{0x52a, "Power-ups", 100.f, number(earned.ofKind(9))},
                        {0x52b, "Mega-ups", 120.f, number(earned.ofKind(10))},
                        {0x52c, "Bonus chips", 140.f, number(earned.ofKind(8))},
                        {0x52e, "Kills", 160.f, number(earned.ofKind(2))},
                        {0x52f, "Nemesis killed", 180.f, number(0)},
                        {0x529, "Fast lap points", 210.f, number(earned.ofKind(4))},
                        {0x530, "Airtime", 230.f, number(earned.ofKind(6))},
                        {0x52d, "Damage caused", 250.f, number(earned.ofKind(0))},
                        {0x531, "Position bonus", 280.f, number(sources.scoring.score.bonus)},
                        {0x532, "Speed modifier", 300.f, times(sources.scoring.score.speedModifier)},
                        {0x533, "Total", 330.f, number(sources.scoring.score.score)}};
    for (const Row &row : rows)
    {
        drawLine(canvas, row.y, kText, strings.race(row.str, row.fallback), row.value);
    }
    const std::u16string hint = leaving() ? std::u16string() : strings.race(0x519, "Press [BUTA] to continue.");
    menus.drawHints(canvas, {savingText(), hint, 1.f});   // leaving: +0x3730 shown, +0x3524 hidden (0x8d4d7)
}
