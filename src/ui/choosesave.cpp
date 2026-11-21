// Argon: profiles. FE CHOOSESAVE (vtable 0x19e090: build 0x1b440, enter 0x1c850, update 0x1bab0, input
// 0x1c070, draw 0x1be00, list 0x1cb30); docs\frontend\profiles.md "Profiles". FE NAME ENTRY: ui\keyboard.cpp.
// The game keeps a profile as an Xbox save; the viewer keeps one JSON file per profile in <game>\profiles (app_profile.cpp).
#include "audio/audio.hpp"
#include "files/string_table.hpp"
#include "hud/hud_painter.hpp"
#include "ui/fe_shell.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_kit.hpp"
#include "ui/page_id.hpp"
#include "ui/fe_shell.hpp"
#include "ui/menu_common.hpp"
#include "ui/picture_quad.hpp"

using namespace menu::fe;

namespace
{
constexpr float kListX = 52.f, kListW = 536.f;   // the header bar and the list frame (and the card's viewport)
}  // namespace

// enter (0x1c850): title fe 0x64 "Select Profile", hints Back / Select and [BUTY] Create New Profile / Delete Profile
// [BUTX], ticker fe 0x6a. The list (0xbfb60 with its last flag set) is "Create New Profile" (fe 0x6d) and then the
// saves; the cursor on the first save when there is one (0x1cef0), here on the current profile
void ChooseSavePage::open()
{
    if (!shell.isOpen())
    {
        door.openMenu();
    }
    profiles.load();
    shell.goTo(menu::PageChooseSave);
    firstShown = 0;
    selected = profileCount() > 0 ? rowOfProfile(0) : kCreateRow;
    for (int i = 0; i < profileCount(); ++i)
    {
        if (profiles.all()[size_t(i)].name == profiles.currentName())
        {
            showRow(rowOfProfile(i));
        }
    }
}

std::u16string ChooseSavePage::title() const
{
    return strings.frontEnd(0x64, "Select Profile");
}

std::optional<std::u16string> ChooseSavePage::ticker() const
{
    return strings.frontEnd(0x6a, "Your profile is your personal racing dossier.");
}

int ChooseSavePage::profileCount() const
{
    return int(profiles.all().size());
}

int ChooseSavePage::rowCount() const
{
    return rowOfProfile(profileCount());
}

int ChooseSavePage::rowOfProfile(int profile)
{
    return profile + 1;
}

int ChooseSavePage::profileOfRow(int row)
{
    return row - 1;
}

void ChooseSavePage::showRow(int row)
{
    selected = row;
    firstShown = std::clamp(firstShown, std::max(0, row - (kRowsShown - 1)), row);
}

// input (0x1c070): up / down move (the list scrolls past 5); A on "Create New Profile" or Y creates one (FE NAME
// ENTRY), A on a profile loads it (an invalid one: fe 0x1f4); X on a profile asks to delete it (fe 0x67 + name);
// B back to FE TITLE. Leaving slides the title and hints out, the next page 0.1 s later (0x1bab0 cases 4 / 7)
void ChooseSavePage::input(const menu::MenuInput &in)
{
    if (in.b)
    {   // 0x1bab0 case 4 with +0x2486: FE TITLE
        play(menu::UiEvent::Back);
        shell.slideOut(menu::UiEvent::LeftBack);
        shell.title().open();
        return;
    }
    if (in.y || (in.a && selected == kCreateRow))
    {
        createProfile();
        return;
    }
    moveCursor(in);
    if (selected == kCreateRow)
    {
        return;
    }
    if (in.x)
    {
        askToDelete(profileOfRow(selected));
    }
    else if (in.a)
    {
        loadProfile(profileOfRow(selected));
    }
}

void ChooseSavePage::moveCursor(const menu::MenuInput &in)
{
    int at = selected;
    if (in.up && at > 0)
    {
        --at;
    }
    if (in.down && at + 1 < rowCount())
    {
        ++at;
    }
    if (at != selected)
    {
        play(menu::UiEvent::Cursor);
    }
    showRow(at);
}

void ChooseSavePage::createProfile()
{
    play(menu::UiEvent::Forward);
    shell.slideOut(menu::UiEvent::LeftForward);
    shell.nameEntry().openForName();
}

void ChooseSavePage::askToDelete(int profile)
{
    deleteTarget = profile;
    shell.messageBox().open(strings.frontEnd(0x67, "Are you sure you want to delete the profile: ") + profiles.all()[size_t(profile)].name + u"?",
                            MessageBoxPage::Kind::YesNo,
                            *this);
}

void ChooseSavePage::messageClosed(bool yes)
{
    if (!yes || deleteTarget < 0 || deleteTarget >= profileCount())
    {
        return;
    }
    profileKeeper.deleteProfile(size_t(deleteTarget));
    const int at = std::min(selected, rowCount() - 1);
    firstShown = std::min(firstShown, std::max(0, rowCount() - kRowsShown));
    showRow(at);
}

void ChooseSavePage::loadProfile(int profile)
{
    const argon::Profile &shownProfile = profiles.all()[size_t(profile)];
    if (!shownProfile.valid)
    {
        shell.messageBox().open(strings.frontEnd(0x1f4, "Unable to load the profile."), MessageBoxPage::Kind::Info);
        return;
    }
    profileKeeper.useProfile(shownProfile);
    play(menu::UiEvent::Forward);
    shell.slideOut(menu::UiEvent::LeftForward);
    door.openMenu();
}

void ChooseSavePage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    drawIdCard(canvas);
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, kListX, kListW), 160.f, boxWidth(originX, kListX, kListW), 20.f}, FrameShade::Red);
    MenuKit::drawGradientFrame(canvas, {boxLeft(originX, kListX, kListW), 180.f, boxWidth(originX, kListX, kListW), 180.f});
    drawHeader(canvas);
    for (int row = firstShown; row < firstShown + kRowsShown && row < rowCount(); ++row)
    {
        drawRow(canvas, row);
    }
    drawScrollArrows(canvas);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), strings.frontEnd(0x32, "Select - [BUTA]")});
    menus.drawHints(
        canvas, {strings.frontEnd(0x34, "[BUTY] - Create New Profile"), strings.frontEnd(0x35, "Delete Profile - [BUTX]"), kHintScale, kHintY + 24.f});
}

// draw 0x1be00: first the ID card, data\frontend\profile\tex0.dds (shell +0x585c, loaded 0x30ef9), through
// hud_draw_rotated_quad 0xcbe50 (320, 260, -20 deg, -160, 160, 640, colour white), additive, clipped by a viewport
// (52, 160) 536 x 200; then a header bar (52, 160) 536 x 20 and the list frame (52, 180) 536 x 180 over it
void ChooseSavePage::enter()
{
    if (!painter.texture("profile_card"))
    {
        painter.loadTexture("profile_card", gameFolder + "/data/frontend/profile/tex0.dds");
    }
}

void ChooseSavePage::drawIdCard(const FeCanvas &canvas) const
{
    const PictureQuad card{painter.texture("profile_card"), {{320.f, 260.f}, -20.f, -160.f, 160.f, 640.f}, ImVec4(kListX, 160.f, kListW, 200.f)};
    drawPictureQuad(painter, canvas, card);
}

// build 0x1b440: fe 0x68 "Select Profile" at (62 + 50, 160), fe 0x6c "Percentage Complete" right at 568, orange
// centred in the 20 high header bar, as the rows in their panels
void ChooseSavePage::drawHeader(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    float y = 160.f + (20.f - menus.fontLine(0) * kFe) * 0.5f;
    canvas.text({atLeft(originX, 112.f), y}, strings.frontEnd(0x68, "Select Profile"), {0, kOrange, 0, kFe, kFe});
    canvas.text({atRight(originX, kRight), y}, strings.frontEnd(0x6c, "Percentage Complete"), {0, kOrange, 1, kFe, kFe});
}

// rows (update 0x1bab0, list 0x1cb30): name at (112, 210 + 25 i), "%d%%" right-aligned at 568 (none on "Create New
// Profile"); the selected one x 1.5 at y 205 + 25 i. The name element has a panel (build 0x1b440: flag 2), the list
// cursor's: as the menu rows
void ChooseSavePage::drawRow(const FeCanvas &canvas, int row) const
{
    const float originX = canvas.originX();
    const bool sel = row == selected;
    const int shownRow = row - firstShown;
    const float es = sel ? 1.5f : 1.f, s = es * kFe, y = (sel ? 205.f : 210.f) + 25.f * float(shownRow);
    if (row == kCreateRow)
    {
        menus.drawMenuRow(canvas, {atLeft(originX, 112.f), y}, strings.frontEnd(0x6d, "Create New Profile"), {0, s, menus.fontLine(0) * es, sel, kText});
        return;
    }
    const argon::Profile &profile = profiles.all()[size_t(profileOfRow(row))];
    std::u16string name = profile.valid ? profile.name : strings.frontEnd(0x69, "Invalid profile");
    float ty = menus.drawMenuRow(canvas, {atLeft(originX, 112.f), y}, name, {0, s, menus.fontLine(0) * es, sel, kText});
    if (!profile.valid)
    {
        return;
    }
    char percent[16];
    std::snprintf(percent, sizeof percent, "%d%%", profile.percent);
    canvas.text({atRight(originX, kRight), ty}, argon::widenLatin1(percent), {0, sel ? menu::blinkColour(kText, MenuKit::menuBlink()) : kText, 1, s, s});
}

// arrows icons\up.dds (112, 181) / icons\down.dds (112, 335), 40 x 20, when the list scrolls that way
void ChooseSavePage::drawScrollArrows(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    if (firstShown > 0)
    {
        menus.drawScrollArrow(canvas, "BUTU", atLeft(originX, 112.f), 181.f);
    }
    if (firstShown + kRowsShown < rowCount())
    {
        menus.drawScrollArrow(canvas, "BUTD", atLeft(originX, 112.f), 335.f);
    }
}
