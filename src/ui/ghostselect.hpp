// Argon: FE GHOSTSELECT "Select Ghost" (vtable 0x19e0f8), after FE SPEEDSELECT in Time Attack (ui\ghostselect.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <vector>

#include "saves/ghost_library.hpp"

class GameModes;
#include "ui/page_hosts.hpp"
#include "ui/fe_page.hpp"
#include "ui/messagebox.hpp"

class GhostSelectPage : public FePage, public MessageListener
{
  public:
    struct Services
    {
        GhostLibrary &ghosts;
        GameModes &modes;   // Time Attack takes the ghost
        RaceStarter &races;
        GhostKeeper &ghostKeeper;   // the rows' names
    };
    GhostSelectPage(const PageKit &kit, const Services &services)
        : FePage(kit), ghosts(services.ghosts), modes(services.modes), races(services.races), ghostKeeper(services.ghostKeeper)
    {
    }
    void open(int pilot, int track);   // straight on to the loading screen when no ghost of this track is saved

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kGhostSelect;
    }
    void messageClosed(bool loadNow) override;   // the Loading box ran its time

  private:
    GhostLibrary &ghosts;
    GameModes &modes;   // Time Attack takes the ghost
    RaceStarter &races;
    GhostKeeper &ghostKeeper;   // the rows' names
    static constexpr int kRowsShown = 5;
    int rowCount() const
    {
        return 1 + int(ghostRows.size());
    }   // row 0: "No Loaded Ghost"
    std::u16string rowLabel(int row) const;
    std::u16string rowTime(int row) const;
    void moveCursor(const menu::MenuInput &in);
    void pickRow();
    void loadPickedGhost();
    void leaveForRace();
    void startRace();

    int pilot = 0, track = 0;
    int selected = 0, firstShown = 0;
    std::vector<int> ghostRows;   // App::ghosts' indices of this track
};
