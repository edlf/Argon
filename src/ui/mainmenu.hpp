// Argon: the main menu FE MAIN (vtable 0x19d798), the hub of the front end (ui\mainmenu.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <iterator>
#include <glm/glm.hpp>

#include "logic/game_logic.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/tourney_win.hpp"
#include "saves/profile_store.hpp"
#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"
#include "ui/menu_list.hpp"
#include "ui/messagebox.hpp"

class MainMenuPage : public FePage, public MessageListener
{
  public:
    enum Row
    {
        kTournament,
        kTimeAttack,
        kQuickRace,
        kMultiplayer,
        kOptions,
        kEditor,
        kArgonSettings,
        kExit,
        kRowKinds
    };
    // the rows shown, top to bottom: the editor's only in the developer build (ARGON_DEVELOP)
    static constexpr Row kShownRows[] = {kTournament,
                                         kTimeAttack,
                                         kQuickRace,
                                         kMultiplayer,
                                         kOptions,
                                         kArgonSettings,
#if ARGON_DEVELOP
                                         kEditor,
#endif
                                         kExit};
    static constexpr int kRows = int(std::size(kShownRows));

    struct Services
    {
        MenuDoor &door;   // the race / editor, the exit
        GameModes &modes;
        MultiplayerSession &multiplayer;   // left when a mode starts
        const argon::ProfileStore &profiles;   // the exit row's text
        const TourneyRun &tourney;   // awards to show
        bool &fastestLapPending;   // FE FASTESTLAP to show
    };
    MainMenuPage(const PageKit &kit, const Services &services)
        : FePage(kit), door(services.door), modes(services.modes), multiplayer(services.multiplayer), profiles(services.profiles), tourney(services.tourney),
          fastestLapPending(services.fastestLapPending)
    {
    }
    void enter() override;   // the rows slide in from x 640 (0x29d30)
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    void step() override;
    void slideOutDone() override;   // the editor row: the menu closes once the rows are out
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kMain;
    }
    FeScene scene() override;
    void messageClosed(bool yes) override;   // the Exit question

    void select(int row);
    void selectKind(Row kind);   // the cursor on that row (a page going back to FE MAIN)
    int selectedRow() const
    {
        return selected;
    }

  private:
    MenuDoor &door;   // the race / editor, the exit
    GameModes &modes;
    MultiplayerSession &multiplayer;   // left when a mode starts
    const argon::ProfileStore &profiles;   // the exit row's text
    const TourneyRun &tourney;   // awards to show
    bool &fastestLapPending;   // FE FASTESTLAP to show
    Row selectedKind() const
    {
        return kShownRows[selected];
    }
    void pickRow();
    void startMode(GameLogic &mode);
    menu::ListLayout listLayout() const;
    void followCursor();
    void drawRow(const FeCanvas &canvas, int row, int pads) const;

    int selected = 0;
    menu::ScrollWindow window;
    float rowX[kRows]{};   // per slot shown
};
