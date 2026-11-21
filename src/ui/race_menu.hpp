// Argon: a menu of the race (the game's game-logic hud groups PAUSEMENU, POSTABLE, SCOREMENU, POINTSMENU,
// WINMENU / LOSEMENU, UPGRADEMENU: each an object with its own vtable; docs\frontend\README.md). RaceMenus
// (ui\race_menus.hpp) owns them and runs the one shown.
#pragma once
#include "logic/player_pads.hpp"
#include "ui/fe_canvas.hpp"
#include <string>

#include "audio/music_fade.hpp"
#include "ui/menu_common.hpp"
#include "ui/race_end_page.hpp"
#include "ui/race_menu_hosts.hpp"

struct ImDrawList;
class GameStrings;
class HudPainter;
class MenuKit;
namespace argon
{
class Audio;
}

// what every race menu draws and talks with: the game's text, the menus' shared pieces, the 2D painter, the sounds
// and App (ui\race_menu_hosts.hpp)
struct RaceMenuKit
{
    RaceMenuHost &host;
    const GameStrings &strings;
    MenuKit &menus;
    HudPainter &painter;
    argon::Audio &audio;
    const PlayerPads &pads;   // the players' pads in the race
};

// the race end's pages as their menus move between them (RaceMenus)
class RaceEndPages
{
  public:
    virtual void openUpgrade() = 0;
    virtual void closeUpgrade() = 0;   // back to WINMENU / LOSEMENU
    virtual void showWinOrLose() = 0;   // WINMENU / LOSEMENU by the player's place, the mode told the race is over

  protected:
    ~RaceEndPages() = default;
};

class RaceMenu
{
  public:
    // what the menu is drawn over: the race going on behind it, blurred each frame, with the menu bars (the race end's
    // pages), or a background the menu draws itself
    enum class Background
    {
        RaceBlur,
        Own
    };

    explicit RaceMenu(const RaceMenuKit &kit)
        : host(kit.host), strings(kit.strings), menus(kit.menus), painter(kit.painter), audio(kit.audio), pads(kit.pads)
    {
    }
    virtual ~RaceMenu() = default;
    RaceMenu(const RaceMenu &) = delete;
    RaceMenu &operator=(const RaceMenu &) = delete;

    virtual void input(const menu::MenuInput &in) = 0;   // each frame, with this frame's presses
    virtual void draw(const FeCanvas &canvas) = 0;
    virtual Background background() const = 0;
    void stopLeaving();   // a new race: no leaving second left running

  protected:
    // A on SCOREMENU / POINTSMENU, Continue on UPGRADEMENU, Yes on WINMENU / LOSEMENU: a second of "Saving Race"
    // (0x520 in the hint's place, the hints hidden, no input taken) with the music fading (audio\music_fade.hpp),
    // then leave() (ui\race_menus.cpp)
    void startLeaving(argon::MusicFadeShape shape);
    bool stepLeaving();   // each frame: true while the second runs; its end runs leave()
    bool leaving() const
    {
        return leavingFade.active();
    }
    std::u16string savingText() const;   // "Saving Race" while leaving, else empty
    virtual void leave()
    {
    }

    RaceMenuHost &host;
    const GameStrings &strings;
    MenuKit &menus;
    HudPainter &painter;
    argon::Audio &audio;
    const PlayerPads &pads;

  private:
    argon::MusicFade leavingFade;
};
