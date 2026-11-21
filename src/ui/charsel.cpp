// Argon: the front end's character pages (docs\frontend\race-setup.md "Character select").
// FE CHARSELECT (vtable 0x19e298: build 0x15550, enter 0x193b0, input 0x163b0, update 0x15db0, draw 0x178a0),
// FE CHAR INFO (0x19d7f0: build 0x14370, enter 0x14bf0, input 0x14a50, update 0x14f70),
// FE SHIP INFO (0x19dd48: build 0x32a30, enter 0x32ee0, input 0x32e80, update 0x33230, draw 0x326a0),
// FE COURSE SELECT (0x19de18: build 0x1cfb0, enter 0x1dfd0, input 0x1da10, update 0x1d860, draw 0x1d360).
#include "ui/charsel.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "craft/craftsim.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

using namespace menu::fe;

namespace
{
constexpr float kDegrees = 3.14159265f / 180.f;
constexpr int kCharacters = 16;

// 0x1b9798: ship weight (kg) and the 7 stats (Acceleration, Power, Turn, Aerodynamics, Track, Off Road, Water)
const int kShipStats[16][8] = {{2500, 7, 6, 6, 8, 7, 6, 5},
                               {4401, 8, 7, 4, 3, 8, 7, 6},
                               {1972, 5, 3, 10, 10, 5, 4, 4},
                               {5460, 8, 8, 3, 1, 8, 8, 6},
                               {2541, 7, 7, 6, 7, 8, 7, 6},
                               {2880, 7, 7, 7, 8, 8, 6, 7},
                               {2241, 6, 5, 5, 9, 6, 6, 5},
                               {4596, 7, 7, 4, 6, 8, 7, 6},
                               {2310, 8, 8, 6, 8, 8, 8, 7},
                               {4145, 10, 10, 4, 3, 10, 9, 7},
                               {1795, 7, 6, 10, 10, 7, 6, 5},
                               {5175, 10, 10, 3, 1, 7, 6, 5},
                               {2332, 9, 9, 6, 7, 9, 8, 8},
                               {2690, 9, 9, 7, 8, 10, 8, 8},
                               {2056, 8, 7, 5, 9, 7, 7, 7},
                               {4368, 9, 9, 4, 6, 10, 9, 8}};

// FE COURSE SELECT: the Earth's turn per row (table 0x1b9a18, degrees x / y)
const float kGlobeTurn[9][2] = {{-33, -2}, {-55, -188}, {-35, -238}, {-30, -137}, {27, -254}, {-56, -104}, {-47, -117}, {-21, 53}, {0, -106}};

void drawScrollArrows(const MenuKit &menus, const FeCanvas &canvas, bool up, bool down)
{
    const float originX = canvas.originX();
    if (up)
    {
        menus.drawScrollArrow(canvas, "BUTU", originX + 300.f, 268.f);
    }
    if (down)
    {
        menus.drawScrollArrow(canvas, "BUTD", originX + 300.f, 368.f);
    }
}

// description ticker on the character pages (0x19760): a locked character's unlock hint (fe 0xaf + id, Ka Hekili
// 0xb6); an unlocked original (0..7) whose nemesis is still locked: 0xb4 (her) / 0xb5 (his); else empty
std::u16string characterTicker(const GameStrings &strings, const PilotPicks &picks)
{
    const int id = picks.character;
    if (!picks.rules.charUnlocked(id))
    {
        return strings.frontEnd(id == 7 ? 0xb6 : 0xaf + id, "");
    }
    if (id < 8 && !picks.rules.charUnlocked(id + 8))
    {
        return strings.frontEnd((id & 1) ? 0xb5 : 0xb4, "");
    }
    return {};
}

void backToCharSelect(FeShell &shell)
{
    shell.goTo(menu::PageCharSelect);
}

// the character on FE CHARSELECT / CHAR INFO: render list +0x690c, camera 2, at its pilot's position, animated from
// when it showed (0x15db0), coming out of the background (the model fade)
FeScene characterScene(FeModels &models, int character)
{
    const int pilot = character & 15;
    FeScene scene;
    scene.camera = FeScene::Camera::Characters;
    scene.models[0] = &models.character(pilot);
    if (models.posesAnimated())
    {
        scene.clocks[0] = float(glfwGetTime() - models.characterShownAt());
    }
    scene.placement[3] = glm::vec4(FeModels::characterPosition(pilot), 1.f);
    scene.fadeInFrom = models.characterShownAt();
    return scene;
}
}  // namespace

// ---- FE CHARSELECT

void CharSelectPage::enter()
{
    shell.models().showCharacter(picks.character);
    infoX = kInfoAway;
    multiplayer.reset();
}

// 0x19990: the info rows slide in from x 1280 to 560 over 10 steps on entering (enter 0x196f2), and back out while
// the page leaves (state 7: 0x16b9f / 0x16f57 / 0x1738a)
void CharSelectPage::step()
{
    const float away = kInfoAway, shown = kInfoShown;
    std::span<float> x(&infoX, 1);
    if (shell.isSlidingOut())
    {
        menu::slideRows(x, shown, away);
    }
    else
    {
        menu::slideRows(x, away, shown);
    }
}

// a locked character is drawn with the lights off (0x15db0 -> 0xaf3d0): black
FeScene CharSelectPage::scene()
{
    if (picks.modes.multiplayerSelected())
    {
        return {};   // the players' boxes (the game's characters in them: portraits here)
    }
    FeScene scene = characterScene(shell.models(), picks.character);
    scene.lightsOff = !picks.rules.charUnlocked(picks.character);
    return scene;
}

std::vector<FeScene> CharSelectPage::boxScenes()
{
    return picks.modes.multiplayerSelected() ? multiplayer.boxScenes() : std::vector<FeScene>{};
}

std::u16string CharSelectPage::title() const
{
    return strings.frontEnd(0xa0, "Select Character");
}

std::optional<std::u16string> CharSelectPage::ticker() const
{
    if (picks.modes.multiplayerSelected())
    {
        return strings.frontEnd(0xa8, "All players: select your character to start race.");   // the players' instructions
    }
    return characterTicker(strings, picks);
}

// input (0x163b0): left / right within the row (0..7 / 8..15, wrapping), up / down switch rows, A picks an unlocked
// character, B back to FE MAIN, Y Ship Data, X Character Data (unlocked only); each slides the title and hints out
void CharSelectPage::input(const menu::MenuInput &in)
{
    if (picks.modes.multiplayerSelected())
    {
        if (!multiplayer.input())
        {
            shell.slideOut(menu::UiEvent::LeftBack);
            shell.goTo(menu::PageMain);
        }
        return;
    }
    moveCursor(in);
    if (in.b)
    {
        play(menu::UiEvent::Back);
        shell.slideOut(menu::UiEvent::LeftBack);
        shell.goTo(menu::PageMain);
        return;
    }
    if (!(in.a || in.x || in.y))
    {
        return;
    }
    if (!picks.rules.charUnlocked(picks.character))
    {
        if (in.a)
        {
            play(menu::UiEvent::Refused);   // X / Y on a locked character: nothing
        }
        return;
    }
    play(menu::UiEvent::Forward);
    shell.slideOut(menu::UiEvent::LeftForward);   // 0x15db0 cases 4 / 7: the next page 0.1 s after
    if (in.a)
    {
        pickCharacter();
    }
    else if (in.y)
    {
        shell.goTo(menu::PageShipInfo);
    }
    else if (in.x)
    {
        shell.goTo(menu::PageCharInfo);
    }
}

void CharSelectPage::moveCursor(const menu::MenuInput &in)
{
    int row = picks.character & 8, col = picks.character & 7;
    if (in.left)
    {
        col = (col + 7) & 7;
    }
    if (in.right)
    {
        col = (col + 1) & 7;
    }
    if (in.up || in.down)
    {
        row ^= 8;
    }
    if (row + col == picks.character)
    {
        return;
    }
    picks.character = row + col;
    play(menu::UiEvent::Cursor);
    shell.restartTicker();
    shell.models().showCharacter(picks.character);
}

// the game mode's next page (0x163b0): Quick Race / Time Attack FE COURSE SELECT, Tournament FE TOURNEYTYPE
void CharSelectPage::pickCharacter()
{
    picks.modes.current().characterPicked();
}

void CharSelectPage::draw(const FeCanvas &canvas)
{
    if (picks.modes.multiplayerSelected())
    {
        multiplayer.draw(canvas);
        return;
    }
    drawSelected(canvas);
    drawGrid(canvas);
    drawCharSelectHints(canvas);
}

// portrait 128 x 128 at (88, 110) (unlocked), name at (560, 110) right-aligned, 0.75 ("?" when locked); the portrait
// stands in until the 3D character is loaded (the game loads it in the background). Info (0x15550 / 0x19ba0,
// right-aligned at infoX, font 0): Trophy Level / the best trophy (0x19c48: infotext 250 + level, or fe 0xcc), Home Circuit / track
// (infotext 300 + id), its difficulty (strings 1700 + id % 8)
std::u16string CharSelectPage::trophyLevelText(int pilot) const
{
    const int level = picks.rules.profileProgress().trophyLevel(pilot);
    return level > 0 ? strings.info(250 + level - 1, "") : strings.frontEnd(0xcc, "None");
}

void CharSelectPage::drawSelected(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int id = picks.character;
    const bool open = picks.rules.charUnlocked(id);
    const bool modelShown = shell.models().characterShowing(id);
    if (open && !modelShown)
    {
        menus.drawPilotPortrait(canvas, id, {originX + 88.f, 110.f, 128.f, 128.f});
    }
    std::u16string name = open ? strings.info(200 + id, argon::pilotName(id)) : argon::widenLatin1("?");
    canvas.text({originX + 560.f, 110.f}, name, {1, kYellow, 1, 0.75f * kFe, 0.75f * kFe});   // yellow (xemu)
    if (!open)
    {
        return;
    }
    const float x = originX + infoX;
    canvas.text({x, 131.f}, strings.frontEnd(0xca, "Trophy Level:"), {0, kOrange, 1, kFe, kFe});
    canvas.text({x, 149.f}, trophyLevelText(id), {0, kText, 1, kFe, kFe});
    canvas.text({x, 169.f}, strings.frontEnd(0xcb, "Home Circuit:"), {0, kOrange, 1, kFe, kFe});
    canvas.text({x, 187.f}, strings.info(300 + id, ""), {0, kText, 1, kFe, kFe});
    canvas.text({x, 205.f}, u"(" + strings.general(1700 + (id & 7), "") + u")", {0, kText, 1, kFe, kFe});
}

// grid (0x178a0): frame panel (84, 240) 472 x 125, fill 0x80404080 / EAX 0x80804040, glow, scale 0.5; tiles 54 x 54
// at x 90 + 58 i, rows y 246 / 305; the selected one highlighted, filled 0xffa08040 (EAX 0); unlocked: the pilot
// icon (2 px bigger when selected), locked: a yellow "?" in font 1 at (x + 29, y + 27), flags 0x12 (centred both ways)
void CharSelectPage::drawGrid(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.panel({originX + 84.f, 240.f, 472.f, 125.f}, {false, true, 0xffffff00u, true, 0x80404080u, 0x80804040u, 0.5f});
    for (int i = 0; i < kCharacters; ++i)
    {
        const float x = originX + 90.f + 58.f * float(i & 7), y = (i & 8) ? 305.f : 246.f;
        const bool sel = i == picks.character;
        canvas.panel({x, y, 54.f, 54.f}, {sel, true, 0xffffff00u, sel, 0xffa08040u, 0u, 1.f});
        if (!picks.rules.charUnlocked(i))
        {
            canvas.text({x + 29.f, y + 27.f}, argon::widenLatin1("?"), {1, 0xffffff00u, 0x12, kFe, kFe});   // 0x17e95 (x + 0x1a39f8)
            continue;
        }
        const float grow = sel ? 1.f : 0.f;
        menus.drawPilotPortrait(canvas, i, {x - grow, y - grow, 54.f + 2.f * grow, 54.f + 2.f * grow});
    }
}

// hints (0x193b0): Back / Select; footers Ship Data / Character Data (hidden for a locked character)
void CharSelectPage::drawCharSelectHints(const FeCanvas &canvas) const
{
    const bool open = picks.rules.charUnlocked(picks.character);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), open ? strings.frontEnd(0x32, "Select - [BUTA]") : std::u16string()});
    if (open)
    {
        menus.drawHints(canvas,
                      {strings.frontEnd(0x37, "[BUTY] - Ship Data"), strings.frontEnd(0x38, "Character Data - [BUTX]"), kHintScale, kHintY + 24.f});
    }
}

// ---- FE CHAR INFO

void CharInfoPage::enter()
{
    firstLine = 0;
    shell.models().showCharacter(picks.character);
}

FeScene CharInfoPage::scene()
{
    return characterScene(shell.models(), picks.character);
}

std::u16string CharInfoPage::title() const
{
    return strings.frontEnd(0x168, "Background");
}

std::optional<std::u16string> CharInfoPage::ticker() const
{
    return characterTicker(strings, picks);
}

std::u16string CharInfoPage::biography() const
{
    return strings.general(1500 + picks.character * 4, "");
}

int CharInfoPage::biographyLines() const
{
    return int(menus.wrapText(biography(), 0, kFe, menus.wrapWidth()).size());
}

// input (0x14a50): B back, up / down scroll the biography by the visible lines
void CharInfoPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::Back);
        backToCharSelect(shell);
        return;
    }
    if (in.up && firstLine > 0)
    {
        firstLine = std::max(0, firstLine - kScrollLines);
    }
    if (in.down && firstLine + kLinesShown < biographyLines())
    {
        firstLine += kScrollLines;
    }
}

void CharInfoPage::draw(const FeCanvas &canvas)
{
    drawFacts(canvas);
    drawBiography(canvas);
}

// FE CHAR INFO (0x14370 / 0x14bf0): name (568, 86) font 1 x 0.75 orange; Age / Origin / Quote labels at 568 (y 131 /
// 167 / 207), values orange below (149 / 187), the quote at (62, 225): its element is 516 x 16, wrapped to 516
void CharInfoPage::drawFacts(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int id = picks.character;
    const float x = atRight(originX, kRight);
    canvas.text({x, 86.f}, strings.info(200 + id, argon::pilotName(id)), {1, kOrange, 1, 0.75f * kFe, 0.75f * kFe});
    canvas.text({x, 131.f}, strings.frontEnd(0xc8, "Age:"), {0, kText, 1, kFe, kFe});
    canvas.text({x, 149.f}, strings.general(1501 + id * 4, ""), {0, kOrange, 1, kFe, kFe});
    canvas.text({x, 167.f}, strings.frontEnd(0xc9, "Origin:"), {0, kText, 1, kFe, kFe});
    canvas.text({x, 187.f}, strings.general(1502 + id * 4, ""), {0, kOrange, 1, kFe, kFe});
    canvas.text({x, 207.f}, strings.frontEnd(0xcd, "Quote:"), {0, kText, 1, kFe, kFe});
    auto quote = menus.wrapText(strings.general(1720 + id, ""), 0, kFe, menus.wrapWidth());
    for (size_t i = 0; i < quote.size() && i < 2; ++i)
    {
        canvas.text({atLeft(originX, 62.f), 225.f + 16.f * float(i)}, quote[i], {0, kOrange, 0, kFe, kFe});
    }
}

// biography (strings 1500 + 4 id) in a frame (52, 265) 536 x 125 from (62, 280), arrows at (300, 268 / 368); the
// arrows bracket the lines between y 280 and ~360: one font 0 line (19) apart
void CharInfoPage::drawBiography(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 52.f, 536.f), 265.f, boxWidth(originX, 52.f, 536.f), 125.f});
    auto lines = menus.wrapText(biography(), 0, kFe, menus.wrapWidth());
    const float lh = painter.font(0).ok ? painter.font(0).line : 19.f;
    for (int i = 0; i < kLinesShown && firstLine + i < int(lines.size()); ++i)
    {
        canvas.text({atLeft(originX, 62.f), 275.f + lh * float(i)}, lines[size_t(firstLine + i)], {0, kText, 0, kFe, kFe});
    }
    const bool canUp = firstLine > 0, canDown = firstLine + kLinesShown < int(lines.size());
    drawScrollArrows(menus, canvas, canUp, canDown);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), std::u16string()});
    if (canUp || canDown)
    {
        menus.drawHints(canvas, {std::u16string(), strings.frontEnd(0x39, "[BUTUFIX][BUTDFIX] - Scroll Biography"), kHintScale, kHintY + 24.f});
    }
}

// ---- FE SHIP INFO

std::u16string ShipInfoPage::title() const
{
    return strings.frontEnd(0xdc, "Ship Data");
}

std::optional<std::u16string> ShipInfoPage::ticker() const
{
    return characterTicker(strings, picks);
}

void ShipInfoPage::input(const menu::MenuInput &in)
{   // 0x32e80: B back
    if (in.b)
    {
        play(menu::UiEvent::Back);
        backToCharSelect(shell);
    }
}

void ShipInfoPage::enter()
{
    spin = 0.f;
    shell.models().showShip(picks.character);
}

// the ship: camera 2, turning, coming out of the background (FE SHIP INFO +0x1e0)
FeScene ShipInfoPage::scene()
{
    FeModels &models = shell.models();
    FeScene scene;
    scene.camera = FeScene::Camera::Characters;
    scene.models[0] = &models.ship(picks.character);
    scene.placement = shipPlacement();
    scene.fadeInFrom = models.shipShownAt();
    return scene;
}

// 0x33230: yaw / pitch / roll (0, 25 deg, 0) after a turn about y by the spin, at (-2, 6.95, -12)
glm::mat4 ShipInfoPage::shipPlacement() const
{
    glm::mat4 pose = glm::yawPitchRoll(0.f, 25.f * kDegrees, 0.f) * glm::rotate(glm::mat4(1.f), spin * kDegrees, glm::vec3(0, 1, 0));
    pose[3] = glm::vec4(-2.f, 6.95f, -12.f, 1.f);
    return pose;
}

void ShipInfoPage::draw(const FeCanvas &canvas)
{
    drawStatBars(canvas);
    drawNameAndPortrait(canvas);
    drawShipFacts(canvas);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), std::u16string()});
}

// FE SHIP INFO (0x326a0): 7 stat bars (0xc2420: black 200 x 24 panel at x 368, orange 0xffff8040 fill of stat x 10 %
// from the right), labels right-aligned at 560 (0.8), rows from y 100 (+28, +44 after Turn and Aerodynamics)
void ShipInfoPage::drawStatBars(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float x = atRight(originX, 368.f);
    float y = 100.f;
    for (int i = 0; i < 7; ++i)
    {
        canvas.panel({x, y, 200.f, 24.f}, {false, false, 0xffffff00u, true, 0xff000000u, 0xff000000u, 1.f});
        float fill = float(kShipStats[picks.character][1 + i]) * 10.f * 0.01f * 200.f;
        canvas.panel({x + 200.f - fill, y, fill, 24.f}, {false, false, 0xffffff00u, true, 0xffff8040u, 0xffff8040u, 1.f});
        canvas.text({atRight(originX, 560.f), y + 4.f}, strings.frontEnd(0xe0 + i, ""), {0, kText, 1, 0.8f * kFe, 0.8f * kFe});
        y += (i == 2 || i == 3) ? 44.f : 28.f;
    }
}

// name at (62, 100) (0.75, shrunk to 300 wide), portrait 64 x 64 in a frame at (62, 130)
void ShipInfoPage::drawNameAndPortrait(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int id = picks.character;
    std::u16string name = strings.info(200 + id, argon::pilotName(id));
    float scale = 0.75f, w = painter.textWidth(name, 1, scale);
    if (w > 300.f)
    {
        scale *= 300.f / w;
    }
    canvas.text({atLeft(originX, 62.f), 100.f}, name, {1, kYellow, 0, scale * kFe, scale * kFe});   // yellow (xemu)
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, 62.f, 64.f), 130.f, boxWidth(originX, 62.f, 64.f), 64.f});
    menus.drawPilotPortrait(canvas, id, {atLeft(originX, 62.f), 130.f, 64.f, 64.f});
}

// Ship Name / Weapons / Weight: panels (62, 309 + 20 i) 120 x 17 fill 0x60404070, labels at (70, 310 + 20 i) 0.8,
// values orange at (190, 308.5 + 20 i) 0.5 x 0.55 (ship name 1564 + id, weapons 1503 + 4 id, weight kg: xemu)
void ShipInfoPage::drawShipFacts(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int id = picks.character;
    const std::u16string values[3] = {strings.general(1564 + id, ""),
                                      strings.general(1503 + id * 4, ""),
                                      argon::widenLatin1(std::to_string(kShipStats[id][0]).c_str()) + strings.frontEnd(0xe7, "kg")};
    for (int i = 0; i < 3; ++i)
    {
        float y = 309.f + 20.f * float(i);
        canvas.panel({atLeft(originX, 62.f), y, 120.f, 17.f}, {false, false, 0xffffff00u, true, 0x60404070u, 0x60404070u, 1.f});
        canvas.text({atLeft(originX, 70.f), y + 1.f}, strings.frontEnd(0xdd + i, ""), {0, kText, 0, 0.8f * kFe, 0.8f * kFe});
        canvas.text({atLeft(originX, 190.f), y - 0.5f}, values[i], {1, kOrange, 0, 0.5f * kFe, 0.55f * kFe});
    }
}

// ---- FE COURSE SELECT

// enter (0x1dfd0): the rows slide in from x 640 (0x1e6a0), the Earth turned to the selected track at once;
// frontend\earth and frontend\mars (shell +0x4a68 / +0x5078, 0x30e0d)
void CourseSelectPage::enter()
{
    shell.models().loadGlobes();
    choosingRoute = false;
    route = 0;
    for (float &x : rowX)
    {
        x = kRowFrom;
    }
    turnGlobeToRow();
    globeTurn = globeTarget;
}

std::u16string CourseSelectPage::title() const
{
    return strings.frontEnd(0x17c, "Select Track");
}

// 0x1e380: empty, but locked Draco: fe 0x183, locked Mars: 0x184
std::optional<std::u16string> CourseSelectPage::ticker() const
{
    if (picks.track == 7 && !rowIsOpen(7))
    {
        return strings.frontEnd(0x183, "");
    }
    if (picks.track == kMarsRow && !rowIsOpen(kMarsRow))
    {
        return strings.frontEnd(0x184, "");
    }
    return std::u16string();
}

bool CourseSelectPage::rowIsOpen(int row) const
{
    return row == kMarsRow ? picks.progress.mars : ((picks.progress.tracks >> row) & 1) != 0;
}

bool CourseSelectPage::hasSecondRoute(int row) const
{   // the second route unlocked (bit + 8; Mars always)
    return row == kMarsRow || ((picks.progress.tracks >> (row + 8)) & 1) != 0;
}

void CourseSelectPage::turnGlobeToRow()
{
    globeTarget = glm::vec2(kGlobeTurn[picks.track][0], kGlobeTurn[picks.track][1]) * kDegrees;
}

// 0x1d360: the Earth eases 0.2 of the way to the selected track's turn each frame; the rows slide between x 640
// and 62 in 10 steps (0x1e6a0: in on entering, out while leaving, 0x1e920)
void CourseSelectPage::step()
{
    globeTurn += (globeTarget - globeTurn) * 0.2f;
    if (shell.isSlidingOut())
    {
        menu::slideRows(rowX, kRowX, kRowFrom);
    }
    else
    {
        menu::slideRows(rowX, kRowFrom, kRowX);
    }
}

// 0x1d360: tracks: the Earth, rotation Y (turn y) then X (turn x), x 0.9, at (-8, 0, 40) in the main menu camera;
// Mars (row 8): the Mars model tilted -25 deg about X after a turn about Y of the clock (1 rad a second)
glm::mat4 CourseSelectPage::globePlacement() const
{
    glm::mat4 pose;
    if (picks.track == kMarsRow)
    {
        pose = glm::rotate(glm::mat4(1.f), -25.f * kDegrees, glm::vec3(1, 0, 0)) * glm::rotate(glm::mat4(1.f), float(glfwGetTime()), glm::vec3(0, 1, 0));
    }
    else
    {
        pose = glm::rotate(glm::mat4(1.f), globeTurn.x, glm::vec3(1, 0, 0)) * glm::rotate(glm::mat4(1.f), globeTurn.y, glm::vec3(0, 1, 0)) *
            glm::scale(glm::mat4(1.f), glm::vec3(0.9f));
    }
    pose[3] = glm::vec4(-8.f, 0.f, 40.f, 1.f);
    return pose;
}

// the globe in camera 1 (as the main menu items), light 0 along (-1, -1, 1) (0x1d360); a locked Mars shows nothing
FeScene CourseSelectPage::scene()
{
    FeScene scene;
    const bool mars = picks.track == kMarsRow;
    if (mars && !rowIsOpen(kMarsRow))
    {
        return scene;
    }
    FeModels &models = shell.models();
    scene.models[0] = mars ? &models.mars() : &models.earth();
    scene.placement = globePlacement();
    scene.lightDir = glm::vec3(-1.f, -1.f, 1.f);
    return scene;
}

// 0x1da10: B closes the route choice, else back to FE CHARSELECT; A on a locked row does nothing, on a track whose
// second route is unlocked it opens the choice "1: <x>'s Route" / "2: <y>'s Route" (Mars: Martian / Terraformed),
// else picks the track -> FE SPEEDSELECT. Y on an open track (not Mars) opens FE TRACK INFO for its first route.
void CourseSelectPage::input(const menu::MenuInput &in)
{
    if (choosingRoute)
    {
        chooseRoute(in);
        return;
    }
    if (in.b)
    {
        play(menu::UiEvent::Back);
        shell.slideOut(menu::UiEvent::LeftBack);
        backToCharSelect(shell);
        return;
    }
    int row = picks.track;
    MenuKit::stepCursor(row, kRows, in);
    if (row != picks.track)
    {
        play(menu::UiEvent::Cursor);
        picks.track = row;
        shell.restartTicker();
        turnGlobeToRow();
    }
    if (in.a)
    {
        pickRow();
    }
    if (in.y && picks.track < kMarsRow && rowIsOpen(picks.track))
    {
        play(menu::UiEvent::Forward);
        shell.slideOut(menu::UiEvent::LeftForward);
        shell.trackInfo().open(picks.track);
    }
}

void CourseSelectPage::chooseRoute(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::Cancel);
        choosingRoute = false;
        return;
    }
    if (in.up || in.down)
    {
        play(menu::UiEvent::Cursor);
        route ^= 1;
    }
    if (!in.a)
    {
        return;
    }
    leaveForSpeedSelect(pickedTrack());
}

void CourseSelectPage::pickRow()
{
    if (!rowIsOpen(picks.track))
    {
        play(menu::UiEvent::Refused);
        return;
    }
    if (!hasSecondRoute(picks.track))
    {
        leaveForSpeedSelect(picks.track);
        return;
    }
    openRouteChoice();
}

void CourseSelectPage::openRouteChoice()
{
    play(menu::UiEvent::Pick);
    choosingRoute = true;
    route = picks.modes.current().routeChoiceDefault(picks.track);
}

// 0x1e920: the sound, the rows sliding out, FE SPEEDSELECT
void CourseSelectPage::leaveForSpeedSelect(int track)
{
    play(menu::UiEvent::Forward);
    shell.slideOut(menu::UiEvent::LeftForward);
    shell.speedSelect().open(picks.character, track);
}

int CourseSelectPage::pickedTrack() const
{
    return picks.track == kMarsRow ? 0x10 + route : picks.track + 8 * route;
}

void CourseSelectPage::draw(const FeCanvas &canvas)
{
    drawRows(canvas);
    if (choosingRoute)
    {
        drawRouteChoice(canvas);
    }
    if (picks.track < kMarsRow)
    {
        drawGlobeBrackets(canvas);
    }
    // hints (0x1dfd0): Back / Select, footer [Y] Track Data (hidden for Mars and while choosing a route)
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
    if (picks.track < kMarsRow && !choosingRoute)
    {
        menus.drawHints(canvas, {strings.frontEnd(0x3b, "[BUTY] - Track Data"), std::u16string(), kHintScale, kHintY + 24.f});
    }
}

// FE COURSE SELECT (0x1cfb0 / 0x1dfd0): 9 rows at (62, 100 + 30 i), scale 0.6, chamfer panels as the main menu's;
// names infotext 300 + i, Mars fe 0x182 (locked: 0x17f "????"); unlocked 0xff85dde5, locked 0xff555555. The track
// rows stay filled while their route choice is open, and a locked row does not blink: drawn here
void CourseSelectPage::drawRows(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float scale = 0.6f * kFe, line = menus.fontLine(1), h = line * 0.6f;
    for (int i = 0; i < kRows; ++i)
    {
        const bool open = rowIsOpen(i);
        std::u16string text =
            i == kMarsRow ? (open ? strings.frontEnd(0x182, "Mars") : strings.frontEnd(0x17f, "????????????????")) : strings.info(300 + i, "");
        const float w = painter.textWidth(text, 1, scale), x = atLeft(originX, rowX[i]), y = 100.f + 30.f * float(i);
        const bool sel = i == picks.track && !choosingRoute;
        canvas.panel({x - h * 0.5f, y, w + h, h}, {sel, true, 0xffffff00u, sel || (i == picks.track && choosingRoute), 0xffa08040u, 0xffa08040u});
        uint32_t colour = open ? kText : kGrey;
        if (sel && open)
        {
            colour = menu::blinkColour(colour, MenuKit::menuBlink());
        }
        canvas.text({x, y + (h - line * scale) * 0.5f}, text, {1, colour, 0, scale, scale});
    }
}

// the route buttons (build 0x1cfb0: elements +0x162c / +0x1838) right-aligned at 568, y 300 / 330, font 0 at 1.0,
// each with a panel (flag 2); strings 900 + i / 0x38c + i, Mars 0x394 / 0x395 (0x1e4d0)
void CourseSelectPage::drawRouteChoice(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int row = picks.track;
    for (int r = 0; r < 2; ++r)
    {
        int id = row == kMarsRow ? 0x394 + r : (r == 0 ? 900 + row : 0x38c + row);
        menus.drawMenuRow(canvas, {atRight(originX, kRight), r == 0 ? 300.f : 330.f}, strings.frontEnd(id, ""), {0, kFe, menus.fontLine(0), r == route, kText, 1});
    }
}

// 0x1d360: corner brackets around the Earth's centre for a track (not Mars): yellow 0xd0ffff00 (413..433, 228..248,
// 5 px arms) over dark 0xd0202000 (411..435, 226..250, 6 px)
void CourseSelectPage::drawGlobeBrackets(const FeCanvas &canvas) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    struct Bracket
    {
        float box[4];
        uint32_t colour;
        float arm;
    };
    const Bracket passes[2] = {{{411, 226, 435, 250}, 0xd0202000u, 6.f}, {{413, 228, 433, 248}, 0xd0ffff00u, 5.f}};
    for (const Bracket &bracket : passes)
    {
        for (int cx = 0; cx < 2; ++cx)
        {
            for (int cy = 0; cy < 2; ++cy)
            {
                float x = cx ? bracket.box[2] : bracket.box[0], y = cy ? bracket.box[3] : bracket.box[1];
                float dx = cx ? -bracket.arm : bracket.arm, dy = cy ? -bracket.arm : bracket.arm;
                ImVec2 corner((originX + x) * pixelsPerUnit, y * pixelsPerUnit);
                drawList->AddLine(corner, ImVec2((originX + x + dx) * pixelsPerUnit, y * pixelsPerUnit), imColour(bracket.colour), pixelsPerUnit);
                drawList->AddLine(corner, ImVec2((originX + x) * pixelsPerUnit, (y + dy) * pixelsPerUnit), imColour(bracket.colour), pixelsPerUnit);
            }
        }
    }
}
