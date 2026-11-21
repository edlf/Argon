// Argon: a front-end page (the game's CFEShell pages, each an object with its own vtable: build, enter,
// input, update, draw, messages; docs\frontend\README.md). The shell (ui\fe_shell.hpp) owns one per menu::Page and runs
// the current one: input each frame, step on each 60 Hz tick, draw each frame; the title and ticker go in the
// shell's own elements, the page's models (scene) in the shell's render lists.
#pragma once
#include "ui/fe_canvas.hpp"
#include <vector>
#include <cstdint>
#include <optional>
#include <string>

#include "ui/fe_scene.hpp"
#include "ui/fe_sounds.hpp"
#include "ui/menu_common.hpp"

struct ImDrawList;
class FeShell;
class GameStrings;
class HudPainter;
class MenuKit;
namespace argon
{
class Audio;
}

// what every page draws and talks with: the game's text, the shell (its pages, message box, models, ticker), the
// menus' shared pieces, the 2D painter and the sounds
struct PageKit
{
    const GameStrings &strings;
    FeShell &shell;
    MenuKit &menus;
    HudPainter &painter;
    argon::Audio &audio;
};

class FePage
{
  public:
    explicit FePage(const PageKit &kit) : strings(kit.strings), shell(kit.shell), menus(kit.menus), painter(kit.painter), audio(kit.audio)
    {
    }
    virtual ~FePage() = default;
    FePage(const FePage &) = delete;
    FePage &operator=(const FePage &) = delete;

    virtual void enter()
    {
    }                                  // the page just opened (FeShell::goTo)
    virtual void input(const menu::MenuInput &in) = 0;       // each frame, with this frame's presses
    virtual void draw(const FeCanvas &canvas) = 0;
    virtual void step()
    {
    }                                   // one 60 Hz tick of the page's elements
    virtual void slideOutDone()
    {
    }                           // FeShell::slideOut ended with no page to open
    virtual std::u16string title() const
    {
        return {};
    }      // the shell title (font 1, right edge at 568, y 36)
    // the shell's description ticker; none: the shell draws no ticker (panel or text). The game's ticker element
    // (shell +0x1230) is shown by the page that writes its text (SetVisible(1), vtable slot 0x4c = 0xaf1c0) and hidden
    // by the slide-out (0x316e0 -> 0x31b84) and the race start (0x23a17). Argon shows it only on a page that has a
    // ticker (docs\argon\differences.md "Ticker on pages without one"); an empty text keeps the panel: the page has a
    // ticker, with nothing to say on this row
    virtual std::optional<std::u16string> ticker() const
    {
        return std::nullopt;
    }
    virtual bool bare() const
    {
        return false;
    }              // only its own elements over the background
    virtual bool overBlack() const
    {
        return false;
    }   // black instead of the shell's background (the credits, as on the Xbox)
    // the page's UI sounds by event (ui\fe_sounds.hpp); play: the one an event plays, if any
    virtual const menu::PageSounds &sounds() const
    {
        return menu::fe_sounds::kSilent;
    }
    void play(menu::UiEvent event) const;
    virtual FeScene scene()
    {
        return {};
    }                   // its models in the shell's render lists (none by default)
    virtual std::vector<FeScene> boxScenes()
    {
        return {};
    }   // models in boxes of their own, each with its camera and viewport (FeScene::boxed)

  protected:
    const GameStrings &strings;
    FeShell &shell;
    MenuKit &menus;
    HudPainter &painter;
    argon::Audio &audio;
};
