// Argon: the in-race pause menu, the game's PAUSEMENU object (pausemenu.cpp; vtable 0x19c5e0: ctor
// 0x88930, init 0x88ab0, update / messages 0x898e0, draw 0x89860; cursor up 0x8bad0 / down 0x8bc80, item positions
// 0x8be20, volume rows 0x8b800); docs\frontend\race-end.md "Pause menu". Texts are race_strings.
#include "ui/pause.hpp"

#include "ui/page_hosts.hpp"

#include "audio/race_sfx.hpp"
#include "audio/sound_ids.hpp"
#include "files/string_table.hpp"
#include "render/pause_background.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/page_id.hpp"
#include "ui/race_menus.hpp"
#include "ui/viewer_text.hpp"

using namespace menu::race;

// "PAUSE ON": the main page, the cursor on Continue; a "Saving Settings" in progress is dropped (a controller pulled out)
void PauseMenu::reset()
{
    page = Page::Main;
    selected = kContinue;
    saving.stop();
    followCursor();
}

void PauseMenu::showOptions()
{
    page = Page::Options;
    selected = kExit;
    followCursor();
}

// message 9 on Restart / Track Select / Main Menu (0x89c3c): +0x60c = 0xc, the hud cursor on No (+0x1aa4)
void PauseMenu::askToConfirm(Item action)
{
    confirmAction = action;
    page = Page::Confirm;
    selected = kNo;
    followCursor();
}

void PauseMenu::startSaving()
{
    settingChanged = true;
    leaveBy(kContinue);
}

// the saving branch of 0x898e0 (0x89f42): "Saving Settings" shown, the hint and the cursor hidden until the second is over;
// then the flags the item set run (+0x628 resume, +0x624 / +0x625 / +0x626 / +0x627 the GAMELOGIC messages)
void PauseMenu::stepSaving(double now)
{
    if (!saving.over(now))
    {
        return;
    }
    saving.stop();
    finishLeaving(leavingBy);
}

// the items shown, top to bottom (0x8be20: x 320, y 116 + 35 n)
std::vector<PauseMenu::Item> PauseMenu::items() const
{
    if (page == Page::Options)
    {
        return optionItems();
    }
    if (page == Page::Confirm)
    {
        return {kYes, kNo};
    }
    return mainItems();
}

// Continue, Options, Restart, Replay Loaded Ghost / Replay Session Ghost (Time Attack with that ghost, 0x52afaf /
// 0x52afae, 0x898e0), Track Select (not in Tournament: g_game_mode 0), Main Menu; the cursor's links 0x8bad0 /
// 0x8bc80 skip the hidden ones the same way
std::vector<PauseMenu::Item> PauseMenu::mainItems() const
{
    std::vector<Item> items = {kContinue, kOptions, kRestart};
    const TimeAttackLogic &ta = sources.modes.timeAttack();
    if (sources.modes.current().hasGhosts() && ta.loadedGhost().valid())
    {
        items.push_back(kReplayLoadedGhost);
    }
    if (sources.modes.current().hasGhosts() && ta.hasSessionGhost())
    {
        items.push_back(kReplaySessionGhost);
    }
    if (!sources.modes.tournamentSelected())
    {
        items.push_back(kTrackSelect);
    }
    items.push_back(kMainMenu);
    return items;
}

// Exit and the three volumes (0x8b6c0); the viewer adds HUD, HUD Size and, racing alone, player 1's vibration
// (docs\argon\differences.md)
std::vector<PauseMenu::Item> PauseMenu::optionItems() const
{
    std::vector<Item> items = {kExit, kSfxVolume, kMusicVolume, kEngineVolume, kHud, kHudSize};
    if (!pads.multiplayerRace())
    {
        items.push_back(kVibration);
    }
    return items;
}

// the Game / Sound Settings entry types; vibration: the entry of player 1's port (Controller 1..4 Vibration, 2..5)
int PauseMenu::settingType(Item item) const
{
    switch (item)
    {
    case kSfxVolume:
        return SettingsMenu::kSfxVolume;
    case kMusicVolume:
        return SettingsMenu::kMusicVolume;
    case kEngineVolume:
        return SettingsMenu::kEngineVolume;
    case kHud:
        return SettingsMenu::kHud;
    case kHudSize:
        return SettingsMenu::kHudSize;
    default:
        return SettingsMenu::kVibrationFirst + std::clamp(pads.padOfPlayer(0), 0, 3);
    }
}

bool PauseMenu::isSettingRow(Item item)
{
    return (item >= kSfxVolume && item <= kEngineVolume) || item == kHud || item == kHudSize || item == kVibration;
}

std::u16string PauseMenu::settingName(Item item) const
{
    switch (item)
    {
    case kSfxVolume:
        return strings.race(0x402, "SFX Volume");
    case kMusicVolume:
        return strings.race(0x403, "Music Volume");
    case kEngineVolume:
        return strings.race(0x404, "Engine Volume");
    case kHud:
        return strings.frontEnd(0x336, "HUD");
    case kHudSize:
        return strings.viewer(viewer_text::kHudSize.label);   // no string of the game's names it
    default:
        return strings.frontEnd(0x321 + settingType(item) - 2, "Controller Vibration");
    }
}

// 0x8b800: "%s: < %d >" (a volume's option texts are its digits)
std::u16string PauseMenu::settingLabel(Item item) const
{
    return settingName(item) + argon::widenLatin1(": < ") + sources.settingsMenu.optionText(settingType(item)) + argon::widenLatin1(" >");
}

std::u16string PauseMenu::label(Item item) const
{
    switch (item)
    {
    case kContinue:
        return strings.race(0x3ed, "Continue");
    case kReplayLoadedGhost:
        return strings.race(0x3ee, "Replay Loaded Ghost");
    case kReplaySessionGhost:
        return strings.race(0x3ef, "Replay Session Ghost");
    case kTrackSelect:
        return strings.race(0x3fb, "Track Select");
    case kMainMenu:
        return strings.race(0x3fc, "Main Menu");
    case kOptions:
        return strings.race(0x3f0, "Options");
    case kExit:
        return strings.race(0x3f5, "Exit");
    case kSfxVolume:
    case kMusicVolume:
    case kEngineVolume:
    case kHud:
    case kHudSize:
    case kVibration:
        return settingLabel(item);
    case kRestart:
        return strings.race(0x3f1, "Restart");
    case kYes:
        return strings.race(0x40a, "Yes");
    default:
        return strings.race(0x40b, "No");
    }
}

// sound_play_ui 0xc3ca0 (id, DAT_00448f58, 0): the race's generic list, which plays on while the race's voices hold
void PauseMenu::play(menu::PauseSound sound) const
{
    if (sound == menu::PauseSound::Navigate)
    {
        audio.raceSfx().playOnListener(argon::GenericSound::MenuNavigate);
    }
    if (sound == menu::PauseSound::Select)
    {
        audio.raceSfx().playOnListener(argon::GenericSound::MenuSelect);
    }
}

// 0x898e0 messages: 5 / 6 up / down move through the page's items (wrapping) with 0x17 (0x8bad0 / 0x8bc80); 7 / 8
// left / right on a volume row step it 0..7 (the Sound Settings entries, saved with the profile) with 0x17 when it
// moved; 9 A runs the item with 0x18 (none on a volume row). The viewer's HUD and vibration rows step the same way
void PauseMenu::input(const menu::MenuInput &in)
{
    takePresses(in);
    followCursor();
}

void PauseMenu::takePresses(const menu::MenuInput &in)
{
    if (in.b)
    {
        back();
        return;
    }
    moveCursor(in);
    changeSetting(in);
    if (in.a)
    {
        play(menu::pickSound(isSettingRow(selected)));
        runItem();
    }
}

// the items' rows (0x8be20: y 116 + 35 n, font 1 at 0.7; the confirm page's from 186): here the large rows, as the
// front end's menus, at the game's step, above the hints; a page with more items than fit shows a window of them
// (ui\menu_list.hpp)
menu::ListLayout PauseMenu::listLayout() const
{
    return menus.listLayout(menu::kLargeRows.spacedBy(35.f), page == Page::Confirm ? 186.f : 116.f, kHintY - 5.f);
}

void PauseMenu::followCursor()
{
    const std::vector<Item> shown = items();
    const int rows = int(shown.size());
    const int at = int(std::find(shown.begin(), shown.end(), selected) - shown.begin());
    window.follow(std::min(at, rows - 1), rows, menu::rowsThatFit(listLayout(), rows));
}

// B: the game's gamelogic never passes B to PAUSEMENU (0x3bf60 sends only 5 / 6 / 7 / 8 up / down / left / right and 9
// A), so B does nothing there. The viewer lets B step back one page like Exit / No (to the main page on the item that
// opened it, with their 0x18) and ignores it on the main page.
void PauseMenu::back()
{
    if (page == Page::Main)
    {
        return;
    }
    selected = page == Page::Options ? kOptions : confirmAction;
    page = Page::Main;
    play(menu::PauseSound::Select);
}

void PauseMenu::moveCursor(const menu::MenuInput &in)
{
    if (!in.up && !in.down)
    {
        return;
    }
    const std::vector<Item> shown = items();
    const int size = int(shown.size());
    int at = 0;
    for (int i = 0; i < size; ++i)
    {
        if (shown[size_t(i)] == selected)
        {
            at = i;
        }
    }
    selected = shown[size_t(in.down ? (at + 1) % size : (at + size - 1) % size)];
    play(menu::PauseSound::Navigate);
}

void PauseMenu::changeSetting(const menu::MenuInput &in)
{
    if (!(in.left || in.right) || !isSettingRow(selected))
    {
        return;
    }
    const int type = settingType(selected);
    const int before = sources.settingsMenu.current(type);
    const int after = menu::steppedOption(before, in.right, sources.settingsMenu.optionCount(type));
    play(menu::optionSound(before, after));
    if (after != before)
    {
        sources.settingsStore.changeSetting(type, after);   // the viewer writes each change at once
        settingChanged = settingChanged || !sources.settingsMenu.isViewerSetting(type);   // 0x584339; settings.json is no profile
    }
}

void PauseMenu::runItem()
{
    switch (selected)
    {
    case kContinue:
    case kReplayLoadedGhost:
    case kReplaySessionGhost:
    case kYes:
        leaveBy(selected);
        break;
    case kOptions:
        showOptions();
        break;
    case kExit:
        reset();
        break;                                       // 0x8b7d0
    case kTrackSelect:
    case kMainMenu:
    case kRestart:
        askToConfirm(selected);
        break;
    case kNo:
        page = Page::Main;
        selected = confirmAction;
        break;
    default:
        break;
    }
}

// Continue (0x89bd6), the ghost replays (0x8b560), Yes (Restart 0x8b6f0, Track Select 0x8b630, Main Menu 0x8b5b0): with a
// volume changed, "Saving Settings" first; the game drops its own pause flag at once (0x782d0) but sends PAUSE OFF and the
// GAMELOGIC message only once the save is over, so the race stays held behind the menu
void PauseMenu::leaveBy(Item item)
{
    if (!settingChanged)
    {
        finishLeaving(item);
        return;
    }
    settingChanged = false;
    leavingBy = item;
    saving.start(glfwGetTime());
}

void PauseMenu::finishLeaving(Item item)
{
    if (item == kContinue)
    {
        pauseHost.resumeRace();   // 0x8b470
        return;
    }
    if (item == kYes)
    {
        confirmed();
        return;
    }
    replayGhost();
}

// 0x8b560: the pause closes (0x782d0) and GAMELOGIC 3 (the loaded ghost) / 4 (the session ghost) replays its lap
void PauseMenu::replayGhost()
{
    const int ghost = selected == kReplayLoadedGhost ? 0 : 1;
    pauseHost.resumeRace();
    pauseHost.startGhostReplay(ghost);
}

void PauseMenu::confirmed()
{
    const Item action = confirmAction;
    pauseHost.resumeRace();
    if (action == kRestart)
    {
        host.restartRace();
    }
    else
    {
        leaveRace();
    }
}

// Track Select / Main Menu; in Time Attack with a session ghost FE GHOSTSAVE first (0x8b5b0 / 0x8b630)
void PauseMenu::leaveRace()
{
    const int next = confirmAction == kTrackSelect ? menu::PageCourseSelect : menu::PageMain;
    if (!sources.modes.current().hasGhosts() || !sources.modes.timeAttack().hasSessionGhost())
    {
        host.quitRaceTo(next);
        return;
    }
    pauseHost.openGhostSave(next, sources.modes.timeAttack().takeSessionGhost());
}

// pausemenu_draw 0x89860 -> 0x7e5e0 (0, 1.75): the race frame blurred, then the menu bars and logo (0xc2870). init
// 0x88ab0: the title "Paused" (0x3ec) font 1 right-aligned at (568 = 630 - 52 - 10, 36); the items font 1 x 0.7 centred
// on 320; the confirm page's "Are you sure?" (0x40c) at y 116, Yes / No at 186 / 221; hint "Select - [BUTA]" (0x408)
// font 0 right-aligned at (568, 421), or "Saving Race" (0x520; the viewer: "Saving Settings") left-aligned at (62, 421) in its place. Every item has
// the panel flag (+0x1cc |= 2); the hud cursor (+0x28fc: mode 0, margins 4 / 0) draws nothing and only marks the
// selected item, whose panel is then highlighted and filled (0xad620), as the front end's rows
void PauseMenu::draw(const FeCanvas &canvas)
{
    ImDrawList *drawList = canvas.drawList();
    const float originX = canvas.originX();
    sources.background.draw(drawList);
    host.drawMenuBars(canvas, 640.f + 2.f * originX);
    canvas.text({atRight(originX, kRight), 36.f}, strings.race(0x3ec, "Paused"), {1, kText, 1, kFe, kFe});
    const float scale = 0.7f * kFe;
    if (page == Page::Confirm)
    {
        canvas.text({originX + 320.f, 116.f}, strings.race(0x40c, "Are you sure?"), {1, kText, 2, scale, scale});
    }
    const std::vector<Item> shown = items();
    const menu::ListLayout layout = listLayout();
    const int end = std::min(window.end(), int(shown.size()));   // the items may change between presses (a ghost saved)
    for (int i = window.first(); i < end; ++i)
    {
        const Item item = shown[size_t(i)];
        menus.drawMenuRow(
            canvas, {originX + 320.f, layout.rowY(i - window.first())}, label(item), menus.listRowLook(menu::kLargeRows, item == selected, kText).alignedTo(2));
    }
    MenuKit::drawScrollMarks(canvas, {originX + 320.f, originX + 320.f}, layout, window);
    const std::u16string savingText =
        isSaving() ? strings.viewer(viewer_text::kSavingSettings) : std::u16string();   // the game: "Saving Race" (docs\argon\differences.md)
    menus.drawHints(canvas, {savingText, hint(), 1.f});   // y as in the game's picture
}

// +0x1480: shown while a page is up and no save runs (0x8b0a6)
std::u16string PauseMenu::hint() const
{
    return isSaving() ? std::u16string() : strings.race(0x408, "Select - [BUTA]");
}
