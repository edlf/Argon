// Argon: tests for the menu helpers' arithmetic (src\menu_common.hpp). No window or GPU: run with
// test.bat (or ctest in build\). The "legacy" checks compare with the code the pages had inline before the helpers,
// so a change to a helper that would move a pixel shows up here.
#include "ui/menu_common.hpp"
#include "ui/menu_list.hpp"
#include "check.hpp"
#include "ui/shell_slide.hpp"
#include "logic/ghost.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;
#define CHECK_NEAR(a, b, eps) CHECK(std::fabs(double(a) - double(b)) <= (eps))

bool sameBits(float a, float b)
{
    return std::memcmp(&a, &b, sizeof a) == 0;
}

// ---- the code the pages had before (kept here as the reference)
float legacyBlink(double seconds)
{
    int vb = int(seconds * 60.0) % 360;
    return std::sin(float(vb) * 10.f * 3.14159265f / 180.f) * 0.25f + 0.75f;
}
uint32_t legacyBlinkColour(uint32_t c, float blink)
{
    auto ch = [&](int sh) { return uint32_t(std::clamp(int(float((c >> sh) & 255u) * blink), 0, 255)) << sh; };
    return (c & 0xff000000u) | ch(16) | ch(8) | ch(0);
}

void testBlinkFactor()
{
    using menu::blinkFactor;
    CHECK_NEAR(blinkFactor(0.0), 0.75, 1e-6);          // sin 0
    CHECK_NEAR(blinkFactor(9.0 / 60.0 + 1e-9), 1.0, 1e-6);    // vblank 9: 90 deg, the brightest
    CHECK_NEAR(blinkFactor(27.0 / 60.0 + 1e-9), 0.5, 1e-6);   // vblank 27: 270 deg, the dimmest
    // always 0.5..1, one cycle every 36 vblanks, the counter wrapping at 360 vblanks (6 s)
    for (int vb = 0; vb < 720; ++vb)
    {
        double t = vb / 60.0 + 1e-9;
        float f = blinkFactor(t);
        CHECK(f >= 0.5f - 1e-6f && f <= 1.f + 1e-6f);
        CHECK_NEAR(f, blinkFactor((vb + 36) / 60.0 + 1e-9), 1e-5);
        CHECK(sameBits(f, blinkFactor(t + 6.0)));
        CHECK(sameBits(f, legacyBlink(t)));
    }
}

void testBlinkColour()
{
    using menu::blinkColour;
    CHECK(blinkColour(menu::kText, 1.f) == menu::kText);                 // factor 1: unchanged
    CHECK(blinkColour(0x80ffffffu, 0.f) == 0x80000000u);                 // factor 0: black, alpha kept
    CHECK(blinkColour(0xff85dde5u, 0.5f) == 0xff426e72u);                // each channel halved (truncated)
    CHECK(blinkColour(0x40808080u, 2.f) == 0x40ffffffu);                 // clamped at 255
    CHECK((blinkColour(0x12345678u, 0.7f) & 0xff000000u) == 0x12000000u);
    const uint32_t cols[] = {menu::kText, menu::kGrey, menu::kOrange, menu::kYellow, menu::kHead, (menu::kText & 0x00ffffffu) | 0x80000000u, 0u, 0xffffffffu};
    for (uint32_t c : cols)
    {
        for (int vb = 0; vb < 360; ++vb)
        {
            float f = legacyBlink(vb / 60.0 + 1e-9);
            CHECK(blinkColour(c, f) == legacyBlinkColour(c, f));
        }
    }
}

void testStepCursor()
{
    using menu::stepCursor;
    int s = 3;
    CHECK(!stepCursor(s, 5, false, false) && s == 3);   // no input: nothing
    s = 4;
    CHECK(stepCursor(s, 5, false, true) && s == 0);   // down wraps to the top
    s = 0;
    CHECK(stepCursor(s, 5, true, false) && s == 4);   // up wraps to the bottom
    s = 2;
    CHECK(stepCursor(s, 5, true, true) && s == 2);    // both: back where it was, but pressed
    s = 0;
    CHECK(stepCursor(s, 1, false, true) && s == 0);   // one row
    s = 7;
    CHECK(!stepCursor(s, 0, true, false) && s == 7);  // no rows: untouched
    s = 0;
    CHECK(stepCursor(s, 4, true, false, menu::ListOrder::BottomUp) && s == 1);   // bottom-up (Game / Sound Settings)
    s = 0;
    CHECK(stepCursor(s, 4, false, true, menu::ListOrder::BottomUp) && s == 3);
    // against the pages' old code: up (sel + n - 1) % n, down (sel + 1) % n; Settings the other way round
    for (int n = 1; n <= 12; ++n)
    {
        for (int sel = 0; sel < n; ++sel)
        {
            for (int in = 0; in < 4; ++in)
            {
                bool up = in & 1, down = in & 2;
                for (int inv = 0; inv < 2; ++inv)
                {
                    int want = sel;
                    if (!inv)
                    {
                        if (up)
                        {
                            want = (want + n - 1) % n;
                        }
                        if (down)
                        {
                            want = (want + 1) % n;
                        }
                    }
                    else
                    {
                        if (up)
                        {
                            want = (want + 1) % n;
                        }
                        if (down)
                        {
                            want = (want + n - 1) % n;
                        }
                    }
                    int got = sel;
                    CHECK(stepCursor(got, n, up, down, inv != 0 ? menu::ListOrder::BottomUp : menu::ListOrder::TopDown) == (up || down));
                    CHECK(got == want);
                }
            }
        }
    }
}

void testSlideRows()
{
    using menu::kRowFrom;
    using menu::kRowX;
    // in: 640 -> 62 in 10 steps, never past 62; bit for bit the pages' old max(62, x - (640 - 62) / 10)
    std::vector<float> xs(9, kRowFrom), old = xs;
    for (int step = 1; step <= 12; ++step)
    {
        menu::slideRows(xs, kRowFrom, kRowX);
        for (float &x : old)
        {
            x = std::max(62.f, x - (640.f - 62.f) / 10.f);
        }
        for (size_t i = 0; i < xs.size(); ++i)
        {
            CHECK(sameBits(xs[i], old[i]));
            CHECK(xs[i] >= kRowX);
        }
        if (step < 10)
        {
            CHECK(xs[0] > kRowX + 1.f);
        }
    }
    CHECK(xs[0] == kRowX);
    // out (the main menu leaving): 62 -> 640, never past 640; the main menu's old two-way formula
    std::vector<float> m(7, kRowX), mo = m;
    for (int step = 1; step <= 12; ++step)
    {
        menu::slideRows(m, kRowX, kRowFrom);
        float from = kRowX, to = kRowFrom, st = (to - from) / 10.f;
        for (float &x : mo)
        {
            x = st < 0 ? std::max(to, x + st) : std::min(to, x + st);
        }
        for (size_t i = 0; i < m.size(); ++i)
        {
            CHECK(sameBits(m[i], mo[i]));
            CHECK(m[i] <= kRowFrom);
        }
    }
    CHECK(m[0] == kRowFrom);
    // a row already in place stays; one part way keeps moving from where it is
    float part[2] = {kRowX, 300.f};
    menu::slideRows(part, kRowFrom, kRowX);
    CHECK(part[0] == kRowX);
    CHECK_NEAR(part[1], 300.f - 57.8f, 1e-3);
}

void testScrollTicker()
{
    using menu::scrollTicker;
    const float w = 300.f;
    // the game's speed: a second of frames at any rate = 60 of its 2 px steps, at 60 / 144 / 240 Hz alike
    for (int hz : {30, 60, 75, 120, 144, 165, 240})
    {
        float x = 640.f;
        for (int f = 0; f < hz; ++f)
        {
            x = scrollTicker(x, 1.f / float(hz), w);
        }
        CHECK_NEAR(x, 640.f - 120.f, 1e-2);
    }
    // per frame at 144 Hz under a pixel (smooth), at 60 Hz the old 2 px
    CHECK_NEAR(640.f - scrollTicker(640.f, 1.f / 144.f, w), 120.f / 144.f, 1e-4);
    CHECK_NEAR(640.f - scrollTicker(640.f, 1.f / 60.f, w), 2.f, 1e-4);
    // wraps to 640 once past -(width + 19), not before
    CHECK(scrollTicker(-(w + 19.f) + 0.5f, 0.001f, w) < 0.f);
    CHECK(scrollTicker(-(w + 19.f) + 0.5f, 0.01f, w) == menu::kRowFrom);
    CHECK(scrollTicker(100.f, 0.f, w) == 100.f);
    // a full loop takes as long as the game's: (640 + width + 19) / 2 frames at 60 Hz
    float x = 640.f;
    int frames = 0;
    while (true)
    {
        float nx = scrollTicker(x, 1.f / 144.f, w);
        ++frames;
        if (nx > x)
        {
            break;
        }
        x = nx;
    }
    CHECK_NEAR(frames / 144.0, (640.0 + w + 19.0) / 2.0 / 60.0, 2.0 / 144.0);
}

void testTickerWrapsAtTheGamesPointIn4By3()
{
    CHECK(menu::tickerWrapWidth(300.f, 0.f) == 300.f);
}

void testWidescreenTickerRunsPastThePanelsLeftEnd()
{
    const float ox = 106.67f, textWidth = 300.f;   // 16:9
    const float wrapX = -(menu::tickerWrapWidth(textWidth, ox) + 19.f);
    const float textEnd = menu::atRight(ox, wrapX) + textWidth, panelLeft = menu::atLeft(ox, menu::kTickerX);
    CHECK_NEAR(panelLeft - textEnd, menu::kTickerX + 19.f, 1e-3);
}

void testInkCentredY()
{
    using menu::inkCentredY;
    // the ticker: panel 370..390 at 0.75: the capitals' ink (1..16 x 0.75 below y) centred on 380
    float y = inkCentredY(370.f, 20.f, 0.75f);
    CHECK_NEAR(y + menu::kFont0CapTop * 0.75f - 370.f, 390.f - (y + menu::kFont0Baseline * 0.75f), 1e-4);
    CHECK_NEAR(y, 373.625f, 1e-4);
    CHECK(y > 372.f);   // lower than the game's 372, which sits the text high in the panel
    // at scale 1 in a 20 box: 1.5 below its top
    CHECK_NEAR(inkCentredY(0.f, 20.f, 1.f), 1.5f, 1e-5);
}

void testRowPanel()
{
    using menu::rowPanel;
    const float w = 120.f, h = 26.25f;
    // left (the list pages: x - h / 2, w + h), right (Course Select's routes, Tournament's choices), centre (pause)
    menu::RowPanel l = rowPanel(62.f, w, h, 0), r = rowPanel(568.f, w, h, 1), c = rowPanel(320.f, w, h, 2);
    CHECK(sameBits(l.x, 62.f - h * 0.5f) && sameBits(l.w, w + h));
    CHECK(sameBits(r.x, 568.f - w - h * 0.5f) && sameBits(r.w, w + h));
    CHECK(sameBits(c.x, 320.f - w * 0.5f - h * 0.5f) && sameBits(c.w, w + h));
    // the text sits h / 2 in from the panel's edges whatever the alignment
    CHECK_NEAR(l.x + h * 0.5f, 62.f, 1e-4);
    CHECK_NEAR(r.x + r.w - h * 0.5f, 568.f, 1e-4);
    CHECK_NEAR(c.x + c.w * 0.5f, 320.f, 1e-4);
}

void testRowTextY()
{
    using menu::rowTextY;
    // the text (line x s high) centred in the panel; as the pages' old y + (h - line * s) * 0.5
    const float line0 = 20.f, line1 = 35.f;
    CHECK(sameBits(rowTextY(100.f, line0, line0, 0.75f), 100.f + (line0 - line0 * 0.75f) * 0.5f));
    CHECK(sameBits(rowTextY(116.f, line1 * 0.7f, line1, 0.7f), 116.f));   // pause: h = line x s: at the top
    float y = 205.f, h = line0 * 1.5f, s = 1.5f * 0.75f, ty = rowTextY(y, h, line0, s);   // Choose Save, selected
    CHECK_NEAR(ty + line0 * s * 0.5f, y + h * 0.5f, 1e-4);
}

ghost::Lap makeLap(int n, float dt, float lapTime)
{   // a straight run along x, turning 90 deg about y over the lap
    ghost::Lap l;
    for (int i = 0; i < n; ++i)
    {
        ghost::Sample s;
        s.seconds = dt * float(i + 1);
        s.pos = glm::vec3(10.f * float(i), 0.f, 0.f);
        s.rot = glm::angleAxis(glm::radians(90.f) * float(i) / float(n - 1), glm::vec3(0, 1, 0));
        s.jet = float(i);
        l.samples.push_back(s);
    }
    l.time = lapTime;
    return l;
}

void testGhostSample()
{
    ghost::Lap lap = makeLap(11, 0.5f, 5.5f);
    ghost::Sample s;
    CHECK(ghost::sampleAt(lap, 0.5f, s) && s.pos.x == 0.f);          // on a sample: that sample
    CHECK(ghost::sampleAt(lap, 2.5f, s) && std::fabs(s.pos.x - 40.f) < 1e-4f);
    CHECK(ghost::sampleAt(lap, 2.75f, s) && std::fabs(s.pos.x - 45.f) < 1e-3f);   // halfway: blended
    CHECK_NEAR(s.jet, 4.5f, 1e-4);
    CHECK_NEAR(glm::degrees(glm::angle(s.rot)), 40.5f, 0.05);   // slerp halfway between 36 and 45 deg
    CHECK(ghost::sampleAt(lap, 0.f, s) && s.pos.x == 0.f);      // before the first sample: the first
    CHECK(ghost::sampleAt(lap, 5.5f, s) && std::fabs(s.pos.x - 100.f) < 1e-4f);   // the last one
    CHECK(!ghost::sampleAt(lap, 5.51f, s));                     // past the end: hidden
    ghost::Lap empty;
    CHECK(!ghost::sampleAt(empty, 1.f, s));
    // every time in between lies between its two samples
    for (int i = 0; i < 200; ++i)
    {
        float t = 0.5f + 5.f * float(i) / 199.f;
        CHECK(ghost::sampleAt(lap, t, s));
        float k = (t - 0.5f) / 0.5f;
        CHECK_NEAR(s.pos.x, 10.f * k, 1e-3);
    }
}

void testGhostReplaces()
{
    ghost::Lap best, a = makeLap(5, 1.f, 50.f), b = makeLap(5, 1.f, 48.f), c = makeLap(5, 1.f, 52.f), open = makeLap(5, 1.f, 0.f);
    CHECK(ghost::replaces(a, best));       // no ghost yet: any finished lap
    CHECK(!ghost::replaces(open, best));   // a lap still being recorded: never
    CHECK(ghost::replaces(b, a));          // faster
    CHECK(!ghost::replaces(c, a));         // slower
    CHECK(!ghost::replaces(a, a));         // as fast: the old one stays
}

void testGhostFile()
{
    ghost::Lap lap = makeLap(30, 1.f / 60.f, 0.5f);
    lap.pilot = 10;
    lap.track = 13;
    lap.speed = 2;
    std::vector<uint8_t> d = ghost::encode(lap);
    CHECK(d.size() == 28 + 30 * 36);
    ghost::Lap back;
    CHECK(ghost::decode(d, back));
    CHECK(back.pilot == 10 && back.track == 13 && back.speed == 2 && back.time == 0.5f && back.samples.size() == 30);
    bool same = true;
    for (size_t i = 0; i < 30; ++i)
    {
        const ghost::Sample &a = lap.samples[i], &b = back.samples[i];
        same = same && a.seconds == b.seconds && a.pos == b.pos && a.rot == b.rot && a.jet == b.jet;
    }
    CHECK(same);
    // broken files leave the lap alone
    ghost::Lap keep = makeLap(3, 1.f, 3.f);
    std::vector<uint8_t> shortFile(d.begin(), d.end() - 1), badMagic = d, outOfOrder = d;
    badMagic[0] = 'X';
    float late = 9.f;
    std::memcpy(outOfOrder.data() + 28, &late, 4);   // the first sample after the second
    CHECK(!ghost::decode(shortFile, keep) && keep.samples.size() == 3);
    CHECK(!ghost::decode(badMagic, keep) && keep.samples.size() == 3);
    CHECK(!ghost::decode(outOfOrder, keep) && keep.samples.size() == 3);
    CHECK(!ghost::decode({}, keep));
}

void testConstants()
{
    static_assert(menu::fe::kFe == 1.f);
    static_assert(menu::race::kFe == 1.f);
    static_assert(menu::kRowX == 62.f && menu::kRowFrom == 640.f && menu::kRight == 568.f);
    static_assert(menu::kHintY == 397.f && menu::kHintScale == 1.f);
    static_assert(menu::kText == 0xff85dde5u && menu::kGrey == 0xff555555u && menu::kOrange == 0xffffbe64u);
}
// the widescreen margin: 4:3 window (ox 0) no shift; 16:9 (ox 106.67) the game's -41 margin; in between 52 from the edge
void testSafeShiftWindows()
{
    CHECK(menu::safeShift(0.f) == 0.f);
    CHECK_NEAR(menu::safeShift(106.67f), -93.f, 1e-4);
    CHECK_NEAR(menu::safeShift(500.f), -93.f, 1e-4);
    CHECK_NEAR(menu::safeShift(40.f), -40.f, 1e-4);
}

// the widescreen flag off: the 4:3 margin in the centred frame whatever the window
void testSafeShiftWithWidescreenOffIsNone()
{
    menu::widescreenLayout = false;
    const float shift = menu::safeShift(106.67f);
    menu::widescreenLayout = true;
    CHECK(shift == 0.f);
}

void testMarginAnchors()
{
    const float ox = 106.67f;
    CHECK_NEAR(menu::atLeft(ox, 62.f), ox + 62.f - 93.f, 1e-3);
    CHECK_NEAR(menu::atRight(ox, 568.f), ox + 568.f + 93.f, 1e-3);
    CHECK_NEAR(menu::spanWidth(ox, 536.f), 536.f + 186.f, 1e-3);
}

void testBoxAnchors()
{
    const float ox = 106.67f;
    CHECK_NEAR(menu::boxLeft(ox, 52.f, 536.f), ox + 52.f - 93.f, 1e-3);   // margin to margin: widens
    CHECK_NEAR(menu::boxWidth(ox, 52.f, 536.f), 722.f, 1e-3);
    CHECK_NEAR(menu::boxLeft(ox, 62.f, 64.f), ox + 62.f - 93.f, 1e-3);    // left half: the left margin
    CHECK_NEAR(menu::boxLeft(ox, 368.f, 200.f), ox + 368.f + 93.f, 1e-3); // right half: the right margin
    CHECK(menu::boxLeft(ox, 300.f, 40.f) == ox + 300.f);                 // centred: stays
    CHECK(menu::boxWidth(ox, 62.f, 64.f) == 64.f);
}
// ---- the shell's title / hint slides (0x316e0 states 1 / 4, element motion 0xad850)
menu::ShellSlide slideAfter(bool leaving, int ticks)
{
    menu::ShellSlide slide;
    if (leaving)
    {
        slide.leave();
    }
    else
    {
        slide.enter();
    }
    for (int i = 0; i < ticks; ++i)
    {
        slide.step();
    }
    return slide;
}

void testTitleSlidesInFromAbove()
{
    CHECK_NEAR(slideAfter(false, 0).titleY(200.f), 36.f - 300.f, 1e-4);   // 36 - (size + 100)
    CHECK_NEAR(slideAfter(false, 5).titleY(200.f), 36.f - 150.f, 1e-4);   // paced: a tenth a frame
    CHECK_NEAR(slideAfter(false, 10).titleY(200.f), 36.f, 1e-4);
    CHECK_NEAR(slideAfter(false, 30).titleY(200.f), 36.f, 1e-4);          // stops on the target
}

void testTitleRisesWhenLeaving()
{
    CHECK_NEAR(slideAfter(true, 0).titleY(200.f), 36.f, 1e-4);
    CHECK_NEAR(slideAfter(true, 6).titleY(200.f), 36.f - 180.f, 1e-4);   // where a 0.1 s leave cuts it
}

void testHintsSlideInByTheirWidth()
{
    CHECK_NEAR(slideAfter(false, 0).hintOutset(120.f), 120.f, 1e-4);
    CHECK_NEAR(slideAfter(false, 4).hintOutset(120.f), 72.f, 1e-4);
    CHECK_NEAR(slideAfter(false, 10).hintOutset(120.f), 0.f, 1e-4);
}

void testHintsLeaveAtTenPixels()
{
    CHECK_NEAR(slideAfter(true, 6).hintOutset(120.f), 60.f, 1e-4);    // unpaced: 10 px a frame
    CHECK_NEAR(slideAfter(true, 40).hintOutset(120.f), 220.f, 1e-4);  // stops at width + 100
}

void testShownSlideIsAtRest()
{
    const menu::ShellSlide slide;
    CHECK(slide.titleY(200.f) == 36.f);
    CHECK(slide.hintOutset(120.f) == 0.f);
}
}  // namespace

// ---- the model fade and the locked character (0x30303)
void testModelFadeStartsFull()
{
    CHECK_NEAR(menu::modelFade(0.0, false), 1.0, 1e-6);
    CHECK_NEAR(menu::modelFade(0.25, false), 0.75, 1e-6);
    CHECK(menu::modelFade(2.0, false) == 0.f);
}

void testLockedCharacterKeepsAFifthOfTheEcho()
{
    CHECK_NEAR(menu::modelFade(2.0, true), 0.2, 1e-6);
    CHECK_NEAR(menu::modelFade(-1.0, true), 0.2, 1e-6);
    CHECK_NEAR(menu::modelFade(0.5, true), 0.5, 1e-6);
}

void testNoFadeWithoutAModel()
{
    CHECK(menu::modelFade(-1.0, false) == 0.f);
}

// ---- the list window (ui\menu_list.hpp)
namespace
{
// FE OPTIONS' rows: 100 + 30 i, 26 high, above the ticker (368)
constexpr menu::ListLayout kOptionsLayout{100.f, 30.f, 26.f, 368.f};

void testAllRowsShownWhenTheyFit()
{
    CHECK(menu::rowsThatFit(kOptionsLayout, 9) == 9);   // the last row ends at 366
}

void testOverflowShowsEveryRowEndingAboveTheLimit()
{
    CHECK(menu::rowsThatFit(kOptionsLayout, 10) == 9);   // 9 rows end at 366, the scroll mark may pass 368
}

void testAtLeastOneRowShown()
{
    CHECK(menu::rowsThatFit(menu::ListLayout{100.f, 30.f, 26.f, 110.f}, 5) == 1);
}

void testArgonOptionsSmallRowsShowTen()
{
    const menu::ListLayout settings{120.f, menu::kSmallRows.step, 20.f, 368.f};
    CHECK(menu::rowsThatFit(settings, 12) == 10);   // 10 rows end at 356, an 11th would end at 380
}

void testNoRowsFitNone()
{
    CHECK(menu::rowsThatFit(kOptionsLayout, 0) == 0);
}

void testWindowStaysUntilTheCursorPassesItsEdge()
{
    menu::ScrollWindow window;
    window.follow(0, 10, 4);
    window.follow(3, 10, 4);
    CHECK(window.first() == 0);
}

void testWindowJumpsOneRowPastTheBottom()
{
    menu::ScrollWindow window;
    window.follow(0, 10, 4);
    window.follow(4, 10, 4);
    CHECK(window.first() == 1 && window.end() == 5);
}

void testWindowJumpsToTheTopOnWrap()
{
    menu::ScrollWindow window;
    window.follow(9, 10, 4);
    window.follow(0, 10, 4);
    CHECK(window.first() == 0);
}

void testWindowShowsTheEndOnWrapUp()
{
    menu::ScrollWindow window;
    window.follow(0, 10, 4);
    window.follow(9, 10, 4);
    CHECK(window.first() == 6 && window.end() == 10);
}

void testScrollMarksFollowTheHiddenRows()
{
    menu::ScrollWindow window;
    window.follow(0, 10, 4);
    CHECK(!window.moreAbove() && window.moreBelow());
    window.follow(9, 10, 4);
    CHECK(window.moreAbove() && !window.moreBelow());
}

void testShortListHasNoScrollMarks()
{
    menu::ScrollWindow window;
    window.follow(2, 3, 3);
    CHECK(window.first() == 0 && window.end() == 3 && !window.moreAbove() && !window.moreBelow());
}

void testWindowShrinksWithTheList()
{
    menu::ScrollWindow window;
    window.follow(9, 10, 4);
    window.follow(2, 5, 4);
    CHECK(window.first() == 1 && window.end() == 5);
}
}  // namespace

int main()
{
    testBlinkFactor();
    testBlinkColour();
    testStepCursor();
    testSlideRows();
    testScrollTicker();
    testTickerWrapsAtTheGamesPointIn4By3();
    testWidescreenTickerRunsPastThePanelsLeftEnd();
    testInkCentredY();
    testRowPanel();
    testRowTextY();
    testConstants();
    testGhostSample();
    testGhostReplaces();
    testGhostFile();
    testSafeShiftWindows();
    testSafeShiftWithWidescreenOffIsNone();
    testMarginAnchors();
    testBoxAnchors();
    testTitleSlidesInFromAbove();
    testTitleRisesWhenLeaving();
    testHintsSlideInByTheirWidth();
    testHintsLeaveAtTenPixels();
    testShownSlideIsAtRest();
    testModelFadeStartsFull();
    testLockedCharacterKeepsAFifthOfTheEcho();
    testNoFadeWithoutAModel();
    testAllRowsShownWhenTheyFit();
    testOverflowShowsEveryRowEndingAboveTheLimit();
    testAtLeastOneRowShown();
    testArgonOptionsSmallRowsShowTen();
    testNoRowsFitNone();
    testWindowStaysUntilTheCursorPassesItsEdge();
    testWindowJumpsOneRowPastTheBottom();
    testWindowJumpsToTheTopOnWrap();
    testWindowShowsTheEndOnWrapUp();
    testScrollMarksFollowTheHiddenRows();
    testShortListHasNoScrollMarks();
    testWindowShrinksWithTheList();
    std::printf("menu_tests: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
