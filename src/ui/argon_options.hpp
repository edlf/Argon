// Argon: Argon Options, the viewer's own settings (a FE MAIN row; docs\argon\differences.md): a page of its pages, laid
// out as FE OPTIONS: General, Graphics, Audio and Game Fixes (settings lists, ui\options.cpp), Key Bindings
// (ui\key_bindings_page.hpp) and Exit, back to FE MAIN.
#pragma once
#include "ui/fe_page.hpp"
#include "ui/menu_list.hpp"

class ArgonOptionsPage : public FePage
{
  public:
    enum Row
    {
        kGeneral,
        kGraphics,
        kAudio,
        kGameFixes,
        kKeyBindings,
        kExit,
        kRowCount
    };

    explicit ArgonOptionsPage(const PageKit &kit) : FePage(kit)
    {
    }
    void open();             // from FE MAIN: the cursor on the top row
    void openAt(Row row);    // back from a row's page: the cursor on it

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    void step() override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kOptions;
    }

  private:
    void openRow();
    void leaveToMain();
    std::u16string rowLabel(int row) const;
    menu::ListLayout listLayout() const;
    void followCursor();

    int selected = 0;
    menu::ScrollWindow window;
    float rowX[kRowCount]{};   // the rows slide in, as FE OPTIONS' (0x2d9d0)
};
