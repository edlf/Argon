// Argon: the race end's pages: POSTABLE "Results" (vtable 0x19d018) and WINMENU / LOSEMENU (ui\results.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <string>
#include <vector>

#include "ui/race_menu.hpp"

class GameModes;
class ProgressKeeper;
class Race;
struct TourneyRun;
namespace argon
{
class ProfileStore;
struct AiGraph;
struct GameProgress;
}  // namespace argon

// what POSTABLE reads and changes
struct ResultsSources
{
    const Race &race;                 // the finish order, the racers' times and kills
    const argon::AiGraph &aiGraph;    // the racers' lap fractions
    const int &hudLaps;
    int &raceEnd;                     // race_end::Page: SCOREMENU after it
};

class ResultsTable : public RaceMenu
{
  public:
    ResultsTable(const RaceMenuKit &kit, RaceEndHost &endHost, const ResultsSources &sources) : RaceMenu(kit), endHost(endHost), sources(sources)
    {
    }
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::RaceBlur;
    }

  private:
    struct Row
    {
        int racer = 0;
        float raceTime = 0.f, lapTime = 0.f;   // lap 999999: none
        int kills = 0;
        bool dnf = false;                      // removed from the race (knocked out, the time limit)
        bool finished = false;                 // past the line
    };
    RaceEndHost &endHost;
    ResultsSources sources;
    std::vector<Row> rows_;   // written on the first draw, as 0x8f140 on the frame POSTABLE ON arrives

    void fillRows();
    void printRows() const;

    void drawPlace(const FeCanvas &canvas) const;
    void drawCells(const FeCanvas &canvas, size_t rows) const;
    void drawHeaders(const FeCanvas &canvas) const;
    void drawRow(const FeCanvas &canvas, size_t index) const;
    Row rowOf(int racer) const;
};

// what WINMENU / LOSEMENU read and change
struct RaceEndSources
{
    RaceEndPages &pages;                     // UPGRADEMENU
    const GameModes &modes;                  // the items the mode has
    const int &raceEnd;                      // race_end::Page: WINMENU or LOSEMENU
    const TourneyRun &tourneyRun;
    const argon::ProfileStore &profiles;     // Save and Quit, or Quit without a profile
    const ProgressKeeper &progressKeeper;    // the tournament's next track
    const argon::GameProgress &progress;
};

// WINMENU / LOSEMENU: one object for both (RaceSession::raceEnd says which)
class RaceEndMenu : public RaceMenu
{
  public:
    RaceEndMenu(const RaceMenuKit &kit, RaceEndHost &endHost, const RaceEndSources &sources) : RaceMenu(kit), endHost(endHost), sources(sources)
    {
    }
    void open();   // the cursor on the first item (Upgrade Menu in a tournament)

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::RaceBlur;
    }   // as the result pages (xemu)

  private:
    enum Item
    {
        kRaceAgain,
        kViewReplay,
        kTrackSelect,
        kMainMenu,
        kNextRace,
        kUpgradeMenu,
        kSaveAndQuit,
        kContinue
    };
    std::vector<Item> items() const;
    std::u16string label(Item item) const;
    void pickItem();
    void answerConfirm();
    void leave() override;
    void runAction(Item action);

    RaceEndHost &endHost;
    RaceEndSources sources;
    bool confirming = false;   // the "Are you sure?" page
    int selected = 0, confirmRow = 0;
};
