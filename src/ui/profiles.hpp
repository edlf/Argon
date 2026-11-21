// Argon: the profile pages FE CHOOSESAVE "Select Profile" (vtable 0x19e090, ui\choosesave.cpp) and the
// on-screen keyboard FE NAME ENTRY (vtable 0x1a2478, ui\keyboard.cpp)
#pragma once
#include "ui/fe_canvas.hpp"
#include "saves/profile_store.hpp"
#include "ui/page_hosts.hpp"
#include "string"
#include "saves/viewer_settings.hpp"
#include "ui/fe_page.hpp"
#include "ui/messagebox.hpp"

class ChooseSavePage : public FePage, public MessageListener
{
  public:
    struct Services
    {
        MenuDoor &door;   // a profile chosen: FE MAIN
        argon::ProfileStore &profiles;
        ProfileKeeper &profileKeeper;
        const std::string &gameFolder;   // the profile card's texture
    };
    ChooseSavePage(const PageKit &kit, const Services &services)
        : FePage(kit), door(services.door), profiles(services.profiles), profileKeeper(services.profileKeeper), gameFolder(services.gameFolder)
    {
    }
    void open();   // the list from the saves, the current profile selected

    void enter() override;   // its pictures loaded here: drawing only looks them up
    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kChooseSave;
    }
    void messageClosed(bool yes) override;   // the delete question

  private:
    MenuDoor &door;   // a profile chosen: FE MAIN
    argon::ProfileStore &profiles;
    ProfileKeeper &profileKeeper;
    const std::string &gameFolder;   // the profile card's texture
    static constexpr int kRowsShown = 5;
    static constexpr int kCreateRow = 0;   // the list's first row, "Create New Profile"; the profiles follow it
    int profileCount() const;
    int rowCount() const;
    static int rowOfProfile(int profile);
    static int profileOfRow(int row);
    void moveCursor(const menu::MenuInput &in);
    void showRow(int row);   // scrolls the list so that the row is shown and selected
    void createProfile();
    void askToDelete(int profile);
    void loadProfile(int profile);
    void drawIdCard(const FeCanvas &canvas) const;
    void drawHeader(const FeCanvas &canvas) const;
    void drawRow(const FeCanvas &canvas, int row) const;
    void drawScrollArrows(const FeCanvas &canvas) const;

    int selected = 0, firstShown = 0;   // the selected row, the first row shown
    int deleteTarget = -1;              // the profile the delete question is about
};

class NameEntryPage : public FePage
{
  public:
    struct Services
    {
        MenuDoor &door;   // a profile made: FE MAIN
        argon::ProfileStore &profiles;   // the names taken
        ProfileKeeper &profileKeeper;
        CheatCodes &cheatCodes;   // a cheat code typed
        const ViewerSettings &settings;   // the language: the Accents key
    };
    NameEntryPage(const PageKit &kit, const Services &services)
        : FePage(kit), door(services.door), profiles(services.profiles), profileKeeper(services.profileKeeper), cheatCodes(services.cheatCodes),
          settings(services.settings)
    {
    }
    void openForName();        // "PLAYER" + the first number not taken, replaced by the first key
    void openForCheatCode();   // from FE CHEATMENU: the text starts empty

    void input(const menu::MenuInput &in) override;
    void draw(const FeCanvas &canvas) override;
    std::u16string title() const override;
    std::optional<std::u16string> ticker() const override;
    const menu::PageSounds &sounds() const override
    {
        return menu::fe_sounds::kNameEntry;
    }

  private:
    MenuDoor &door;   // a profile made: FE MAIN
    argon::ProfileStore &profiles;   // the names taken
    ProfileKeeper &profileKeeper;
    CheatCodes &cheatCodes;   // a cheat code typed
    const ViewerSettings &settings;   // the language: the Accents key
    static constexpr size_t kMaxLength = 9;
    void resetKeyboard();
    bool keyExists(int row, int col) const;
    int keyCode() const;
    void moveCursor(const menu::MenuInput &in);
    void pressKey(int code);
    void type(char16_t character);
    void typePcKeyboard();
    void done();
    void finishName();
    void back();
    std::u16string keyLabel(int code) const;
    void drawKeys(const FeCanvas &canvas) const;

    std::u16string text;
    bool cheatCode = false;    // typing a cheat code (title fe 0x304)
    bool fresh = true;         // the suggested name, replaced by the first key
    bool caps = false;
    int keyPage = 0, keyRow = 4, keyCol = 1;
    int rememberedCol = 0;   // +0x14a30 (ui\keyboard_moves.hpp)
};
