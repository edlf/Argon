// Argon: FE GHOSTSAVE "Save Ghost" (vtable 0x19de80), on leaving a Time Attack race (ui\ghostsave.cpp)
#pragma once
#include "logic/ghost.hpp"
#include "saves/ghost_library.hpp"
#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"
#include "ui/messagebox.hpp"

class GhostSavePage : public FePage, public MessageListener
{
  public:
    GhostSavePage(const PageKit &kit, GhostLibrary &ghosts, GhostKeeper &ghostKeeper) : FePage(kit), ghosts(ghosts), ghostKeeper(ghostKeeper)
    {
    }
    void open(int nextPage, ghost::Lap sessionGhost);   // nextPage: the menu::Page once the ghost is saved or dropped

    void input(const menu::MenuInput &) override
    {
    }       // only its questions: the message box takes the input
    void draw(const FeCanvas &) override
    {
    }   // and draws over the page
    std::u16string title() const override;
    void messageClosed(bool yes) override;

  private:
    GhostLibrary &ghosts;
    GhostKeeper &ghostKeeper;   // the save itself
    enum class Question
    {
        None,
        Save,
        Saving,
        SureWithoutSaving,
        Failed
    };
    void askToSave();
    void answerSave(bool yes);
    void saveGhost();
    void answerSureWithoutSaving(bool yes);
    void finish();

    int nextPage = 0;
    ghost::Lap session;   // the race's fastest lap (the game saves its event buffer)
    bool ghostExists = false;
    Question question = Question::None;
};
