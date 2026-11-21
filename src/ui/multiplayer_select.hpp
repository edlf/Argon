// Argon: FE CHARSELECT in game mode 4 (fe_charselect_input 0x163b0 / draw 0x178a0 with 0x52ccfc = 4;
// docs\frontend\multiplayer.md "Multiplayer"): each controller joins with START (A), moves its own cursor over the grid and picks;
// once every player who joined (two at least) has picked, the players are ordered by controller (0x174a0) and FE
// COURSE SELECT opens. B unpicks, or, before a pick, leaves for FE MAIN and drops every player.
#pragma once
#include "ui/fe_canvas.hpp"
#include <array>
#include <vector>

#include "ui/fe_scene.hpp"
#include "input/player_input.hpp"
#include "logic/game_progress.hpp"
#include "logic/multiplayer_session.hpp"
#include "ui/fe_page.hpp"
#include "ui/menu_common.hpp"
#include "ui/page_hosts.hpp"

class GameModes;
struct ImDrawList;

// what the character, ship and course pages read and set: the picks, the unlocks, the game mode; and for game mode 4's
// seats the ports, the session and the --mp-seats test
struct PilotPicks
{
    int &character;   // FE CHARSELECT's pick (0..15)
    int &track;       // FE COURSE SELECT's row (0..7, 8 Mars)
    const UnlockRules &rules;
    GameModes &modes;
    const argon::GameProgress &progress;
    const PlayerInput &players;
    MultiplayerSession &multiplayer;
    RaceStarter &races;
    const int &testSeats;   // --mp-seats: ports 0..n-1 joined
};

class MultiplayerSelect
{
  public:
    static const int kPads = 4;

    // owner: FE CHARSELECT, whose sounds the players' inputs play
    MultiplayerSelect(const PageKit &kit, const PilotPicks &picks, const FePage &owner)
        : strings(kit.strings), shell(kit.shell), menus(kit.menus), picks(picks), owner(owner)
    {
    }
    void reset();                // no one joined (0x52abec = -1, +0x2b0a = 0)
    bool input();                // each controller's presses; false: B left for FE MAIN
    void draw(const FeCanvas &canvas) const;
    std::vector<FeScene> boxScenes();   // each joined player's character in their box
    bool hasJoined(int pad) const
    {
        return pad >= 0 && pad < kPads && seats[size_t(pad)].joined;
    }

  private:
    struct Seat
    {
        bool joined = false, picked = false;
        int cursor = 0;            // +0x1118 + pad x 4
        int shownCharacter = -1;   // +0x1128: the character the box shows ...
        double shownAt = 0;        // ... since (+0x1148: its animation and the camera's swing)
    };
    FeScene boxScene(Seat &seat, int pad) const;
    void padInput(int pad, const menu::MenuInput &in);
    bool backOut(int pad);
    void moveCursor(Seat &s, const menu::MenuInput &in) const;   // the cursor sound on every press (0x16491 ..)
    void pick(Seat &seat);
    int joinedCount() const;
    bool everyonePicked() const;
    void startRace();
    void drawBox(const FeCanvas &canvas, int pad) const;
    void drawGrid(const FeCanvas &canvas) const;
    void drawCursorNumbers(const FeCanvas &canvas, float x, float y, int character) const;

    const GameStrings &strings;
    FeShell &shell;
    MenuKit &menus;
    PilotPicks picks;
    const FePage &owner;
    std::array<Seat, kPads> seats{};
    bool leaving = false;
};
