// Argon: the race's score pages SCOREMENU (vtable 0x19c018) and POINTSMENU, the breakdown (0x19c500)
// (ui\score.cpp; the scores themselves: app_score.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "ui/race_menu.hpp"

class GameModes;
struct RaceScoring;

// what SCOREMENU and POINTSMENU read and change
struct ScoreSources
{
    RaceEndPages &pages;          // WINMENU / LOSEMENU after them
    RaceScoring &scoring;         // the score, its count-up and the points
    const GameModes &modes;       // the cash line in a tournament
    const int &speedClass;
    int &raceEnd;                 // race_end::Page: POINTSMENU from SCOREMENU
};

class ScoreMenu : public RaceMenu
{
  public:
    ScoreMenu(const RaceMenuKit &kit, const ScoreSources &sources) : RaceMenu(kit), sources(sources)
    {
    }
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::RaceBlur;
    }

  private:
    void leave() override;
    void countScore();
    void skipCount();   // A: the count jumps to the score
    void stepCount();
    void finishCount();
    void drawScoreLines(const FeCanvas &canvas) const;
    void drawScoreThisRace(const FeCanvas &canvas) const;
    void drawCashLine(const FeCanvas &canvas) const;
    ScoreSources sources;
    float countClock_ = 0.f;
};

class PointsMenu : public RaceMenu
{
  public:
    PointsMenu(const RaceMenuKit &kit, const ScoreSources &sources) : RaceMenu(kit), sources(sources)
    {
    }
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    Background background() const override
    {
        return Background::RaceBlur;
    }

  private:
    void leave() override;
    ScoreSources sources;
};
