// Argon: the Options pages: FE OPTIONS (vtable 0x19da00), FE OPTIONS MAIN = Game Settings (0x19d990) and
// FE OPTIONS SOUND = Sound Settings (0x19dc70), both run by the settings list engine, and FE MUSICSELECT (0x19e160)
// (ui\options.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include <vector>

#include "audio/custom_music.hpp"
#include "saves/profile_store.hpp"
#include "ui/fe_page.hpp"
#include "ui/page_hosts.hpp"
#include "ui/settings_menu.hpp"

#include <string>
#include "ui/viewer_text.hpp"
#include "ui/menu_list.hpp"
#include "ui/messagebox.hpp"

// what FE OPTIONS, the settings pages and Music Options read and change
struct OptionSources
{
    MenuDoor &door;                 // FE MAIN again after a profile's options were saved
    SettingsStore &settingsStore;   // the rows built, changed and stored
    SettingsMenu &settingsMenu;     // the rows
    CustomMusic &music;
    MusicStore &musicStore;
    argon::ProfileStore &profiles;
    const std::string &gameFolder;   // the engine test's sounds
};

class OptionsPage : public FePage, public MessageListener
{
  public:
    OptionsPage(const PageKit &kit, const OptionSources &sources) : FePage(kit), sources(sources)
    {
    }
    void open();                // the cursor where it was
    void openAt(int row);       // coming back from a row's page (its kOptionsRows index): the cursor on it

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    void step() override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kOptions;
    }
    FeScene scene() override;
    void messageClosed(bool) override;   // "Saving" ran its time: the profile is written, FE MAIN

  private:
    OptionSources sources;
    static constexpr int kRows = 9;
    void openRow();
    void leaveToMain();
    std::vector<int> shownRows() const;   // kOptionsRows indices, top first
    int selectedRow() const;
    menu::ListLayout listLayout() const;
    void followCursor();

    int selected = 0;   // the position in shownRows
    menu::ScrollWindow window;
    int profileWritesAtEntry = 0;   // App::profiles.writes() when FE MAIN opened the page
    float rowX[kRows]{};   // the rows slide in from x 640 to 62 in 10 steps (0x2d9d0)
};

class SettingsPage : public FePage, public MessageListener
{
  public:
    // the entries without options
    enum EntryType
    {
        kExit = 0,
        kResetToDefault = 1,
        kMusicOptions = 9,
    };

    // page: menu::PageGameSettings / PageSoundSettings or one of Argon Options' (PageArgonGeneral ..); list: its rows;
    // parentRow: its row on the page B goes back to (FE OPTIONS, or Argon Options for its lists), the cursor left on it
    struct Place
    {
        int page;
        SettingsList list;
        int parentRow;
        int titleId;   // the FE text of its title, -1 for the fallback
        ViewerText titleFallback;
    };
    SettingsPage(const PageKit &kit, const OptionSources &sources, const Place &place);
    void open();                  // the lists rebuilt, the cursor on the last entry (the top row)
    void resumeAt(int type);      // back from a sub-page: the cursor on the entry of that type

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kSettings;
    }
    void messageClosed(bool yes) override;   // the Reset to Default question

  private:
    OptionSources sources;
    // the Renderer's restart notice: the page is left once it closes
    class RestartNotice : public MessageListener
    {
      public:
        explicit RestartNotice(SettingsPage &page) : page_(page)
        {
        }
        void messageClosed(bool) override
        {
            page_.leavePage();
        }

      private:
        SettingsPage &page_;
    };

    std::vector<SettingRow> &entries() const;
    int entryCount() const;
    void changeOption(const menu::MenuInput &in);
    void runEntry();
    std::u16string resetQuestion() const;
    void saveArgonSettings();
    void previewEngineVolume();
    void leavePage();
    bool isArgonSettings() const;
    std::u16string rightHint() const;
    static std::u16string chosenWithArrows(const std::vector<std::u16string> &options, int chosen);
    menu::ListLayout listLayout() const;
    void followCursor();
    void drawEntry(const FeCanvas &canvas, int entry, float y) const;

    const int page;
    const SettingsList list;
    const int parentRow, titleId;
    const ViewerText titleFallback;
    int selected = 0;
    menu::ScrollWindow window;   // over the rows top first (the last entry is the top row)
    RestartNotice restartNotice{*this};
};

class MusicSelectPage : public FePage
{
  public:
    MusicSelectPage(const PageKit &kit, const OptionSources &sources) : FePage(kit), sources(sources)
    {
    }
    void openAt(int row);

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kMusicSelect;
    }

  private:
    OptionSources sources;
    enum Row
    {
        kMusicSource,
        kEditPlaylist,
        kExit,
        kRowCount
    };
    bool canEditPlaylist() const;
    void editPlaylist();
    void drawSourceChoices(const FeCanvas &canvas, float y) const;
    std::u16string rightHint() const;

    int selected = 0;
};
