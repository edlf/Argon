// Argon: MULTIPLAYERENDMENU (multiplayerend.cpp, opened at 0x52afa8 = 7 by 0x41de0 / 0x620e0): the
// multiplayer race's results and its menu (docs\frontend\multiplayer.md "Multiplayer").
#pragma once
#include "ui/fe_canvas.hpp"
#include <string>
#include <vector>

#include "ui/race_menu.hpp"

class MultiplayerSession;
class Race;

// what MULTIPLAYERENDMENU reads
struct MultiplayerEndSources
{
    const Race &race;                        // the finish order, the players' crafts and times
    const MultiplayerSession &multiplayer;   // the players, their wins, the winner by default
    const int &hudLaps;
};

class MultiplayerEndMenu : public RaceMenu
{
  public:
    MultiplayerEndMenu(const RaceMenuKit &kit, const MultiplayerEndSources &sources) : RaceMenu(kit), sources(sources)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::RaceBlur;
    }

  private:
    enum Item
    {
        kRaceAgain,
        kTrackSelect,
        kMainMenu
    };
    static const int kItems = 3;
    std::u16string label(int item) const;
    void answerConfirm();
    void runAction(int item);
    void drawItems(const FeCanvas &canvas) const;
    void drawTable(const FeCanvas &canvas) const;
    void drawPlayerRow(const FeCanvas &canvas, size_t row, int player) const;
    std::u16string raceTimeText(int player) const;

    MultiplayerEndSources sources;
    bool confirming = false;   // "Are you sure?" (0x40c)
    int selected = 0, confirmRow = 0;
};
