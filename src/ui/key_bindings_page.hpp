// Argon: Argon Options' Key Bindings: the keyboard's craft keys (input\key_bindings.hpp), each action two, laid out as
// the settings lists' small rows. Up / down picks the action, left / right its first or second key, A waits for the new
// key (Esc or a pad's B keeps the old one; Esc, P, F2, F11 and F12 are refused), X clears it. Like Argon Options' lists
// nothing changes until Save writes the keys to settings.json; B leaves without them.
#pragma once
#include "input/key_bindings.hpp"
#include "ui/fe_page.hpp"
#include "ui/menu_list.hpp"
#include "ui/messagebox.hpp"
#include "ui/page_hosts.hpp"

class PlayerInput;

class KeyBindingsPage : public FePage, public MessageListener
{
  public:
    struct Services
    {
        KeyBindingStore &store;
        PlayerInput &players;   // the new key
    };
    KeyBindingsPage(const PageKit &kit, const Services &services) : FePage(kit), services(services)
    {
    }
    void open();   // the keys settings.json keeps, the cursor on the top row

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kSettings;
    }
    void messageClosed(bool yes) override;   // the Reset to Default question

  private:
    // the rows, top first: the actions (KeyAction order), Reset to Default, Save
    static constexpr int kResetRow = kKeyActionCount, kSaveRow = kKeyActionCount + 1, kRowCount = kKeyActionCount + 2;

    void takeAwaitedKey(const menu::MenuInput &in);
    void changeSlot(const menu::MenuInput &in);
    void runRow();
    void leavePage();
    bool onAction() const;
    std::u16string rowLabel(int row) const;
    std::u16string keyText(int row, int slot) const;
    menu::ListLayout listLayout() const;
    void followCursor();
    void drawRow(const FeCanvas &canvas, int row, float y) const;

    Services services;
    KeyBindings edited;
    int selected = 0, slot = 0;
    bool awaitingKey = false;
    menu::ScrollWindow window;
};
