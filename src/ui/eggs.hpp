// Argon: the Options pages FE EASTEREGGS (vtable 0x19d928) and FE CHEATMENU (vtable 0x19db38) (ui\eggs.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "logic/game_progress.hpp"
#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"

class EasterEggsPage : public FePage
{
  public:
    EasterEggsPage(const PageKit &kit, argon::GameProgress &progress, ProgressStore &progressStore)
        : FePage(kit), progress(progress), progressStore(progressStore)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    void step() override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kEasterEggs;
    }

  private:
    argon::GameProgress &progress;   // the unlocked eggs and cheats, their switches
    ProgressStore &progressStore;
    static constexpr int kEggs = 8, kExitRow = 8;
    bool rowIsOpen(int row) const;
    void drawOffOn(const FeCanvas &canvas, int row, float y) const;

    int selected = 0;
    int pulse = 0xff;   // the cursor row's choice: grey 255 -> 64, -16 a frame (update 0x1f440)
};

class CheatMenuPage : public FePage
{
  public:
    CheatMenuPage(const PageKit &kit, argon::GameProgress &progress, ProgressStore &progressStore)
        : FePage(kit), progress(progress), progressStore(progressStore)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kCheatMenu;
    }

  private:
    argon::GameProgress &progress;   // the unlocked eggs and cheats, their switches
    ProgressStore &progressStore;
    static constexpr int kCheats = 7, kFirstSwitch = 5, kEnterCodeRow = 7, kExitRow = 8;
    bool rowIsOpen(int row) const;
    bool isSwitch(int row) const
    {
        return row >= kFirstSwitch && row < kCheats;
    }   // Infinite Shield / Turbo
    void pickCheat();
    void drawValue(const FeCanvas &canvas, int row, float y) const;
    std::u16string rightHint() const;

    int selected = 0;
};
