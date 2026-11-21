// Argon: FE TOURNEYTYPE "Tournament" (vtable 0x19dfc0), after FE CHARSELECT in Tournament (ui\tourney.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "logic/game_progress.hpp"
#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"
#include "ui/messagebox.hpp"
#include "ui/tourney_icons.hpp"

class TourneyTypePage : public FePage, public MessageListener
{
  public:
    struct Services
    {
        argon::GameProgress &progress;   // each character's tournaments and trophies
        const int &character;   // FE CHARSELECT's pick
        const UnlockRules &rules;
        RaceStarter &races;
        const std::string &gameFolder;
    };
    TourneyTypePage(const PageKit &kit, const Services &services)
        : FePage(kit), progress(services.progress), character(services.character), rules(services.rules), races(services.races), gameFolder(services.gameFolder)
    {
    }
    void open();   // enter 0x36320: the cursor on the last tournament started (0x1bbbec)

    void enter() override;   // its pictures loaded here: drawing only looks them up
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    FeScene scene() override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kTourneyType;
    }
    void messageClosed(bool yes) override;   // New Tournament's question (fe 0x156)

  private:
    argon::GameProgress &progress;   // each character's tournaments and trophies
    const int &character;   // FE CHARSELECT's pick
    const UnlockRules &rules;
    RaceStarter &races;
    const std::string &gameFolder;
    static constexpr int kLevels = 5;
    static constexpr int kNoChoice = -1, kNewTournament = 0, kContinueTournament = 1;
    bool selectedIsOpen() const;
    bool selectedHasProgress() const;
    void moveLevelCursor(const menu::MenuInput &in);
    void chooseNewOrContinue(const menu::MenuInput &in);
    void pickLevel();
    void startNewTournament();
    void drawLevels(const FeCanvas &canvas) const;
    void drawNewOrContinue(const FeCanvas &canvas) const;
    void drawLevelIcons(const FeCanvas &canvas, int level) const;
    menu::TourneyRowState rowState(int level) const;

    int selected = 0;
    int choice = kNoChoice;   // New / Continue Tournament (+0x1218 shown), asked when the level has races won
};
