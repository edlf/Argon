// Argon: FE MESSAGEBOX (vtable 0x19da68): a text and Continue, a Yes / No question, the Saving / Loading
// pause before a save or load, the reconnect-the-controller message (ui\messagebox.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <string>

#include "input/player_input.hpp"
#include "ui/fe_page.hpp"
#include "ui/message_buttons.hpp"

// the page that asked: told the answer once the box closes
class MessageListener
{
  public:
    // yes: the first button (Yes, Continue without saving) was picked, or a Saving / Loading box ran its time (the
    // caller saves / loads now); false for the others, info boxes and the controller message.
    virtual void messageClosed(bool yes) = 0;

  protected:
    ~MessageListener() = default;
};

// a shell page of its own in the game (the caller's page is left, fe_shell_goto_page); in the viewer the shell
// (ui\fe_shell.hpp) runs it instead of the current page while it is open, and keeps that page's title and ticker
class MessageBoxPage : public FePage
{
  public:
    using Kind = menu::MessageKind;

    MessageBoxPage(const PageKit &kit, const PlayerInput &players) : FePage(kit), players(players)
    {
    }
    void open(const std::u16string &text, Kind kind, MessageListener &caller);   // open 0x2b000
    void open(const std::u16string &text, Kind kind);                            // nobody waits for the answer
    void openTimed(const std::u16string &text, float seconds);                  // kind 1: closes by itself
    void openForPad(const std::u16string &text, int pad);   // kind 3: a controller message only that port closes
    void close();                                                                 // close 0x2b470, no answer
    bool isOpen() const
    {
        return shown;
    }
    bool hidesShellText() const
    {
        return isBusy();
    }   // Saving / Loading hide the title and ticker (0x2b341)

    void input(const menu::MenuInput &in) override;
    void step() override;
    void draw(const FeCanvas &canvas) override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kMessageBox;
    }

  private:
    const PlayerInput &players;   // the closing pad's own presses
    class NobodyListens : public MessageListener
    {
      public:
        void messageClosed(bool) override
        {
        }
    };

    bool isBusy() const
    {
        return kind == Kind::Saving || kind == Kind::Loading;
    }
    menu::MessageButtons buttons() const
    {
        return menu::MessageButtons(kind);
    }
    void moveCursor(const menu::MenuInput &in);
    void answer(bool yes);
    void drawText(const FeCanvas &canvas) const;
    void drawButtons(const FeCanvas &canvas) const;
    void drawBusyLabel(const FeCanvas &canvas) const;
    uint32_t textColour() const;

    NobodyListens nobody;
    MessageListener *listener = &nobody;
    std::u16string text;
    Kind kind = Kind::Info;
    int selected = 0;
    int ticks = 0;          // 60 Hz steps since it opened
    int timedTicks = 0;     // Timed: +0x1f4, the close time (open + seconds)
    uint8_t flash = 255;    // +0xe1c: the controller message's grey, 255 down to 64 by 16 a frame
    int closingPad = -1;    // kind 3: +0x1f4, the port whose A / START closes it (-1: any)
    bool shown = false;
};
