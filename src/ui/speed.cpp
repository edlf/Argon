// Argon: FE SPEEDSELECT (vtable 0x19d720: build 0x33560, enter 0x34200, input 0x33ac0, update 0x33fa0,
// ticker 0x343e0) and FE TRACK INFO "Track Data" (vtable 0x19e300: build 0x36a60, enter 0x37370, input 0x371e0,
// update 0x37800, draw 0x36840); docs\frontend\race-setup.md "Select Speed and Track Data".
#include "ui/speed.hpp"
#include "track/course_folder.hpp"

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

// ---- FE SPEEDSELECT

// enter (0x34200): hints Back / Select (fe 0x3d; Time Attack 0x32); the cursor on the game speed, at most the
// highest open one
void SpeedSelectPage::open(int pilotId, int trackId)
{
    pilot = pilotId;
    track = trackId;
    selected = std::min(std::clamp(speedClass, 0, kSpeeds - 1), rules.speedMax());
    shell.goTo(menu::PageSpeedSelect);
}

std::u16string SpeedSelectPage::title() const
{
    return strings.frontEnd(0x2bc, "Select Speed");
}

bool SpeedSelectPage::selectedIsOpen() const
{
    return selected <= rules.speedMax();
}

// input (0x33ac0): up / down (wrapping 0..4), A on an open speed sets it and goes on (Quick Race: LOADER; Time
// Attack FE GHOSTSELECT, Tournament FE GOALDISPLAY), on a locked one the bad-select sound; B back to FE COURSE SELECT
void SpeedSelectPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::Back);
        shell.goTo(menu::PageCourseSelect);
        return;
    }
    if (menus.menuCursor(selected, kSpeeds, in))
    {
        play(menu::UiEvent::Cursor);
    }
    if (!in.a)
    {
        return;
    }
    if (!selectedIsOpen())
    {
        play(menu::UiEvent::Refused);
        return;
    }
    pickSpeed();
}

// update 0x33fa0: Time Attack FE GHOSTSELECT, else LOADER (no sound of its own)
void SpeedSelectPage::pickSpeed()
{
    play(menu::UiEvent::Pick);
    speedClass = selected;
    modes.current().speedPicked(pilot, track);
}

// update 0x33fa0: the speedometer (frontend\speedselect) with its needle on the cursor's speed
FeScene SpeedSelectPage::scene()
{
    return shell.models().speedometerScene(selected);
}

// build 0x33560: the speeds (infotext 0xfa + i: Novice, Amateur, Expert, Master, Redshift) at (62, 100 + 40 i),
// font 1 x 0.75 (as the main menu rows); locked ones 0xff555555
void SpeedSelectPage::draw(const FeCanvas &canvas)
{
    const float originX = canvas.originX();
    const float scale = 0.75f * kFe, h = menus.fontLine(1) * 0.75f;
    for (int i = 0; i < kSpeeds; ++i)
    {
        menus.drawMenuRow(canvas,
                          {atLeft(originX, kRowX), 100.f + 40.f * float(i)},
                          strings.info(0xfa + i, "Speed"),
                          {1, scale, h, i == selected, i <= rules.speedMax() ? kText : kGrey});
    }
    menus.drawHints(canvas,
                    {strings.frontEnd(0x33, "[BUTB] - Back"), // enter 0x34200: 0x32 in Time Attack, else 0x3d
                     strings.frontEnd(modes.current().speedSelectHint(), "Select - [BUTA]")});
}

// 0x343e0: a locked speed's unlock hint (fe 0x116 + speed), else none
std::optional<std::u16string> SpeedSelectPage::ticker() const
{
    return selectedIsOpen() ? std::u16string() : strings.frontEnd(0x116 + selected, "");
}

// ---- FE TRACK INFO

// FE COURSE SELECT, Y on an open track (not Mars, first route): this page (0x1da10 sets +0x1a48; 0x1d860)
void TrackInfoPage::open(int trackId)
{
    track = trackId;
    firstLine = 0;
    shell.goTo(menu::PageTrackInfo);
}

std::u16string TrackInfoPage::title() const
{
    return strings.frontEnd(0x190, "Track Data");
}

void TrackInfoPage::enter()
{
    const std::string name = "track_" + courseFolder(infoIndex());
    if (!painter.texture(name))
    {
        painter.loadTexture(name, gameFolder + "/data/frontend/" + name + "/" + name + ".dds");
    }
}

int TrackInfoPage::infoIndex() const
{
    return track >= 0x10 ? 0x10 : track & 7;
}

// strings.str per track t (0..7; second routes t - 8, Mars 0x10 / 0x11 -> 0x10): 0x640 + 10 t length in km,
// 0x643 + 10 t in miles (Speed Units), 0x641 + 10 t location, 0x642 + 10 t description; Mars 0x690..0x693
std::u16string TrackInfoPage::description() const
{
    return strings.general(isMars() ? 0x692 : 0x642 + 10 * infoIndex(), "");
}

int TrackInfoPage::descriptionLines() const
{
    return int(menus.wrapText(description(), 0, kFe, menus.wrapWidth()).size());
}

// input (0x371e0): B back to FE COURSE SELECT; up / down scroll the description by the 4 lines shown
void TrackInfoPage::input(const menu::MenuInput &in)
{
    if (in.b)
    {
        play(menu::UiEvent::Back);
        shell.goTo(menu::PageCourseSelect);
        return;
    }
    if (in.up && firstLine > 0)
    {
        firstLine = std::max(0, firstLine - kLinesShown);
    }
    if (in.down && firstLine + kLinesShown < descriptionLines())
    {
        firstLine += kLinesShown;
    }
}

// build 0x36a60 / enter 0x37370 / draw 0x36840: first the track's picture behind the top, then the texts
void TrackInfoPage::draw(const FeCanvas &canvas)
{
    drawTrackPicture(canvas);
    drawFacts(canvas);
    drawDescription(canvas);
    menus.drawHints(canvas, {strings.frontEnd(0x33, "[BUTB] - Back"), std::u16string()});
    if (descriptionLines() > kLinesShown)   // footer, 24 below
    {
        menus.drawHints(canvas, {std::u16string(), strings.frontEnd(0x3a, "[BUTUFIX][BUTDFIX] - Scroll Track Data"), kHintScale, kHintY + 24.f});
    }
}

// enter 0x37370 loads data\frontend\track_<t>\track_<t>.dds (shell +0x5a68; second routes as the first, Terraformed
// Mars as Mars); draw 0x36840 puts it through hud_draw_rotated_quad (320, 150, 0 deg, -128, 128, 640, white),
// additive, clipped by a viewport (52, 76) 536 x 188
void TrackInfoPage::drawTrackPicture(const FeCanvas &canvas) const
{
    const std::string name = "track_" + courseFolder(infoIndex());
    const PictureQuad picture{painter.texture(name), {{320.f, 150.f}, 0.f, -128.f, 128.f, 640.f}, ImVec4(52.f, 76.f, 536.f, 188.f)};
    drawPictureQuad(painter, canvas, picture);
}

// right-aligned at 568 (630 - 52 - 10), font 0 at 1.0 but the track: the track (infotext 300 + id, font 1, orange) at y 90, "Location:" (fe 0x17e,
// orange) 120, the location 140, "Length:" (0x17d, orange) 160, the length 180, "Home Track Of:" (0x180, orange)
// 200, the character (infotext 200 + t) 220, the nemesis (208 + t, once unlocked) 240 (both hidden for Mars)
void TrackInfoPage::drawFacts(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const int infoRow = infoIndex();
    const bool mars = isMars();
    const float x = atRight(originX, kRight);
    canvas.text({x, 90.f}, strings.info(300 + track, ""), {1, kOrange, 1, kFe, kFe});   // build 0x36a60: font 1 at 1.0
    canvas.text({x, 120.f}, strings.frontEnd(0x17e, "Location:"), {0, kOrange, 1, kFe, kFe});
    canvas.text({x, 140.f}, strings.general(mars ? 0x691 : 0x641 + 10 * infoRow, ""), {0, kText, 1, kFe, kFe});
    canvas.text({x, 160.f}, strings.frontEnd(0x17d, "Length:"), {0, kOrange, 1, kFe, kFe});
    int length = mars ? (speedMph ? 0x693 : 0x690) : (speedMph ? 0x643 : 0x640) + 10 * infoRow;
    canvas.text({x, 180.f}, strings.general(length, ""), {0, kText, 1, kFe, kFe});
    if (mars)
    {
        return;
    }
    canvas.text({x, 200.f}, strings.frontEnd(0x180, "Home Track Of:"), {0, kOrange, 1, kFe, kFe});
    canvas.text({x, 220.f}, strings.info(200 + infoRow, argon::pilotName(infoRow)), {0, kText, 1, kFe, kFe});
    if (rules.charUnlocked(infoRow + 8))
    {
        canvas.text({x, 240.f}, strings.info(208 + infoRow, argon::pilotName(infoRow + 8)), {0, kText, 1, kFe, kFe});
    }
}

// the description from (62, 280), 516 wide, 4 lines, in a frame (52, 265) 536 x 125 (0xc2310, as the records' frames)
void TrackInfoPage::drawDescription(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float left = atLeft(originX, 52.f);
    MenuKit::drawGradientFrame(canvas, {left, 265.f, atRight(originX, 588.f) - left, 125.f});
    auto lines = menus.wrapText(description(), 0, kFe, menus.wrapWidth());
    float lh = painter.font(0).ok ? painter.font(0).line : 19.f;
    for (int i = 0; i < kLinesShown && firstLine + i < int(lines.size()); ++i)
    {
        canvas.text({atLeft(originX, 62.f), 280.f + lh * float(i)}, lines[size_t(firstLine + i)], {0, kText, 0, kFe, kFe});
    }
    drawScrollArrows(canvas, int(lines.size()));
}

// the up / down arrows at (300, 268 / 368) 40 x 20, while there is more to scroll to
void TrackInfoPage::drawScrollArrows(const FeCanvas &canvas, int lines) const
{
    const float originX = canvas.originX();
    if (firstLine > 0)
    {
        menus.drawScrollArrow(canvas, "BUTU", originX + 300.f, 268.f);
    }
    if (firstLine + kLinesShown < lines)
    {
        menus.drawScrollArrow(canvas, "BUTD", originX + 300.f, 368.f);
    }
}
