// Argon: FE AWARDS (ui\awards.hpp; docs\frontend\tournament.md "Tournament").
#include "ui/awards.hpp"

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

namespace
{
constexpr float kFrameX = 52.f, kFrameY = 195.f, kFrameW = 536.f, kFrameH = 170.f;   // 0x13790
constexpr float kTrophyRowY = 200.f, kRowsY = 220.f, kRowStep = 20.f;                 // 0x128d0: row 0 moved to 200
constexpr float kCongratulationsY = 100.f;
const glm::vec3 kTrophyPosition(50.f, 70.f, -550.f);   // 0x13742: the trophy, rotation 0
}  // namespace

// enter (0x12e70): title "Rewards" (fe 0x1c3), the rows (0x137f0: none without a new trophy, then the page leaves at
// once), the level's trophy (objects\fe_trophy1..5), the clock of the character's animation
void AwardsPage::enter()
{
    enteredAt = glfwGetTime();
    listAwards();
    if (rows.empty())
    {
        leave();
        return;
    }
    shell.models().trophy(tourney.awards.trophy);
}

// 0x137f0's order: the trophy (fe 0x1b9 + level), Ka Hekili (0x1c2), Kutoo (0x1c0), a nemesis ("You now have access to"
// 0x1c4 + infotext 200 + id), a track ("You unlocked " 0x1be + infotext 300 + track), the level opened (0x1c6..0x1c8),
// Draco Island (0x1bf), Mars (0x1c1), an easter egg (0x1c9 + n); the first character of these is shown in 3D
void AwardsPage::listAwards()
{
    const TourneyAwards &awards = tourney.awards;
    rows.clear();
    character = -1;
    if (awards.trophy < 0)
    {
        return;
    }
    addRow(strings.frontEnd(0x1b9 + awards.trophy, "You have won the trophy!"));
    if (awards.kaHekili)
    {
        addRow(strings.frontEnd(0x1c2, "You unlocked Ka Hekili."));
        showCharacter(7);
    }
    if (awards.kutoo)
    {
        addRow(strings.frontEnd(0x1c0, "You unlocked Kutoo."));
        showCharacter(15);
    }
    if (awards.character >= 0)
    {
        addRow(strings.frontEnd(0x1c4, "You now have access to ") + strings.info(200 + awards.character, ""));
        showCharacter(awards.character);
    }
    if (awards.track >= 0)
    {
        addRow(strings.frontEnd(0x1be, "You unlocked ") + strings.info(300 + awards.track, ""));
    }
    if (awards.level >= 2)
    {
        addRow(strings.frontEnd(0x1c6 + awards.level - 2, "You have unlocked a tournament."));
    }
    if (awards.draco)
    {
        addRow(strings.frontEnd(0x1bf, "You unlocked Draco Island."));
    }
    if (awards.mars)
    {
        addRow(strings.frontEnd(0x1c1, "You unlocked the Mars tracks!"));
    }
    if (awards.easterEgg >= 0)
    {
        addRow(strings.frontEnd(0x1c9 + awards.easterEgg, "You unlocked an easter egg."));
    }
}

void AwardsPage::showCharacter(int pilot)
{
    if (character < 0)
    {
        character = pilot;
    }
}

// input (0x12b60): A or START leaves
void AwardsPage::input(const menu::MenuInput &in)
{
    if (in.a)
    {
        leave();
    }
}

// 0x133c0's leave: UI sound 3, 0x52ac00 cleared, FE MAIN
void AwardsPage::leave()
{
    tourney.awardsPending = false;
    play(menu::UiEvent::LeftForward);
    shell.goTo(menu::PageMain);
}

std::u16string AwardsPage::title() const
{
    return strings.frontEnd(0x1c3, "Rewards");
}

// the ticker (from x 640), only with an easter egg: "You can activate unlocked easter eggs ..." (fe 0x1d1)
std::optional<std::u16string> AwardsPage::ticker() const
{
    if (!awardsShowTicker(tourney.awards))
    {
        return std::nullopt;
    }
    return strings.frontEnd(0x1d1, "");
}

// "Congratulations!" (fe 0x1b8) right-aligned at (568, 100); the frame (52, 195, 536 x 170); row 0 at (62, 200), the
// others from y 220 + 20 n (row 1 at 240); "Press [BUTA] to continue" (fe 0x3c). The elements' font is not decoded
// (the text element's default, drawn here in font 0).
void AwardsPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    canvas.text({atRight(originX, kRight), kCongratulationsY}, strings.frontEnd(0x1b8, "Congratulations!"), {0, kText, 1, kFe, kFe});
    MenuKit::drawGradientFrame(canvas, {atLeft(originX, kFrameX), kFrameY, kFrameW, kFrameH});
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const float y = i == 0 ? kTrophyRowY : kRowsY + kRowStep * float(i);
        canvas.text({atLeft(originX, kRowX), y}, rows[i], {0, kText, 0, kFe, kFe});
    }
    menus.drawHints(canvas, {std::u16string(), strings.frontEnd(0x3c, "Press [BUTA] to continue")});
}

// camera 2 (0x133c0): the unlocked character at its 0x1b9a88 position, animated from the page's enter, then the trophy
// at (50, 70, -550)
FeScene AwardsPage::scene()
{
    FeModels &models = shell.models();
    FeScene scene;
    scene.camera = FeScene::Camera::Characters;
    int slot = 0;
    if (character >= 0)
    {
        scene.models[slot] = &models.character(character);
        scene.offsets[slot] = glm::translate(glm::mat4(1.f), FeModels::characterPosition(character));
        if (models.posesAnimated())
        {
            scene.clocks[slot] = float(glfwGetTime() - enteredAt);
        }
        ++slot;
    }
    if (!rows.empty())
    {
        scene.models[slot] = &models.trophy(tourney.awards.trophy);
        scene.offsets[slot] = glm::translate(glm::mat4(1.f), kTrophyPosition);
        scene.clocks[slot] = float(glfwGetTime() - enteredAt);
    }
    return scene;
}
