// Argon: FE AWARDS, the rewards after a tournament win (build 0x128d0, enter 0x12e70, input 0x12b60, update /
// models 0x133c0, frame 0x13790, rows 0x137f0; docs\frontend\tournament.md "Tournament"). FE MAIN opens it while
// App::tourneyRun.awardsPending is set (0x52ac00, set with the last race's WINMENU); it lists App::tourneyRun.awards.
#pragma once
#include "ui/fe_canvas.hpp"
#include <string>
#include <vector>

#include "logic/tourney_win.hpp"
#include "ui/fe_page.hpp"

class AwardsPage : public FePage
{
  public:
    AwardsPage(const PageKit &kit, TourneyRun &tourney) : FePage(kit), tourney(tourney)
    {
    }

    void enter() override;   // the rows from what the win gave; without a new trophy straight back to FE MAIN
    void input(const menu::MenuInput &in) override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kAwards;
    }
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    FeScene scene() override;

  private:
    void listAwards();
    void addRow(const std::u16string &text)
    {
        rows.push_back(text);
    }
    void showCharacter(int pilot);
    void leave();

    TourneyRun &tourney;                // the win's awards, and whether FE MAIN still has to show them
    std::vector<std::u16string> rows;   // row 0 the trophy, then the unlocks
    int character = -1;                 // the unlocked character shown (Ka Hekili, Kutoo or a nemesis)
    double enteredAt = 0;
};
