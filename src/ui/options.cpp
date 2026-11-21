// Argon: Options. FE OPTIONS (vtable 0x19da00: build 0x2c740, enter 0x2d9d0, input 0x2cef0, update 0x2d830,
// ticker 0x2e000), FE OPTIONS MAIN = Game Settings (0x19d990: build 0x2f000, enter 0x2f1a0, input 0x2f0b0) and
// FE OPTIONS SOUND = Sound Settings (0x19dc70: build 0x2e970, enter 0x2ef90, input 0x2ea20), both run by the settings
// list engine (rows 0xabca0, input 0xac040, enter 0xac6c0) over the lists built by 0x81fd0 / 0x824c0, and
// FE MUSICSELECT (0x19e160); docs\frontend\options.md "Options". The viewer's Argon Options page (a main menu row) runs on the same engine
// (docs\argon\differences.md); its pages' lists run here too. The settings entries and their effect: app_settings.cpp.
#include "ui/options.hpp"

#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/menu_common.hpp"
#include "ui/viewer_text.hpp"

using namespace menu::fe;

namespace
{
// FE OPTIONS rows (0x2c740): string, ticker (0x2e000)
struct OptionsRow
{
    int str, desc;
    const char *fb;
    const char *descFb;
};

const OptionsRow kOptionsRows[9] = {
    {0x281, 0x370, "Game Settings", "Adjust HUD, Brightness, and Controller Settings"},
    {0x282, 0x371, "Sound Settings", "Adjust Volumes, and Edit Playlist"},
    {0x283, 0x372, "Track Records", "View Track Records"},
    {0x3c0, 0x377, "High Scores", "View High Scores"},
    {0x287, 0x378, "Ghost Data", "View and Delete Ghosts"},
    {0x284, 0x373, "Credits", "View Credits"},
    {0x8, 0x374, "Exit", "Return to Main Menu"},
    {0x286, 0x376, "Easter Eggs", "Activate Easter Eggs"},
    {0x285, 0x375, "Cheats", "Enter Cheat Codes"},
};
enum OptionsRowId
{
    kGameSettings,
    kSoundSettings,
    kTrackRecords,
    kHighScores,
    kGhostData,
    kCredits,
    kExit,
    kEasterEggs,
    kCheats
};
}  // namespace

// ---- FE OPTIONS

// the rows shown, top first: Easter Eggs with the cheat menu or once an egg is unlocked (0x1bbc08..), Cheats with the
// cheat menu (0x1bbc26), both before Exit (the game has them after it; docs\argon\differences.md)
std::vector<int> OptionsPage::shownRows() const
{
    std::vector<int> rows{kGameSettings, kSoundSettings, kTrackRecords, kHighScores, kGhostData, kCredits};
    if (sources.settingsStore.easterEggsShown())
    {
        rows.push_back(kEasterEggs);
    }
    if (sources.settingsStore.cheatsShown())
    {
        rows.push_back(kCheats);
    }
    rows.push_back(kExit);
    return rows;
}

int OptionsPage::selectedRow() const
{
    const std::vector<int> rows = shownRows();
    return rows[size_t(std::clamp(selected, 0, int(rows.size()) - 1))];
}

// enter (0x2d9d0): hints Back / Select; the rows slide in from x 640 to 62 (y 100 + 30 i)
void OptionsPage::open()
{
    if (!shell.isOpen())
    {
        sources.door.openMenu();
    }
    if (shell.current() == menu::PageMain)
    {
        profileWritesAtEntry = sources.profiles.writes();
    }
    sources.settingsStore.buildSettings();
    sources.profiles.load();
    shell.goTo(menu::PageOptions);
    for (float &x : rowX)
    {
        x = kRowFrom;
    }
    followCursor();
}

void OptionsPage::openAt(int row)
{
    open();
    const std::vector<int> rows = shownRows();
    for (size_t position = 0; position < rows.size(); ++position)
    {
        if (rows[position] == row)
        {
            selected = int(position);
        }
    }
    followCursor();
}

std::u16string OptionsPage::title() const
{
    return strings.frontEnd(0x280, "Options");
}

std::optional<std::u16string> OptionsPage::ticker() const
{
    const OptionsRow &row = kOptionsRows[selectedRow()];
    return strings.frontEnd(row.desc, row.descFb);
}

void OptionsPage::step()
{
    menu::slideRows(rowX, kRowFrom, kRowX);
}

// the page's model (0x2d830): the main menu's Options item (shell +0x1ff8) at (-4, -1, 0), camera 1
FeScene OptionsPage::scene()
{
    FeScene scene;
    scene.models[0] = &shell.models().menuItem(4);
    scene.placement = glm::translate(glm::mat4(1.f), glm::vec3(-4.f, -1.f, 0.f));
    scene.zNear = 0.001f;
    return scene;
}

// input (0x2cef0): up / down, A opens the row's page; Exit or B back to FE MAIN; the title and hints slide out first
void OptionsPage::input(const menu::MenuInput &in)
{
    if (menus.menuCursor(selected, int(shownRows().size()), in))   // Easter Eggs / Cheats only once unlocked
    {
        play(menu::UiEvent::Cursor);
        followCursor();
    }
    if (in.b || (in.a && selectedRow() == kExit))
    {
        leaveToMain();
        return;
    }
    if (!in.a)
    {
        return;
    }
    shell.slideOut(menu::UiEvent::LeftForward);   // 0x2cef0: state 4, the page 0.1 s later (0x2d830 case 7)
    openRow();
}

// 0x2d405: with a profile whose settings changed in Options (+0x1651), FE MESSAGEBOX kind 4 ("Saving", 0.5 s) first;
// then (0x2dfd0) state 4 and FE MAIN 0.1 s later. The viewer stores each change at once, so "changed" is: a profile
// was written since FE MAIN opened Options
void OptionsPage::leaveToMain()
{
    if (sources.profiles.hasCurrent() && sources.profiles.writes() != profileWritesAtEntry)
    {
        shell.messageBox().open(std::u16string(), MessageBoxPage::Kind::Saving, *this);
        return;
    }
    play(menu::UiEvent::Back);
    shell.slideOut(menu::UiEvent::LeftBack);
    sources.door.openMenu();
}

void OptionsPage::messageClosed(bool)
{
    sources.profiles.saveCurrent();
    profileWritesAtEntry = sources.profiles.writes();
    leaveToMain();
}

void OptionsPage::openRow()
{
    FeShell &fe = shell;
    switch (selectedRow())
    {
    case kGameSettings:
        fe.gameSettings().open();
        break;
    case kSoundSettings:
        fe.soundSettings().open();
        break;
    case kTrackRecords:
        fe.trackRecords().open();
        break;
    case kHighScores:
        fe.highScores().open();
        break;
    case kGhostData:
        fe.ghostData().open();
        break;
    case kCredits:
        fe.credits().open();
        break;
    case kEasterEggs:
        fe.easterEggs().open();
        break;
    case kCheats:
        fe.cheatMenu().open();
        break;
    default:
        break;
    }
}

// the page's model (0x2d830): the main menu's Options item (shell +0x1ff8) at (-4, -1, 0), camera 1; drawn by
// drawMenu's scene callback (app_frame.cpp). Rows: the large rows (font 1 at 0.75, as the main menu's) at y 100 + 30 i;
// a list too long for the room above the ticker shows a window of it (ui\menu_list.hpp), each slot sliding in on its own
menu::ListLayout OptionsPage::listLayout() const
{
    return menus.listLayout(menu::kLargeRows, 100.f, kTickerY - 2.f);
}

void OptionsPage::followCursor()
{
    const int rows = int(shownRows().size());
    window.follow(selected, rows, std::min(kRows, menu::rowsThatFit(listLayout(), rows)));
}

void OptionsPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const menu::ListLayout layout = listLayout();
    const std::vector<int> rows = shownRows();
    for (int i = window.first(); i < window.end(); ++i)
    {
        const OptionsRow &row = kOptionsRows[rows[size_t(i)]];
        const int slot = i - window.first();
        menus.drawMenuRow(canvas,
                        {atLeft(originX, rowX[slot]), layout.rowY(slot)},
                        strings.frontEnd(row.str, row.fb),
                        menus.listRowLook(menu::kLargeRows, i == selected, kText));
    }
    const float firstX = atLeft(originX, rowX[0]), lastX = atLeft(originX, rowX[window.end() - window.first() - 1]);
    MenuKit::drawScrollMarks(canvas, {firstX + menu::kScrollMarkIndent, lastX + menu::kScrollMarkIndent}, layout, window);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
}

// ---- Game Settings / Sound Settings (the settings list engine)

SettingsPage::SettingsPage(const PageKit &kit, const OptionSources &sources, const Place &place)
    : FePage(kit), sources(sources), page(place.page), list(place.list), parentRow(place.parentRow), titleId(place.titleId),
      titleFallback(place.titleFallback)
{
}

std::vector<SettingRow> &SettingsPage::entries() const
{
    return sources.settingsMenu.rows(list);
}

// enter (0xac6c0 / 0xaca90): the cursor on the last entry (the top row)
void SettingsPage::open()
{
    sources.settingsStore.buildSettings();
    shell.goTo(page);
    selected = entryCount() - 1;
    followCursor();
}

void SettingsPage::resumeAt(int type)
{
    shell.goTo(page);
    const auto &rows = entries();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (rows[i].type == type)
        {
            selected = int(i);
        }
    }
    followCursor();
}

int SettingsPage::entryCount() const
{
    return int(entries().size());
}

std::u16string SettingsPage::title() const
{
    return strings.frontEnd(titleId, titleFallback);
}

std::optional<std::u16string> SettingsPage::ticker() const
{
    const auto &rows = entries();
    if (rows.empty())
    {
        return std::u16string();
    }
    return SettingsMenu::description(rows[size_t(std::clamp(selected, 0, entryCount() - 1))], strings);
}

// input (0xac040): up = the next entry (the row above; past the top: Exit at the bottom), down = the previous;
// left / right step the option (no wrap); A on Exit back, on Reset to Default asks fe 0x341, on an entry without
// options runs its action; B back to FE OPTIONS (Argon Options' lists: to Argon Options, without the changes Save did
// not keep)
void SettingsPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        if (isArgonSettings())
        {
            sources.settingsStore.buildArgonSettings();   // back to what Save kept
        }
        leavePage();
        return;
    }
    if (menus.menuCursor(selected, entryCount(), in, menu::ListOrder::BottomUp))   // the list is drawn bottom-up
    {
        play(menu::UiEvent::Cursor);
        followCursor();
    }
    previewEngineVolume();
    if (!entries()[size_t(selected)].opts.empty())
    {
        changeOption(in);
    }
    else if (in.a)
    {
        runEntry();
    }
}

// 0xac040 after a cursor move: on Engine Volume (type 0xb) the engine test sound plays and the music is muted
// (0x2ea60), anywhere else it stops (0x2eb00; the game fades it over 0.5 s)
void SettingsPage::previewEngineVolume()
{
    if (entries()[size_t(selected)].type == SettingsMenu::kEngineVolume)
    {
        audio.playEngineTest(sources.gameFolder);
    }
    else
    {
        audio.stopEngineTest();
    }
}

void SettingsPage::messageClosed(bool yes)
{
    if (!yes)
    {
        return;
    }
    play(menu::UiEvent::Pick);   // 0xac725
    SettingsMenu::resetToDefaults(entries());
    if (!isArgonSettings())   // Argon Options: Save applies and keeps them
    {
        sources.settingsStore.storeSettings();
    }
}

void SettingsPage::changeOption(const menu::MenuInput &in)
{
    SettingRow &row = entries()[size_t(selected)];
    if (!sources.settingsMenu.isChangeable(row.type))
    {
        return;
    }
    int chosen = row.chosen;
    if (in.left && chosen > 0)
    {
        --chosen;
    }
    if (in.right && chosen + 1 < int(row.opts.size()))
    {
        ++chosen;
    }
    if (chosen == row.chosen)
    {
        return;
    }
    if (row.type != SettingsMenu::kEngineVolume)
    {
        play(menu::UiEvent::Cursor);   // Engine Volume: its test sound instead (0x2ea60)
    }
    if (isArgonSettings())
    {
        sources.settingsMenu.select(row.type, chosen);   // Save applies and keeps it
        return;
    }
    sources.settingsStore.changeSetting(row.type, chosen);
}

void SettingsPage::runEntry()
{
    int type = entries()[size_t(selected)].type;
    if (type == kExit)
    {
        leavePage();
    }
    else if (type == SettingsMenu::kSave)
    {
        saveArgonSettings();
    }
    else if (type == kMusicOptions)
    {
        play(menu::UiEvent::LeftForward);
        shell.musicSelect().openAt(0);   // Music Options (0x2ebd0)
    }
    else if (type == kResetToDefault)
    {
        shell.messageBox().open(resetQuestion(), MessageBoxPage::Kind::YesNo, *this);
    }
}

// the game's question on its lists; Argon Options' pages say that only the page's rows go back
std::u16string SettingsPage::resetQuestion() const
{
    if (isArgonSettings())
    {
        return strings.viewer(viewer_text::kResetPageQuestion);
    }
    return strings.frontEnd(0x341, "Are you sure you want to reset all options to their default setting?");
}

// all of Argon Options' lists applied and written, then back to Argon Options; a new Renderer first says it needs a
// restart
void SettingsPage::saveArgonSettings()
{
    const bool needsRestart = sources.settingsStore.chosenRendererNeedsRestart();
    sources.settingsStore.storeArgonSettings();
    if (needsRestart)
    {
        shell.messageBox().open(strings.viewer(viewer_text::kRendererRestart), MessageBoxPage::Kind::Info, restartNotice);
        return;
    }
    leavePage();
}

// its rows change nothing until Save applies and writes them (the game's lists apply and write each change)
bool SettingsPage::isArgonSettings() const
{
    return isArgonList(list);
}

// back to the page this list is a row of, the cursor on that row
void SettingsPage::leavePage()
{
    play(menu::UiEvent::LeftBack);
    audio.stopEngineTest();
    if (isArgonSettings())
    {
        shell.argonOptions().openAt(ArgonOptionsPage::Row(parentRow));
        return;
    }
    shell.options().openAt(parentRow);
}

// rows (0xabca0): label at (62, 100 + 20 x (count - i)): the last entry at the top, y 120; panel (the cursor's), font 0
// at 1.0; here the small rows, 24 apart. A list too long for the room above the ticker shows a window of it
// (ui\menu_list.hpp)
menu::ListLayout SettingsPage::listLayout() const
{
    return menus.listLayout(menu::kSmallRows, 120.f, kTickerY - 2.f);
}

// the window over the rows (top first: row n - 1 - entry) following the cursor
void SettingsPage::followCursor()
{
    const int entries = entryCount();
    window.follow(entries - 1 - selected, entries, menu::rowsThatFit(listLayout(), entries));
}

void SettingsPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const menu::ListLayout layout = listLayout();
    const int entries = entryCount();
    for (int row = window.first(); row < window.end(); ++row)
    {
        drawEntry(canvas, entries - 1 - row, layout.rowY(row - window.first()));
    }
    const float markX = atLeft(originX, kRowX) + menu::kScrollMarkIndent;   // these rows do not slide
    MenuKit::drawScrollMarks(canvas, {markX, markX}, layout, window);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), rightHint()});
}

// one entry's row at y: its options from x 300, each its text width + 10 apart, the chosen one 0xff85dde5, the others
// 0xff555555. A row of one option at a time (Argon Options' Resolution) shows the chosen one, "<" before it when there
// are more to the left, ">" after when there are more to the right
void SettingsPage::drawEntry(const FeCanvas &canvas, int entry, float y) const
{
    const float originX = canvas.originX();
    const SettingRow &row = entries()[size_t(entry)];
    const uint32_t chosenColour = sources.settingsMenu.isChangeable(row.type) ? kText : kGrey;
    const float textY = menus.drawMenuRow(
        canvas, {atLeft(originX, kRowX), y}, SettingsMenu::label(row, strings), menus.listRowLook(menu::kSmallRows, entry == selected, chosenColour));
    if (row.oneAtATime)
    {
        canvas.text({originX + 300.f, textY}, chosenWithArrows(row.opts, row.chosen), {0, chosenColour, 0, kFe, kFe});
        return;
    }
    float x = 300.f;
    for (size_t o = 0; o < row.opts.size(); ++o)
    {
        canvas.text({originX + x, textY}, row.opts[o], {0, int(o) == row.chosen ? chosenColour : kGrey, 0, kFe, kFe});
        x += painter.textWidth(row.opts[o], 0, kFe) / kFe + 10.f;   // widths measured at the game's scale (0x81ee0)
    }
}

std::u16string SettingsPage::chosenWithArrows(const std::vector<std::u16string> &options, int chosen)
{
    if (chosen < 0 || chosen >= int(options.size()))
    {
        return {};
    }
    const std::u16string left = chosen > 0 ? u"< " : u"";
    const std::u16string right = chosen + 1 < int(options.size()) ? u" >" : u"";
    return left + options[size_t(chosen)] + right;
}

std::u16string SettingsPage::rightHint() const
{
    const auto &rows = entries();
    if (rows.empty())
    {
        return {};
    }
    bool hasOptions = !rows[size_t(std::clamp(selected, 0, entryCount() - 1))].opts.empty();
    return hasOptions ? strings.frontEnd(0x48, "[BUTLFIX][BUTRFIX] - Change Settings") : strings.frontEnd(0x32, "Select - [BUTA]");
}

// ---- FE MUSICSELECT (build 0x2b920, enter 0x2c240, input 0x2bd80, update 0x2c430, ticker 0x2c660)
// rows: Music Source (fe 0x259, (62, 120), choices Default 0x25a at x 62 + 200 / Custom 0x25b at 62 + 300), Edit
// Custom Playlist (0x25c, (62, 140), A -> FE LEVELMUSIC with Custom on), Exit (8, (62, 160)). Custom needs at least
// 2 ripped soundtracks on the Xbox (0x440e74); without them (always, in the viewer) Custom is hidden, Default and Edit
// Custom Playlist grey, the ticker fe 0x3ea, the right hint only on Exit. B / Exit: back to Sound Settings.

void MusicSelectPage::openAt(int row)
{
    selected = row;
    shell.goTo(menu::PageMusicSelect);
}

std::u16string MusicSelectPage::title() const
{
    return strings.frontEnd(0x258, "Music");
}

bool MusicSelectPage::canEditPlaylist() const
{
    return sources.music.available() && sources.music.customChosen();
}

// with ripped music: left / right on Music Source switches Default / Custom (0x2bd80); A on Edit Custom Playlist
// (Custom only) opens FE LEVELMUSIC
void MusicSelectPage::input(const menu::MenuInput &in)
{
    sources.musicStore.scanCustomMusic();
    if (menus.menuCursor(selected, kRowCount, in))
    {
        play(menu::UiEvent::Cursor);
    }
    if ((in.left || in.right) && selected == kMusicSource && sources.music.available())
    {
        play(menu::UiEvent::Cursor);
        sources.music.chooseCustom(!sources.music.customChosen());
        shell.restartTicker();
        sources.musicStore.storeMusic();
    }
    if (in.a && selected == kEditPlaylist)
    {
        editPlaylist();
        return;
    }
    if (in.b || (in.a && selected == kExit))
    {
        play(menu::UiEvent::LeftBack);
        shell.soundSettings().resumeAt(SettingsPage::kMusicOptions);
    }
}

// A on Edit Custom Playlist: FE LEVELMUSIC with Custom on, else the bad-select sound
void MusicSelectPage::editPlaylist()
{
    if (!canEditPlaylist())
    {
        play(menu::UiEvent::Refused);
        return;
    }
    play(menu::UiEvent::LeftForward);
    shell.levelMusic().open();
}

void MusicSelectPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const int labels[kRowCount] = {0x259, 0x25c, 0x8};
    const char *fallbacks[kRowCount] = {"Music Source", "Edit Custom Playlist", "Exit"};
    for (int i = 0; i < kRowCount; ++i)
    {
        uint32_t colour = i == kEditPlaylist && !canEditPlaylist() ? kGrey : kText;
        const float rowY = 120.f + menu::kSmallRows.step * float(i);
        float y = menus.drawMenuRow(
            canvas, {atLeft(originX, kRowX), rowY}, strings.frontEnd(labels[i], fallbacks[i]), menus.listRowLook(menu::kSmallRows, i == selected, colour));
        if (i == kMusicSource)
        {
            drawSourceChoices(canvas, y);
        }
    }
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), rightHint()});
}

// Default at 62 + 200, Custom at 62 + 300 (hidden without ripped music)
void MusicSelectPage::drawSourceChoices(const FeCanvas &canvas, float y) const
{
    const float originX = canvas.originX();
    bool available = sources.music.available();
    canvas.text({atLeft(originX, 262.f), y}, strings.frontEnd(0x25a, "Default"), {0, available && !sources.music.customChosen() ? kText : kGrey, 0, kFe, kFe});
    if (available)
    {
        canvas.text({atRight(originX, 362.f), y}, strings.frontEnd(0x25b, "Custom"), {0, sources.music.customChosen() ? kText : kGrey, 0, kFe, kFe});
    }
}

std::u16string MusicSelectPage::rightHint() const
{
    if (selected == kExit || (selected == kEditPlaylist && canEditPlaylist()))
    {
        return strings.frontEnd(0x32, "Select - [BUTA]");
    }
    if (selected == kMusicSource && sources.music.available())
    {
        return strings.frontEnd(0x48, "[BUTLFIX][BUTRFIX] - Change Settings");
    }
    return {};
}

std::optional<std::u16string> MusicSelectPage::ticker() const
{   // 0x2c660
    if (selected == kExit)
    {
        return strings.frontEnd(0x3ec, "Return to Sound Settings.");
    }
    if (!sources.music.available())
    {
        return strings.frontEnd(0x3ea, "You have no ripped music available on your Xbox.");
    }
    if (selected == kEditPlaylist)
    {
        return strings.frontEnd(0x3eb, "Create or edit your music playlist.");
    }
    return sources.music.customChosen() ? strings.frontEnd(0x3e9, "Play random music from your ripped tracks.")
                                        : strings.frontEnd(0x3e8, "Play music from the Junkie XL soundtrack.");
}
