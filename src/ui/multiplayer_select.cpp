#include "ui/multiplayer_select.hpp"

#include <algorithm>
#include <GLFW/glfw3.h>
#include <cmath>

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"

using namespace menu::fe;

namespace
{
constexpr int kCharacters = 16;
constexpr float kBoxX = 60.f, kBoxStep = 128.f, kBoxY = 112.f, kBoxSize = 124.f;
constexpr float kDegrees = 3.14159265f / 180.f;

float tileX(int character)
{
    return 90.f + 58.f * float(character & 7);
}

float tileY(int character)
{
    return (character & 8) ? 305.f : 246.f;
}
}  // namespace

void MultiplayerSelect::reset()
{
    seats.fill(Seat{});
    leaving = false;
    for (int pad = 0; pad < std::min(picks.testSeats, kPads); ++pad)
    {
        seats[size_t(pad)].joined = true;   // tests: --mp-seats
        seats[size_t(pad)].cursor = pad * 3;
    }
}

// every controller in turn (0x163b0 reads the pad the event came from); false once B has left for FE MAIN
bool MultiplayerSelect::input()
{
    for (int pad = 0; pad < kPads && !leaving; ++pad)
    {
        padInput(pad, picks.players.portPresses(pad));
    }
    return !leaving;
}

void MultiplayerSelect::padInput(int pad, const menu::MenuInput &in)
{
    Seat &seat = seats[size_t(pad)];
    if (in.b && backOut(pad))
    {
        return;
    }
    if (!seat.joined)
    {
        if (in.a)
        {   // 0x17640: the controller takes the next player slot ("AppState - setting num players")
            seat = Seat{true, false, 0};
            owner.play(menu::UiEvent::Pick);
        }
        return;
    }
    if (!seat.picked)
    {
        moveCursor(seat, in);
        if (in.a)
        {
            pick(seat);
        }
    }
}

// B: a pick is undone (the bad-select sound); else the page leaves for FE MAIN and every player is dropped
bool MultiplayerSelect::backOut(int pad)
{
    Seat &seat = seats[size_t(pad)];
    if (seat.picked)
    {
        seat.picked = false;
        owner.play(menu::UiEvent::Cancel);
        return true;
    }
    owner.play(menu::UiEvent::Back);
    reset();
    leaving = true;
    return true;
}

// as the single player's cursor (left / right within a row, up / down switch rows), one per controller
void MultiplayerSelect::moveCursor(Seat &seat, const menu::MenuInput &in) const
{
    int row = seat.cursor & 8, col = seat.cursor & 7;
    if (in.left || in.right || in.up || in.down)
    {
        owner.play(menu::UiEvent::Cursor);
    }
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
    seat.cursor = row + col;
}

// A on an unlocked character picks it (+0x2b0a); with everyone joined picked (two at least) the race goes on
// without a sound (0x16f81 plays only for the picks before)
void MultiplayerSelect::pick(Seat &seat)
{
    if (!picks.rules.charUnlocked(seat.cursor))
    {
        owner.play(menu::UiEvent::Refused);
        return;
    }
    seat.picked = true;
    if (joinedCount() >= 2 && everyonePicked())
    {
        startRace();
        return;
    }
    owner.play(menu::UiEvent::Pick);
}

int MultiplayerSelect::joinedCount() const
{
    int count = 0;
    for (const Seat &seat : seats)
    {
        count += seat.joined ? 1 : 0;
    }
    return count;
}

bool MultiplayerSelect::everyonePicked() const
{
    for (const Seat &seat : seats)
    {
        if (seat.joined && !seat.picked)
        {
            return false;
        }
    }
    return true;
}

// 0x78d60 (the craft count = the players) and 0x174a0 (the players in controller order), then FE COURSE SELECT
void MultiplayerSelect::startRace()
{
    picks.races.startMultiplayer(joinedCount());
    int player = 0;
    for (int pad = 0; pad < kPads; ++pad)
    {
        if (seats[size_t(pad)].joined)
        {
            picks.multiplayer.join(player, seats[size_t(pad)].cursor, pad);
            ++player;
        }
    }
    picks.character = picks.multiplayer.pick(0);
    shell.slideOut(menu::UiEvent::LeftForward);
    shell.goTo(menu::PageCourseSelect);
}

// 0x178a0: a joined player's box shows their cursor's character from a camera on its skel_head bone: the eye 14
// out (Kumiko, 15: 21) and 1.4 up (2.1), swinging 90 +- 30 degrees round it with sin(time x 10 deg); fov 36 degrees
std::vector<FeScene> MultiplayerSelect::boxScenes()
{
    std::vector<FeScene> scenes;
    for (int pad = 0; pad < kPads; ++pad)
    {
        Seat &seat = seats[size_t(pad)];
        if (!seat.joined || !picks.players.portConnected(pad) || !picks.rules.charUnlocked(seat.cursor))
        {
            continue;
        }
        FeScene scene = boxScene(seat, pad);
        if (scene.models[0]->vb)
        {
            scenes.push_back(scene);
        }
    }
    return scenes;
}

FeScene MultiplayerSelect::boxScene(Seat &seat, int pad) const
{
    const double now = glfwGetTime();
    if (seat.shownCharacter != seat.cursor)
    {
        seat.shownCharacter = seat.cursor;
        seat.shownAt = now;
    }
    const float age = float(now - seat.shownAt);
    const bool kumiko = seat.cursor == 15;
    const float radius = kumiko ? 21.f : 14.f, up = kumiko ? 2.1f : 1.4f;
    const float angle = (std::sin(age * 10.f * kDegrees) * 30.f + 90.f) * kDegrees;
    FeScene scene;
    scene.models[0] = &shell.models().character(seat.cursor);
    scene.clocks[0] = age;
    scene.boxed = true;
    // the shell's 640 x 480 frame centred in the window (FeShell::draw), in framebuffer pixels
    const ImGuiIO &io = ImGui::GetIO();
    const float pixelsPerUnit = io.DisplaySize.y / 480.f, originX = (io.DisplaySize.x / pixelsPerUnit - 640.f) * 0.5f, px = pixelsPerUnit * io.DisplayFramebufferScale.y;
    scene.box.x = (originX + kBoxX + kBoxStep * float(pad) + 2.f) * px;
    scene.box.y = kBoxY * px;
    scene.box.w = scene.box.h = kBoxSize * px;
    scene.box.eyeOffset = glm::vec3(std::cos(angle) * radius, up, std::sin(angle) * radius);
    scene.box.fovY = 3.14159265f * 0.2f;
    return scene;
}

void MultiplayerSelect::draw(const FeCanvas &canvas) const
{
    ImDrawList *drawList = canvas.drawList();
    for (int pad = 0; pad < kPads; ++pad)
    {
        drawBox(canvas, pad);
    }
    shell.drawBoxModels(drawList);   // the players' characters over their boxes
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    drawGrid(canvas);
}

// 0x178a0 with mode 4: a gradient frame per controller (124 x 124 at y 112) with "Please connect a controller" (fe
// 0xab), "Press START to join!" (0xa7) or the player's pick (the game shows the 3D character; the portrait here)
void MultiplayerSelect::drawBox(const FeCanvas &canvas, int pad) const
{
    const float originX = canvas.originX();
    const Seat &seat = seats[size_t(pad)];
    const bool connected = picks.players.portConnected(pad);
    const float x = originX + kBoxX + kBoxStep * float(pad);
    MenuKit::drawGradientFrame(canvas, {x + 2.f, kBoxY, kBoxSize, kBoxSize}, connected && seat.joined ? FrameShade::Red : FrameShade::Blue);
    if (!connected || !seat.joined)
    {
        const std::u16string text = connected ? strings.frontEnd(0xa7, "Press START to join!") : strings.frontEnd(0xab, "Please connect a controller");
        const std::vector<std::u16string> lines = menus.wrapText(text, 0, kFe, 116.f);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            canvas.text({x + 64.f, 160.f + 18.f * float(i)}, lines[i], {0, kText, 2, kFe, kFe});
        }
        return;
    }
    if (!shell.models().characterLoaded(seat.cursor))
    {
        menus.drawPilotPortrait(canvas, seat.cursor, {x + 2.f, kBoxY, kBoxSize, kBoxSize});   // until the model is up
    }
}

// the grid as the single player's; a tile any player is on is highlighted, filled once all of them on it picked, each
// player's number at a corner (1 top left, 2 top right, 3 bottom left, 4 bottom right)
void MultiplayerSelect::drawGrid(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.panel({originX + 84.f, 240.f, 472.f, 125.f}, {false, true, 0xffffff00u, true, 0x80404080u, 0x80804040u, 0.5f});
    for (int c = 0; c < kCharacters; ++c)
    {
        bool on = false, allPicked = true;
        for (const Seat &seat : seats)
        {
            if (seat.joined && seat.cursor == c)
            {
                on = true;
                allPicked = allPicked && seat.picked;
            }
        }
        const float x = originX + tileX(c), y = tileY(c);
        const bool filled = on && allPicked;
        canvas.panel({x, y, 54.f, 54.f}, {on, true, 0xffffff00u, filled, 0xffa08040u, 0u, 1.f});
        if (picks.rules.charUnlocked(c))
        {
            menus.drawPilotPortrait(canvas, c, {x, y, 54.f, 54.f});
        }
        else
        {
            canvas.text({x + 27.f, y + 27.f - 9.f}, argon::widenLatin1("?"), {1, kText, 2, kFe, kFe});
        }
        drawCursorNumbers(canvas, x, y, c);
    }
}

void MultiplayerSelect::drawCursorNumbers(const FeCanvas &canvas, float x, float y, int character) const
{
    static const char16_t *const kNumbers[kPads] = {u"1", u"2", u"3", u"4"};
    for (int pad = 0; pad < kPads; ++pad)
    {
        const Seat &seat = seats[size_t(pad)];
        if (!seat.joined || seat.cursor != character)
        {
            continue;
        }
        const bool right = pad & 1, bottom = pad & 2;
        canvas.text({right ? x + 50.f : x + 4.f, bottom ? y + 36.f : y + 2.f}, kNumbers[pad], {0, kYellow, right ? 1 : 0, kFe, kFe});
    }
}
