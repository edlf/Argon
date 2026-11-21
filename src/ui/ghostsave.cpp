// Argon: FE GHOSTSAVE "Save Ghost" (vtable 0x19de80: update 0x22780, messages 0x228d0): on leaving a Time
// Attack race through the pause menu's Main Menu / Track Select with a session ghost (0x8b5b0 / 0x8b630, 0x459a8e).
// The page is its message boxes: without a valid lap (0x52ac27) it goes straight on; with no ghost saved for this
// character and track, "Do you want to save your ghost from that session?" (fe 0x1a4, Yes / No), with one, "A ghost
// already exists for this character and track. ..." (0x1a5, kind 6: "Yes, replace the old ghost" / "No, keep the
// old ghost"); not saving asks "Are you sure you want to continue without saving your ghost?" (0x1a9) / "Are you
// sure?" (0x1a6), No back to the first question. Then FE MAIN (0x1a3120; Track Select: FE COURSE SELECT).
#include "ui/ghostsave.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"

void GhostSavePage::open(int next, ghost::Lap sessionGhost)
{
    nextPage = next;
    session = std::move(sessionGhost);
    question = Question::None;
    shell.goTo(menu::PageGhostSave);
    if (!session.valid())
    {
        finish();
        return;
    }
    ghosts.scan();
    ghostExists = ghosts.find(session.track, session.pilot).has_value();
    askToSave();
}

std::u16string GhostSavePage::title() const
{
    return strings.frontEnd(0x35c, "Save Ghost");
}

void GhostSavePage::askToSave()
{
    question = Question::Save;
    if (ghostExists)
    {
        shell.messageBox().open(strings.frontEnd(0x1a5, "A ghost already exists for this character and track. Do you want to replace it with your lap?"),
                                  MessageBoxPage::Kind::ReplaceGhost,
                                  *this);
    }
    else
    {
        shell.messageBox().open(strings.frontEnd(0x1a4, "Do you want to save your ghost from that session?"), MessageBoxPage::Kind::YesNo, *this);
    }
}

void GhostSavePage::messageClosed(bool yes)
{
    if (question == Question::Save)
    {
        answerSave(yes);
    }
    else if (question == Question::Saving)
    {
        saveGhost();
    }
    else if (question == Question::SureWithoutSaving)
    {
        answerSureWithoutSaving(yes);
    }
    else
    {
        finish();
    }
}

// Yes: FE MESSAGEBOX kind 0xb ("Saving", 0.5 s), then the ghost is written (0x2b4b0)
void GhostSavePage::answerSave(bool yes)
{
    if (yes)
    {
        question = Question::Saving;
        shell.messageBox().open(std::u16string(), MessageBoxPage::Kind::Saving, *this);
        return;
    }
    question = Question::SureWithoutSaving;
    shell.messageBox().open(ghostExists ? strings.frontEnd(0x1a6, "Are you sure?")
                                          : strings.frontEnd(0x1a9, "Are you sure you want to continue without saving your ghost?"),
                              MessageBoxPage::Kind::YesNo,
                              *this);
}

void GhostSavePage::saveGhost()
{
    if (ghostKeeper.saveGhost(session))
    {
        finish();
        return;
    }
    question = Question::Failed;
    shell.messageBox().open(strings.frontEnd(0x35f, "Could not save ghost."), MessageBoxPage::Kind::Info, *this);
}

void GhostSavePage::answerSureWithoutSaving(bool yes)
{
    if (yes)
    {
        finish();
    }
    else
    {
        askToSave();
    }
}

void GhostSavePage::finish()
{
    session = {};
    shell.goTo(nextPage);
}
