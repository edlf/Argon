// Argon: the Options record pages: FE FASTESTLAP as Track Records (vtable 0x19e028), FE HIGHSCORES (0x19e230)
// and FE GHOSTMANAGE as Ghost Data (0x19dad0) (ui\records.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <vector>
#include "logic/game_progress.hpp"
#include "saves/ghost_library.hpp"
#include "saves/lap_records.hpp"
#include "saves/profile_store.hpp"
#include "ui/fe_page.hpp"
#include "ui/page_hosts.hpp"

#include <array>
#include "ui/messagebox.hpp"

// the two pages of nine tracks (0..7 and Mars, up / down: their second routes) in three framed columns
// what the record pages read and write: the ghosts, the lap record table and its new records, the profile's high
// scores, the picks they open on
struct RecordSources
{
    GhostLibrary &ghosts;
    GhostKeeper &ghostKeeper;   // the ghosts' names
    LapRecordBook &lapRecords;
    std::array<bool, 18> &newLapRecords;   // the tracks whose record fell since FE FASTESTLAP last saved
    LapRecordStore &lapRecordStore;         // the table written back
    const argon::ProfileStore &profiles;
    const argon::GameProgress &progress;
    const int &speedClass;
    const int &character;
    const UnlockRules &rules;
};

class RecordTablePage : public FePage
{
  public:
    struct Place
    {
        int page, optionsRow;   // the menu::Page and its FE OPTIONS row
    };
    RecordTablePage(const PageKit &kit, const RecordSources &sources, const Place &place);
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;

  protected:
    RecordSources sources;
    static constexpr int kTrackRows = 9;
    void openTable();
    void moveAround(const menu::MenuInput &in);   // up / down the routes, left / right the column choice
    void leaveForMain();                            // after a race: on to FE MAIN
    int trackOfRow(int row) const;   // the record index: tracks 0..7 (second routes + 8), Mars 0x10 / 0x11
    bool secondRoutes = false;
    virtual uint32_t rowColour(int track) const;
    virtual void drawPageHints(const FeCanvas &canvas) const;

  private:
    virtual void changeColumn(const menu::MenuInput &in) = 0;   // left / right
    virtual void drawHeaders(const FeCanvas &canvas) const = 0;
    virtual void drawCells(const FeCanvas &canvas, float y, int track) const = 0;
    virtual float middleColumnWidth() const = 0;
    virtual std::u16string leftRightHint() const = 0;
    void drawFrames(const FeCanvas &canvas) const;
    std::u16string trackName(int row) const;

    const int page, optionsRow;
};

class TrackRecordsPage : public RecordTablePage, public MessageListener
{
  public:
    TrackRecordsPage(const PageKit &kit, const RecordSources &sources);
    void open();            // the current game speed (enter 0x20fa0)
    void openAfterRace();   // FE MAIN's pending page (0x29f49): the race's speed and the routes with a new record
    void input(const menu::MenuInput &in) override;
    void step() override;
    void messageClosed(bool yes) override;   // Saving ran its time: the table is written, FE MAIN
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kRecordTable;
    }

  private:
    void changeColumn(const menu::MenuInput &in) override;
    void drawHeaders(const FeCanvas &canvas) const override;
    void drawCells(const FeCanvas &canvas, float y, int track) const override;
    float middleColumnWidth() const override
    {
        return 150.f;
    }
    std::u16string leftRightHint() const override;
    uint32_t rowColour(int track) const override;
    void drawPageHints(const FeCanvas &canvas) const override;
    bool savesNewRecords() const;   // +0x4150: a record fell since the last save
    void continueAfterNewRecords();

    int speed = 0;
    uint8_t flash = 255;   // +0x414c: the new records' grey
};

class HighScoresPage : public RecordTablePage
{
  public:
    HighScoresPage(const PageKit &kit, const RecordSources &sources);
    void open();   // the current character, if unlocked (enter 0x26490)
    std::u16string title() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kRecordTable;
    }

  private:
    void changeColumn(const menu::MenuInput &in) override;
    void drawHeaders(const FeCanvas &canvas) const override;
    void drawCells(const FeCanvas &canvas, float y, int track) const override;
    float middleColumnWidth() const override
    {
        return 160.f;
    }
    std::u16string leftRightHint() const override;
    std::u16string scoreText(int score) const;

    int character = 0;
};

class GhostDataPage : public FePage, public MessageListener
{
  public:
    GhostDataPage(const PageKit &kit, const RecordSources &sources) : FePage(kit), sources(sources)
    {
    }
    void open();

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kGhostManage;
    }
    void messageClosed(bool yes) override;   // the delete question

  private:
    RecordSources sources;
    static constexpr int kRowsShown = 5;
    static constexpr int kTracks = 18;   // the game's track ids 0..0x11 (second routes 8..15, Mars 16 / 17)
    std::vector<int> ghostsOfTrack() const;   // App::ghosts' indices on this page's track
    void showTrackWithGhosts(int step);       // from the current track, this one or the next with ghosts (+1 / -1)
    void showNextTrackWithGhosts(int step);
    void moveCursor(const menu::MenuInput &in);
    void askToDelete();
    std::u16string rowLabel(int ghost) const;
    void keepCursorInList();
    void drawNoGhosts(const FeCanvas &canvas) const;
    void drawList(const FeCanvas &canvas) const;

    int track = 0;
    int selected = 0, firstShown = 0;
};
