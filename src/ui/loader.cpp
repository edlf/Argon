// Argon: the race loading screen ("LOADER", drawn by the loader thread 0x808e0 each frame while the
// track loads; docs\frontend\loading.md "Loading screen"). Quick Race: FE COURSE SELECT -> this -> the race.
#include "ui/loader.hpp"
#include "movie/loading_flyby.hpp"

#include <cmath>

#include "files/game_path.hpp"
#include "logic/replay_race.hpp"
#include "movie/story_scene.hpp"
#include "movie/story_scenes.hpp"
#include "logic/tourney_rules.hpp"
#include "ui/fe_shell.hpp"
#include "track/course_folder.hpp"
#include "ui/game_strings.hpp"
#include "ui/menu_common.hpp"
#include "ui/page_id.hpp"

using namespace menu::fe;

namespace
{
constexpr uint32_t kWhite = 0xffffffffu;
constexpr float kHintTextScale = 0.65f, kStatusScale = 0.7f;
constexpr int kFramesBeforeLoad = 2;
constexpr int kStoryTailFrames = 12;   // 0x9ae05: the scene's page pops 12 frames after it ended (0x439f78 > 12)
}  // namespace

void LoaderPage::open()
{
    phase = Phase::Showing;
    framesShown = 0;
    playerReady.fill(false);
    openedAtStep = shell.frameCount();
    const bool tournament = &sources.modes.current() == &sources.modes.tournament();
    nemesisRace = tournament && argon::tourney::isNemesisRace(sources.progress.tourneyLevel,
                                                              sources.rules.tourneyDone(sources.progress.tourneyLevel, sources.tourney.character));
    storyPending = false;
    if (tournament)
    {
        findStory();
    }
    shell.goTo(menu::PageLoader);
}

// 0x80354: a tournament's race opens with data\movies\<track>\<track>_<pilot>.nms (the table 0x1b4158); the disc
// lacks many, and then the loading screen shows as in the other modes
void LoaderPage::findStory()
{
    storyFolder = courseFolder(sources.track);
    storyFile = argon::story::preRaceScene(storyFolder, sources.pilot);
    storyPending = argon::gamePathExists(sources.gameFolder + "/data/movies/" + storyFolder + "/" + storyFile);
}

// loader_run's first pass (0x80385): the scene's page instead of the loading screen; the flyby waits for it
void LoaderPage::startStory()
{
    storyPending = false;
    storyEndedStep = -1;
    if (sources.story->start(storyFolder, storyFile))
    {
        phase = Phase::Story;
        sources.flyby.stop();
    }
}

// the scene to its end or A / START (0x9af10) and its 12-frame hold (0x9ade3); then loader_run's second pass
// (0x803b1): the loading screen from its start (its clock +0x1a80: the Nemesis Challenge intro), the flyby, the 0.25 s
// transition (0x80556)
void LoaderPage::playStory(const menu::MenuInput &in)
{
    sources.story->update();
    if (in.a)
    {
        sources.story->skip();
    }
    if (!sources.story->finished())
    {
        return;
    }
    if (storyEndedStep < 0)
    {
        storyEndedStep = shell.frameCount();
    }
    if (shell.frameCount() - storyEndedStep < kStoryTailFrames)
    {
        return;
    }
    sources.story->stop();
    phase = Phase::Loaded;
    openedAtStep = shell.frameCount();
    sources.loading.startGlorySequence(storyFolder);
    sources.loading.startTransition();
}

float LoaderPage::seconds() const
{
    return float(shell.frameCount() - openedAtStep) / 60.f + clockShift;
}

// the track loads once the screen has been shown, then "Press [BUTA] to continue." waits for A. Like the game's
// loader thread the screen keeps animating: the track is read on a worker thread and put on the GPU a slice per
// frame (App::stepRaceLoad). The Nemesis Challenge intro takes no input (0x805b5).
void LoaderPage::input(const menu::MenuInput &in)
{
    if (phase == Phase::Showing && framesShown >= kFramesBeforeLoad)
    {
        phase = sources.loading.beginRaceLoad() ? Phase::Loading : Phase::Loaded;
    }
    if (phase == Phase::Loading && sources.loading.stepRaceLoad())
    {
        phase = Phase::Loaded;
    }
    if (phase == Phase::Loaded && storyPending)
    {
        startStory();
    }
    if (phase == Phase::Story)
    {
        playStory(in);
        return;
    }
    if (sources.flyby.on())
    {
        sources.loading.updateSequence();
    }
    if (showingNemesisIntro())
    {
        return;
    }
    if (in.y && !nemesisRace)
    {
        sources.modes.current().nextLoaderHint();   // "Press [BUTY] for more hints." (0x8068a: not before the nemesis race)
    }
    if (sources.pads.multiplayerRace())
    {
        playersInput();
        return;
    }
    if (phase == Phase::Loaded && (in.a || loadsTheAttractDemo()))
    {
        sources.loading.finishQuickRace();
    }
}

// mode 4 (0x81052): each player presses A on their own controller; the race starts once all have
void LoaderPage::playersInput()
{
    if (phase != Phase::Loaded)
    {
        return;
    }
    bool all = true;
    for (int p = 0; p < sources.multiplayer.players(); ++p)
    {
        playerReady[size_t(p)] = playerReady[size_t(p)] || sources.players.portPresses(sources.pads.padOfPlayer(p)).a;
        all = all && playerReady[size_t(p)];
    }
    if (all)
    {
        sources.loading.finishQuickRace();
    }
}

// the attract demo's race starts as soon as it is loaded: no "Press [BUTA]" wait (seen in the game)
bool LoaderPage::loadsTheAttractDemo() const
{
    return sources.replay.playing() && sources.replay.kind() == ReplayRace::Kind::AttractDemo;
}

// 0x808e0, single player (mode != 2 / 4), after the background (the front-end echo): the Nemesis Challenge intro
// alone for its 6 s, else the title box, the hint and the status
void LoaderPage::draw(const FeCanvas &canvas)
{
    ++framesShown;
    if (phase == Phase::Story)
    {
        sources.story->drawOverlay(canvas);
        return;
    }
    if (storyPending)
    {
        drawStoryLoading(canvas);
        return;
    }
    if (showingNemesisIntro())
    {
        drawNemesisIntro(canvas);
        return;
    }
    drawTitleBox(canvas);
    drawHint(canvas);
    if (sources.pads.multiplayerRace() && phase == Phase::Loaded)
    {
        drawPlayerList(canvas);
        return;
    }
    drawStatus(canvas);
}

// 0x81acb: while the story scene loads, only infotext 0x210 "Loading..." in font 1, centred at (320, 256)
void LoaderPage::drawStoryLoading(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.text({originX + 320.f, 256.f}, strings.info(0x210, "Loading..."), {1, kWhite, 2, kFe, kFe});
}

// 0x81052: a row per player still to press A, at (margin + 20, 234 + 40 n), font 1 x 0.7 in the status colour over a
// highlighted panel: "Player n - Press [BUTA] to race." (race 0x537 + controller), or with the controller unplugged
// "Player n - Reconnect controller." and its port (0x53c / 0x544 + controller)
void LoaderPage::drawPlayerList(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const float scale = 0.7f * kFe;
    for (int p = 0; p < sources.multiplayer.players(); ++p)
    {
        if (playerReady[size_t(p)])
        {
            continue;
        }
        const int pad = sources.pads.padOfPlayer(p);
        const std::u16string text = sources.players.portConnected(pad) ? strings.race(0x537 + pad, "Press [BUTA] to race.")
                                                                    : strings.race(0x53c + pad, "Reconnect controller.") + u" " + strings.race(0x544 + pad, "");
        const float y = 234.f + 40.f * float(p), w = painter.textWidth(text, 1, scale), h = menus.fontLine(1) * scale;
        canvas.panel({atLeft(originX, 64.f), y - 4.f, w + 16.f, h + 8.f}, {true, true, 0xffffff00u, true, 0xffa08040u, 0xffa08040u, 1.f});
        canvas.text({atLeft(originX, 72.f), y}, text, {1, statusColour(), 0, scale, scale});
    }
}

// 0x80a12: race 0x535 "NEMESIS CHALLENGE" in font 1, centred on 320 (ui\nemesis_intro.cpp)
void LoaderPage::drawNemesisIntro(const FeCanvas &canvas) const
{
    for (const menu::IntroText &text : menu::NemesisIntro::texts(seconds()))
    {
        drawIntroText(canvas, text);
    }
}

// 0xb1270: the text's glyphs turned about its anchor (x, y) by its angle
void LoaderPage::drawIntroText(const FeCanvas &canvas, const menu::IntroText &text) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    const int firstVertex = drawList->VtxBuffer.Size;
    canvas.text({originX + text.x, text.y}, strings.race(0x535, "NEMESIS CHALLENGE"), {1, text.colour, 2, text.scale, text.scale});
    if (text.angleDegrees == 0.f)
    {
        return;
    }
    const float angle = text.angleDegrees * 3.14159265f / 180.f, cosine = std::cos(angle), sine = std::sin(angle);
    const ImVec2 anchor((originX + text.x) * pixelsPerUnit, text.y * pixelsPerUnit);
    for (int i = firstVertex; i < drawList->VtxBuffer.Size; ++i)
    {
        ImVec2 &position = drawList->VtxBuffer[i].pos;
        const float dx = position.x - anchor.x, dy = position.y - anchor.y;
        position = ImVec2(anchor.x + dx * cosine - dy * sine, anchor.y + dx * sine + dy * cosine);
    }
}

// title box: "Welcome to" (race 0x51a) / [F:1] track (infotext 300 + id) / [F:0] race 0x550 + id ("Home circuit of
// Tara Vivas"), font 0 at 1.0, at (62 + 8, 36 + 8); panel (62, 36) text + (120, 16), fill 0x80401000, scale 0.5;
// that pilot's portrait (data\hud icon id, track id < 16) 96 x 96 at (62 + text w + 16, 44)
void LoaderPage::drawTitleBox(const FeCanvas &canvas) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const float originX = canvas.originX();
    const int id = sources.track;
    const std::u16string lines[3] = {strings.race(0x51a, "Welcome to"), strings.info(300 + id, ""), strings.race(0x550 + id, "")};
    const int fonts[3] = {0, 1, 0};
    float w = 0, h = 0;
    for (int i = 0; i < 3; ++i)
    {
        w = std::max(w, painter.textWidth(lines[i], fonts[i], kFe));
        h += menus.fontLine(fonts[i]) * kFe;
    }
    canvas.panel({atLeft(originX, 62.f), 36.f, w + 120.f, h + 16.f}, {false, false, 0xffffff00u, true, 0x80401000u, 0x80401000u, 0.5f});
    float y = 44.f;
    for (int i = 0; i < 3; ++i)
    {
        canvas.text({atLeft(originX, 70.f), y}, lines[i], {fonts[i], kWhite, 0, kFe, kFe});
        y += menus.fontLine(fonts[i]) * kFe;
    }
    if (id >= 16)
    {
        return;
    }
    char name[16];
    std::snprintf(name, sizeof name, "pilot%02d", menu::kPilotPortrait[id]);
    if (const Found<const HudTexture> portrait = painter.texture(name))
    {
        float x = 62.f + w + 16.f;
        drawList->AddImage(portrait->image, ImVec2((originX + x) * pixelsPerUnit, 44.f * pixelsPerUnit), ImVec2((originX + x + 96.f) * pixelsPerUnit, 140.f * pixelsPerUnit));
    }
}

// hint panel (62, 170) 516 x 180 (no fill, scale 0.5): race 0x420 "Hint:" at (72, 172) 0.65; the hint (Tournament
// race 0x5dc + n, Quick Race 0x618 + n, Time Attack 0x64a + n) at (72, 260), 486 wide, 10 lines, centred on y 260;
// race 0x41f "Press [BUTY] for more hints." right-aligned at 560, bottom 346. Before the nemesis race (0x818b7 /
// 0x81978): race 0x425 instead of "Hint:", 0x426 as the text, no "Press [BUTY]" line.
void LoaderPage::drawHint(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    canvas.panel({atLeft(originX, 62.f), 170.f, spanWidth(originX, 516.f), 180.f}, {false, false, 0xffffff00u, false, 0u, 0u, 0.5f});
    const std::u16string header = nemesisRace ? strings.race(0x425, "") : strings.race(0x420, "Hint:");
    canvas.text({atLeft(originX, 72.f), 172.f}, header, {0, kWhite, 0, kHintTextScale * kFe, kHintTextScale * kFe});
    const int hint = nemesisRace ? 0x426 : sources.modes.current().loaderHint();
    auto lines = menus.wrapText(strings.race(hint, ""), 0, kFe, 486.f);
    if (lines.size() > 10)
    {
        lines.resize(10);
    }
    const float line = menus.fontLine(0) * kFe;
    float y = 260.f - line * float(lines.size()) * 0.5f;
    for (const auto &textLine : lines)
    {
        canvas.text({atLeft(originX, 72.f), y}, textLine, {0, kWhite, 0, kFe, kFe});
        y += line;
    }
    if (nemesisRace)
    {
        return;
    }
    canvas.text({atRight(originX, 560.f), 346.f - line * kHintTextScale},
                strings.race(0x41f, "Press [BUTY] for more hints."),
                {0, kWhite, 1, kHintTextScale * kFe, kHintTextScale * kFe});
}

// bottom right, right-aligned at 560 with the bottom at 436, 0.7, in a panel 8 px around it (bottom 444): loading:
// infotext 0x210 "Loading..."; done: race 0x519 "Press [BUTA] to continue." in a highlighted panel (fill 0xffa08040)
void LoaderPage::drawStatus(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const bool loaded = phase == Phase::Loaded;
    const std::u16string text = loaded ? strings.race(0x519, "Press [BUTA] to continue.") : strings.info(0x210, "Loading...");
    const float scale = kStatusScale * kFe;
    const float tw = painter.textWidth(text, 0, scale), th = menus.fontLine(0) * scale;
    canvas.panel({atRight(originX, 568.f) - (tw + 16.f), 444.f - (th + 16.f), tw + 16.f, th + 16.f},
                 {loaded, true, 0xffffff00u, loaded, 0xffa08040u, 0xffa08040u, 1.f});
    canvas.text({atRight(originX, 560.f), 436.f - th}, text, {0, statusColour(), 1, scale, scale});
}

// loading: grey (vblanks & 31) x 4 + 128; done: (n x 0x11, n x 0x11, 0) with n = the loader frame & 15
uint32_t LoaderPage::statusColour() const
{
    if (phase == Phase::Loaded)
    {
        uint32_t grey = uint32_t(shell.frameCount() & 15) * 0x11u;
        return 0xff000000u | (grey << 16) | (grey << 8);
    }
    uint32_t grey = std::min(255u, uint32_t(int(ImGui::GetTime() * 60.0) & 31) * 4u + 128u);
    return 0xff000000u | grey * 0x10101u;
}
